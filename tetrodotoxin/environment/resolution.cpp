// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/environment/resolution.hpp"

#include "tetrodotoxin/environment/origin.hpp"
#include "tetrodotoxin/package/archive/archive.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::System;
using namespace Perimortem::Utility;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin;

static constexpr auto selection_error_name(
    Package::Repository::SelectionError error) -> View::Bytes {
  switch (error) {
  case Package::Repository::SelectionError::NotDeclared:
    return "NotDeclared"_view;
  case Package::Repository::SelectionError::Unreadable:
    return "Unreadable"_view;
  case Package::Repository::SelectionError::InvalidFormat:
    return "InvalidFormat"_view;
  case Package::Repository::SelectionError::UnsupportedFormat:
    return "UnsupportedFormat"_view;
  case Package::Repository::SelectionError::PackageKeyMismatch:
    return "PackageKeyMismatch"_view;
  case Package::Repository::SelectionError::ArtifactMismatch:
    return "ArtifactMismatch"_view;
  case Package::Repository::SelectionError::ArtifactNotDeclared:
    return "ArtifactNotDeclared"_view;
  default:
    return "Unknown"_view;
  }
}

static_assert(
    selection_error_name(Package::Repository::SelectionError::Unknown) ==
    "Unknown"_view);
static_assert(
    selection_error_name(
        static_cast<Package::Repository::SelectionError>(Unsigned_8(-2))) ==
    "Unknown"_view);

enum class CacheMiss : Unsigned_8 {
  Unknown = Unsigned_8(-1),
  Missing = 0,
  Incomplete,
  VersionConflict,
};

using CacheSelection = Result<Package::Language::Monograph&, CacheMiss>;
using TransactionResult =
    Result<Package::Language::Monograph&, Package::Repository::SelectionError>;
using ResolutionResult =
    Result<Language::Dialect::Monograph&, Package::Repository::SelectionError>;

struct PackageKey {
  View::Bytes identity;
  Version version;
};

struct DependencyHop {
  View::Bytes alias;
  View::Bytes identity;
  Version version;
};

struct CompletedPackage {
  View::Bytes identity;
  Version version;
  Package::Language::Monograph& value;
};

template <typename SelectRestored, typename ReserveRestored>
class ResolutionState {
 public:
  ResolutionState(
      Errors& errors,
      Allocator::Arena& domain,
      Environment::Dialects& dialects,
      Environment::Retention& retention,
      Package::Repository::Repository& repository,
      SelectRestored select_restored,
      ReserveRestored reserve_restored)
      : temporary(),
        domain(domain),
        dialects(dialects),
        retention(retention),
        repository(repository),
        errors(errors),
        select_restored(select_restored),
        reserve_restored(reserve_restored),
        active_packages(temporary),
        active_hops(temporary),
        completed_packages(temporary),
        active_package_count(0),
        active_hop_count(0),
        rejection() {}

  auto restore(
      Count first_monograph,
      View::Bytes root_package_identity,
      Version root_package_version,
      Package::Language::Monograph& root_package) -> TransactionResult {
    PackageKey root_key = {
      .identity = root_package_identity,
      .version = root_package_version,
    };
    push_package(root_key);

    // Freeze the authored discovery boundary before restoration retains binary
    // descendants. Each descendant recurses through its parent and cannot enter
    // again as a top level request.
    const Count authored_monograph_count = retention.get_size();
    for (Count monograph_index = first_monograph;
         monograph_index < authored_monograph_count; monograph_index++) {
      Language::Dialect::Monograph& retained =
          retention.get_monograph(monograph_index);
      if (!retained.is<Package::Language::Monograph>()) {
        continue;
      }

      auto& package = static_cast<Package::Language::Monograph&>(retained);
      View::Vector<Package::Language::Dependency> dependencies =
          package.get_dependencies();
      View::Vector<Span> spans = package.get_dependency_spans();
      for (Count dependency_index = 0;
           dependency_index < dependencies.get_size(); dependency_index++) {
        Option<Environment::Origin> origin;
        retention.get_origin(monograph_index)
            .visit(
                []() {},
                [&](const Environment::Origin& authored) {
                  if (spans.get_size() != dependencies.get_size()) {
                    return;
                  }

                  origin = Environment::Origin(
                      authored.get_path(), authored.get_body(),
                      spans.get_data()[dependency_index]);
                });

        Bool report_published = False;
        Bool restored = restore_dependency(
            package, dependencies.get_data()[dependency_index], origin,
            report_published);
        if (!restored) {
          reject();
        }
      }
    }

    pop_package();
    if (rejection) {
      return *rejection;
    }

    return root_package;
  }

 private:
  auto reject() -> void {
    if (!rejection) {
      rejection = Package::Repository::SelectionError::Unknown;
    }
  }

  auto reject(Package::Repository::SelectionError error) -> void {
    if (!rejection ||
        *rejection == Package::Repository::SelectionError::Unknown) {
      rejection = error;
    }
  }

  auto push_package(const PackageKey& key) -> void {
    if (active_package_count == active_packages.get_size()) {
      active_packages.insert(key);
    } else {
      active_packages[active_package_count] = key;
    }
    active_package_count++;
  }

  auto pop_package() -> void { active_package_count--; }

  auto push_hop(const DependencyHop& hop) -> void {
    if (active_hop_count == active_hops.get_size()) {
      active_hops.insert(hop);
    } else {
      active_hops[active_hop_count] = hop;
    }
    active_hop_count++;
  }

  auto pop_hop() -> void { active_hop_count--; }

  auto publish_dependency_failure(
      Option<Environment::Origin> origin,
      Bool& report_published,
      View::Bytes reason) -> void {
    if (report_published) {
      return;
    }

    // Absence means the complete active chain is source free. The binary
    // failure retains its owner log and never acquires pretend source bytes.
    origin.visit(
        []() {},
        [&](const Environment::Origin& authored) {
          Errors::Report report(
              errors, authored.get_path(), authored.get_body(),
              authored.get_span());
          report << "Package dependency chain "_view;
          for (Count i = 0; i < active_hop_count; i++) {
            if (i != 0) {
              report << " then "_view;
            }

            const DependencyHop& hop = active_hops[i];
            report << hop.alias << " resolves "_view << hop.identity << '@'
                   << hop.version.get_major() << '.' << hop.version.get_minor();
          }

          report << " failed because "_view << reason << '.';
          report_published = True;
        });
  }

  auto restore_dependency(
      Package::Language::Monograph& owner,
      const Package::Language::Dependency& dependency,
      Option<Environment::Origin> origin,
      Bool& report_published) -> Bool {
    DependencyHop hop = {
      .alias = dependency.get_local_name(),
      .identity = dependency.get_package_name(),
      .version = dependency.get_version(),
    };
    push_hop(hop);

    // One method owns active chain mutation. Every rejection unwinds exactly
    // one hop without scattering stack changes through selection and recursion.
    Bool restored =
        restore_dependency_active(owner, dependency, origin, report_published);
    pop_hop();
    return restored;
  }

  auto restore_dependency_active(
      Package::Language::Monograph& owner,
      const Package::Language::Dependency& dependency,
      Option<Environment::Origin> origin,
      Bool& report_published) -> Bool {
    // Active keys precede persistent and transaction complete inventories. That
    // ordering distinguishes recursion from reuse after a branch has drained.
    for (Count i = 0; i < active_package_count; i++) {
      if (active_packages[i].identity != dependency.get_package_name()) {
        continue;
      }

      View::Bytes reason =
          active_packages[i].version == dependency.get_version()
              ? "the exact Package key is already active"_view
              : "the Package identity is already active with another Version"_view;
      publish_dependency_failure(origin, report_published, reason);
      return False;
    }

    CacheSelection cached = select_restored(
        dependency.get_package_name(), dependency.get_version());
    Bool incomplete_cache_entry = False;
    Option<Bool> cache_result = cached.visit(
        [&](Package::Language::Monograph& retained) -> Option<Bool> {
          Bool bound = owner.bind_dependency(dependency, retained);
          if (!bound) {
            publish_dependency_failure(
                origin, report_published,
                "the completed Package root rejected its exact Alias binding"_view);
          }
          return bound;
        },
        [&](CacheMiss miss) -> Option<Bool> {
          switch (miss) {
          case CacheMiss::Missing:
            return Option<Bool>();
          case CacheMiss::Incomplete:
            incomplete_cache_entry = True;
            return Option<Bool>();
          case CacheMiss::VersionConflict:
            publish_dependency_failure(
                origin, report_published,
                "the Package identity is already retained with another Version"_view);
            return False;
          default:
            publish_dependency_failure(
                origin, report_published,
                "the restored Package cache returned an unknown state"_view);
            return False;
          }
        });
    if (cache_result) {
      return *cache_result;
    }

    // A sibling may reuse a fully restored root before post pass promotes the
    // complete transaction into the persistent inventory.
    for (Count i = 0; i < completed_packages.get_size(); i++) {
      CompletedPackage& completed = completed_packages[i];
      if (completed.identity != dependency.get_package_name()) {
        continue;
      }

      if (completed.version != dependency.get_version()) {
        publish_dependency_failure(
            origin, report_published,
            "the Package identity completed with another Version"_view);
        return False;
      }

      Bool bound = owner.bind_dependency(dependency, completed.value);
      if (!bound) {
        publish_dependency_failure(
            origin, report_published,
            "the completed Package root rejected its exact Alias binding"_view);
      }
      return bound;
    }

    if (incomplete_cache_entry) {
      publish_dependency_failure(
          origin, report_published,
          "the exact Package key was already restored but did not complete"_view);
      return False;
    }

    // Repository owns selection detail and its typed result. Visiting Result
    // preserves that category without parsing supplemental owner log text.
    auto selected = repository.select_archive(
        dependency.get_package_name(), dependency.get_version());
    return selected.visit(
        [&](const Package::Archive::Archive& archive) {
          return restore_archive(
              owner, dependency, origin, report_published, archive);
        },
        [&](Package::Repository::SelectionError error) {
          reject(error);
          if (origin) {
            publish_dependency_failure(
                origin, report_published, selection_error_name(error));
          }
          return False;
        });
  }

  auto restore_archive(
      Package::Language::Monograph& owner,
      const Package::Language::Dependency& dependency,
      Option<Environment::Origin> origin,
      Bool& report_published,
      const Package::Archive::Archive& archive) -> Bool {
    // The selected Archive borrows Repository Arena storage. Resolution calls
    // the Monograph owners with the longer Workspace domain, so only envelope
    // values retained after this call cross that lifetime boundary here.
    View::Bytes restored_identity = domain.proxy(archive.get_identity());
    Managed::Vector<Package::Language::Dependency> dependencies(domain);
    View::Vector<Package::Language::Dependency> archive_dependencies =
        archive.get_dependencies();
    const auto* archive_dependency_data = archive_dependencies.get_data();
    for (Count i = 0; i < archive_dependencies.get_size(); i++) {
      Package::Language::Dependency retained_dependency(
          domain.proxy(archive_dependency_data[i].get_local_name()),
          domain.proxy(archive_dependency_data[i].get_package_name()),
          archive_dependency_data[i].get_version());
      dependencies.insert(retained_dependency);
    }

    Option<Language::Dialect&> package_dialect = dialects.find("Package"_view);
    if (!package_dialect) {
      publish_dependency_failure(
          origin, report_published,
          "the Package Dialect is not installed"_view);
      return False;
    }

    Package::Language::Monograph& restored_package =
        Package::Language::Monograph::create_source_free(
            domain, Documentation::get_empty(), *package_dialect, dependencies);
    retention.retain(restored_package, origin);

    // Reserve the key before the first concrete restore hook. Partial graph
    // mutation is durable and a later request cannot invoke the hook twice.
    reserve_restored(
        restored_identity, archive.get_version(), restored_package);
    PackageKey restored_key = {
      .identity = restored_identity,
      .version = archive.get_version(),
    };
    push_package(restored_key);

    // Archive order is discovery order. Missing Dialects and rejected payloads
    // stay independent so one Archive exposes every concrete owner failure.
    Bool rejected = False;
    View::Vector<Package::Archive::Member> members = archive.get_members();
    for (Count i = 0; i < members.get_size(); i++) {
      Option<Language::Dialect&> member_dialect =
          dialects.find(members.get_data()[i].get_dialect_name());
      if (!member_dialect) {
        publish_dependency_failure(
            origin, report_published,
            "an Archive member names a Dialect that is not installed"_view);
        rejected = True;
        continue;
      }

      // Payload stays borrowed for this call. The concrete Dialect receives
      // the destination domain and owns every byte retained from its payload.
      Option<Language::Dialect::Monograph&> restored_member =
          member_dialect->restore(domain, members.get_data()[i].get_payload());
      if (!restored_member) {
        publish_dependency_failure(
            origin, report_published,
            "an Archive member payload could not be restored"_view);
        rejected = True;
        continue;
      }

      retention.retain(*restored_member, origin);
      View::Bytes member_name =
          domain.proxy(members.get_data()[i].get_semantic_name());
      Bool member_bound =
          restored_package.bind_member(member_name, *restored_member);
      if (!member_bound) {
        publish_dependency_failure(
            origin, report_published,
            "an Archive member rejected its exact Alias binding"_view);
      }
      rejected |= !member_bound;
    }

    // The reconstructed Package owns its Dependency values. Recursion extends
    // the nearest authored Origin and one publication flag through every child.
    View::Vector<Package::Language::Dependency> retained_dependencies =
        restored_package.get_dependencies();
    const auto* retained_dependency_data = retained_dependencies.get_data();
    for (Count i = 0; i < retained_dependencies.get_size(); i++) {
      rejected |= !restore_dependency(
          restored_package, retained_dependency_data[i], origin,
          report_published);
    }

    pop_package();
    if (rejected) {
      return False;
    }

    CompletedPackage completed = {
      .identity = restored_identity,
      .version = archive.get_version(),
      .value = restored_package,
    };
    completed_packages.insert(completed);

    Bool bound = owner.bind_dependency(dependency, restored_package);
    if (!bound) {
      publish_dependency_failure(
          origin, report_published,
          "the restored Package root rejected its exact Alias binding"_view);
    }
    return bound;
  }

  // One transaction Arena serves every traversal vector. Growth copies only
  // passive records and releases the complete page chain when resolution ends.
  Allocator::Arena temporary;
  Allocator::Arena& domain;
  Environment::Dialects& dialects;
  Environment::Retention& retention;
  Package::Repository::Repository& repository;
  Errors& errors;
  SelectRestored select_restored;
  ReserveRestored reserve_restored;
  Managed::Vector<PackageKey> active_packages;
  Managed::Vector<DependencyHop> active_hops;
  Managed::Vector<CompletedPackage> completed_packages;
  Count active_package_count;
  Count active_hop_count;
  Option<Package::Repository::SelectionError> rejection;
};

Environment::Resolution::Resolution(
    Allocator::Arena& arena,
    Dialects& dialects,
    Retention& retention)
    : arena(arena),
      dialects(dialects),
      retention(retention),
      restored_packages(arena) {}

auto Environment::Resolution::resolve(
    Errors& errors,
    Count first_monograph,
    View::Bytes root_package_identity,
    Version root_package_version,
    Package::Language::Monograph& root_package,
    Package::Repository::Repository& repository) -> ResolutionResult {
  const Count first_restored_package = restored_packages.get_size();
  auto select_restored = [&](View::Bytes identity,
                             Version version) -> CacheSelection {
    Bool attempted_exact_key = False;
    for (Count i = 0; i < restored_packages.get_size(); i++) {
      RestoredPackage& retained = restored_packages[i];
      if (retained.identity != identity) {
        continue;
      }

      if (retained.version != version) {
        return CacheMiss::VersionConflict;
      }

      if (!retained.ready) {
        attempted_exact_key = True;
        continue;
      }

      return retained.value;
    }

    return attempted_exact_key ? CacheMiss::Incomplete : CacheMiss::Missing;
  };
  auto reserve_restored = [&](View::Bytes identity, Version version,
                              Package::Language::Monograph& value) {
    RestoredPackage retained = {
      .identity = identity,
      .version = version,
      .value = value,
      .ready = False,
    };
    restored_packages.insert(retained);
  };
  ResolutionState state(
      errors, arena, dialects, retention, repository, select_restored,
      reserve_restored);
  TransactionResult restoration = state.restore(
      first_monograph, root_package_identity, root_package_version,
      root_package);
  Bool completed = retention.complete(errors);

  return restoration.visit(
      [&](Package::Language::Monograph& restored) -> ResolutionResult {
        if (!completed) {
          return Package::Repository::SelectionError::Unknown;
        }

        // Restore hooks reserve failed keys before mutating concrete graph
        // state. Promotion begins at this call boundary so an older partial
        // attempt cannot become reusable after an independent import succeeds.
        for (Count i = first_restored_package; i < restored_packages.get_size();
             i++) {
          restored_packages[i].ready = True;
        }

        RestoredPackage retained_root = {
          .identity = root_package_identity,
          .version = root_package_version,
          .value = restored,
          .ready = True,
        };
        restored_packages.insert(retained_root);
        return restored;
      },
      [](Package::Repository::SelectionError error) -> ResolutionResult {
        return error;
      });
}
