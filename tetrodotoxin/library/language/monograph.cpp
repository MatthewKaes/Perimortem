// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/monograph.hpp"

#include "perimortem/core/diagnostics/log.hpp"

#include "tetrodotoxin/package/language/monograph.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Ttx::Concept;
using namespace Ttx::Model;
using namespace Tetrodotoxin;

static constexpr View::Bytes import_operation =
    "Library::Language::Monograph Import post pass"_view;

struct ImportCandidate {
  View::Bytes import_route;
  View::Bytes provider_member;
  Reference<Library::Language::Function> function;
};

template <typename selected_type>
static auto select_abstract(const Abstract& value)
    -> Perimortem::Utility::Option<const selected_type&> {
  return value.visit<selected_type>(
      [](const selected_type& selected)
          -> Perimortem::Utility::Option<const selected_type&> {
        return selected;
      },
      [](const Abstract&) -> Perimortem::Utility::Option<const selected_type&> {
        return {};
      });
}

static auto visibility_text(Library::Language::Visibility visibility)
    -> View::Bytes {
  switch (visibility) {
  case Library::Language::Visibility::Public:
    return "public"_view;
  case Library::Language::Visibility::Private:
    return "private"_view;
  }

  return "unknown"_view;
}

Library::Language::Monograph::Monograph(
    Allocator::Arena& domain,
    const Documentation& documentation,
    Library::Dialect& host,
    const Abstract& interpretation_context)
    : Tetrodotoxin::Language::Dialect::Monograph(domain, documentation, host),
      library_host(host),
      interpretation_context(interpretation_context),
      imports(domain),
      functions(domain),
      public_functions(domain) {}

auto Library::Language::Monograph::bind_function(Function& function) -> Bool {
  const View::Bytes name = function.get_name();

  // Both mutations happen after the exact duplicate check. A rejected
  // declaration therefore preserves the first edge and its public position
  // while the failed interpretation discards the unreachable transaction.
  if (name.is_empty() || functions.contains(name)) {
    return False;
  }

  functions.launder(name, Reference<Function>(function));
  if (function.get_visibility() == Visibility::Public) {
    public_functions.insert(function);
  }

  return True;
}

auto Library::Language::Monograph::retain_import(const Import& import) -> void {
  imports.insert(import);
}

auto Library::Language::Monograph::post_pass() -> Bool {
  if (imports.is_empty()) {
    return True;
  }

  const Abstract& source_context = interpretation_context.resolve();
  auto source_package =
      select_abstract<Package::Language::Monograph>(source_context);
  if (!source_package) {
    for (Count i = 0; i < imports.get_size(); i++) {
      Diagnostics::Log::Message<768> message(Diagnostics::Log::Level::Info);
      message << import_operation
              << " failed. reason=the source context is not a Package "
                 "Monograph import_route="_view
              << imports[i].get_route() << " source_context="_view
              << source_context.get_name();
    }

    return False;
  }

  Managed::Vector<ImportCandidate> candidates(domain);
  Bool failed = False;

  // Import order leads Package member order and provider publication order.
  // Staging that complete sequence first keeps visible lookup untouched while
  // later routes can still expose independent graph failures.
  for (Count import_index = 0; import_index < imports.get_size();
       import_index++) {
    const Import& import = imports[import_index];
    View::Bytes route = import.get_route();
    Bool duplicate = False;
    for (Count earlier = 0; earlier < import_index; earlier++) {
      duplicate |= imports[earlier].get_route() == route;
    }

    if (duplicate) {
      Diagnostics::Log::Message<768> message(Diagnostics::Log::Level::Info);
      message << import_operation
              << " failed. reason=duplicate Import route import_route="_view
              << route << " source_package="_view << source_package->get_name();
      failed = True;
      continue;
    }

    const Abstract& selected = source_package->resolve_context(route).resolve();
    auto target_package =
        select_abstract<Package::Language::Monograph>(selected);
    if (!target_package) {
      Diagnostics::Log::Message<896> message(Diagnostics::Log::Level::Info);
      message << import_operation
              << " failed. reason=the Import target is not a Package "
                 "Monograph import_route="_view
              << route << " source_package="_view << source_package->get_name()
              << " selected_target="_view << selected.get_name();
      failed = True;
      continue;
    }

    View::Vector<Reference<Alias>> members = target_package->get_members();
    for (Count member_index = 0; member_index < members.get_size();
         member_index++) {
      const Alias& member = members.get_data()[member_index].get();
      const Abstract& provider = member.resolve();
      auto library = select_abstract<Library::Language::Monograph>(provider);
      if (!library) {
        continue;
      }

      View::Vector<Reference<Function>> public_functions =
          library->get_public_functions();
      for (Count function_index = 0;
           function_index < public_functions.get_size(); function_index++) {
        const Function& function =
            public_functions.get_data()[function_index].get();
        if (!function.is_complete()) {
          Diagnostics::Log::Message<1024> message(
              Diagnostics::Log::Level::Info);
          message << import_operation
                  << " failed. reason=the provider Function is incomplete "
                     "import_route="_view
                  << route << " provider_member="_view << member.get_name()
                  << " candidate_function="_view << function.get_name()
                  << " candidate_visibility="_view
                  << visibility_text(function.get_visibility());
          failed = True;
          continue;
        }

        ImportCandidate candidate = {
          .import_route = route,
          .provider_member = member.get_name(),
          .function = function,
        };
        candidates.insert(candidate);
      }
    }
  }

  // Local declarations and every earlier candidate participate in one exact
  // collision domain. The complete scan runs even after a failure so each
  // rejected edge keeps the provider facts that Retention cannot reconstruct.
  for (Count candidate_index = 0; candidate_index < candidates.get_size();
       candidate_index++) {
    const ImportCandidate& candidate = candidates[candidate_index];
    const Function& function = candidate.function.get();
    View::Bytes name = function.get_name();
    auto local_entry = functions.find(name);
    if (local_entry) {
      const Function& local = local_entry->value.get();
      Diagnostics::Log::Message<1152> message(Diagnostics::Log::Level::Info);
      message << import_operation
              << " failed. reason=an imported Function collides with a local "
                 "Function import_route="_view
              << candidate.import_route << " provider_member="_view
              << candidate.provider_member << " candidate_function="_view
              << name << " candidate_visibility="_view
              << visibility_text(function.get_visibility())
              << " conflicting_provider=local conflicting_function="_view
              << local.get_name() << " conflicting_visibility="_view
              << visibility_text(local.get_visibility());
      failed = True;
    }

    for (Count earlier = 0; earlier < candidate_index; earlier++) {
      const ImportCandidate& conflict = candidates[earlier];
      const Function& conflicting_function = conflict.function.get();
      if (conflicting_function.get_name() != name) {
        continue;
      }

      Diagnostics::Log::Message<1280> message(Diagnostics::Log::Level::Info);
      message << import_operation
              << " failed. reason=two imported Functions collide "
                 "import_route="_view
              << candidate.import_route << " provider_member="_view
              << candidate.provider_member << " candidate_function="_view
              << name << " candidate_visibility="_view
              << visibility_text(function.get_visibility())
              << " conflicting_import_route="_view << conflict.import_route
              << " conflicting_provider_member="_view
              << conflict.provider_member << " conflicting_function="_view
              << conflicting_function.get_name()
              << " conflicting_visibility="_view
              << visibility_text(conflicting_function.get_visibility());
      failed = True;
    }
  }

  if (failed) {
    return False;
  }

  for (Count i = 0; i < candidates.get_size(); i++) {
    const Function& function = candidates[i].function.get();
    functions.launder(function.get_name(), candidates[i].function);
  }

  return True;
}

auto Library::Language::Monograph::get_name() const -> View::Bytes {
  return "Library"_view;
}

auto Library::Language::Monograph::resolve_context(View::Bytes route) const
    -> const Abstract& {
  return functions.visit(
      route,
      [](const Reference<Function>& function) -> const Abstract& {
        return function.get();
      },
      [&]() -> const Abstract& {
        return library_host.resolve_intrinsic(route);
      });
}

auto Library::Language::Monograph::get_public_functions() const
    -> View::Vector<Reference<Function>> {
  return public_functions;
}
