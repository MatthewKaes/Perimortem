// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/monograph.hpp"

#include "tetrodotoxin/library/language/function.hpp"
#include "tetrodotoxin/library/language/types/source.hpp"
#include "tetrodotoxin/package/language/monograph.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/model/addressable.hpp"
#include "ttx/model/alias.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Utility;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Ttx::Model;
using namespace Tetrodotoxin;

struct ImportCandidate {
  Ttx::Lexical::Span import_span;
  Reference<const Abstract> binding;
};

static auto get_binding_target(const Abstract& binding) -> const Abstract& {
  return binding.visit<Alias>(
      [](const Alias& alias) -> const Abstract& {
        return get_binding_target(alias.get_target());
      },
      [](const Abstract& direct) -> const Abstract& { return direct; });
}

static auto bindings_collide(const Abstract& first, const Abstract& second)
    -> Bool {
  const Abstract& first_target = get_binding_target(first);
  const Abstract& second_target = get_binding_target(second);
  return Bool(
      (first_target.is<Type>() && second_target.is<Type>()) ||
      (first_target.is<Addressable>() && second_target.is<Addressable>()));
}

Library::Language::Monograph::Monograph(
    Allocator::Arena& domain,
    const Documentation& documentation,
    Library::Dialect& host,
    const Abstract& interpretation_context,
    Materializations& materializations)
    : Tetrodotoxin::Language::Monograph(domain, documentation),
      library_host(host),
      interpretation_context(interpretation_context),
      materializations(materializations),
      imports(domain),
      imported_providers(domain),
      authored_bindings(domain),
      authored_binding_observations(domain) {}

auto Library::Language::Monograph::create_authored(
    Allocator::Arena& domain,
    const Documentation& documentation,
    Library::Dialect& host,
    const Abstract& interpretation_context,
    Materializations& materializations) -> Monograph& {
  Monograph& monograph = domain.construct_from<Monograph>([&]() -> Monograph {
    return Monograph(
        domain, documentation, host, interpretation_context, materializations);
  });
  auto& source = Library::Language::Types::Source::create_synthetic(
      domain, documentation, monograph, materializations);
  monograph.source_type = Reference<Type>(source);
  return monograph;
}

auto Library::Language::Monograph::bind_static(
    Abstract& binding,
    Visibility visibility) -> Bool {
  auto source = get_source().select<Types::Source>();
  BAIL_IF(!source || !source->bind_static(binding, visibility));

  authored_bindings.insert(binding);
  authored_binding_observations.insert(binding);
  return True;
}

auto Library::Language::Monograph::retain_import(const Import& import) -> Bool {
  BAIL_IF(imports_linked);

  imports.insert(import);
  return True;
}

auto Library::Language::Monograph::link_imports() -> Bool {
  if (imports.is_empty()) {
    imports_linked = True;
    return True;
  }

  const Abstract& source_context = interpretation_context.resolve();
  auto source_package = source_context.select<Package::Language::Monograph>();
  if (!source_package) {
    for (Count i = 0; i < imports.get_size(); i++) {
      report(
          Ttx::Lexical::Anchor::create(imports[i].get_span()),
          "Library Import source context is not a Package Monograph."_view,
          "Interpret this Library source inside its owning Package."_view);
    }

    return False;
  }

  auto source_type = get_source().select<Types::Source>();
  BAIL_IF(!source_type);

  Managed::Vector<ImportCandidate> candidates(domain);
  Managed::Vector<Reference<Monograph>> providers(domain);
  Managed::Vector<Reference<const Types::Source>> pending_field_providers(
      domain);
  Bool failed = False;

  // Package order leads member order and each provider source order. Staging
  // the complete sequence keeps the importing Source unchanged until every
  // contextual source and collision has been observed.
  for (Count import_index = 0; import_index < imports.get_size();
       import_index++) {
    const Import& import = imports[import_index];
    View::Bytes route = import.get_route();
    Bool duplicate = imports.get_view()
                         .slice(0, import_index)
                         .contains([&](const Import& earlier) {
                           return earlier.get_route() == route;
                         });

    if (duplicate) {
      report(
          Ttx::Lexical::Anchor::create(import.get_span()),
          "Library source repeats one exact Import route."_view,
          "Keep one authored Import for each Package member route."_view);
      failed = True;
      continue;
    }

    const Abstract& selected =
        import.get_type_access().resolve(*source_package);
    auto target_package = selected.select<Package::Language::Monograph>();
    if (!target_package) {
      report(
          Ttx::Lexical::Anchor::create(import.get_span()),
          "Library Import route did not resolve to a Package Monograph."_view,
          "Publish the selected dependency Package before linking."_view);
      failed = True;
      continue;
    }

    View::Vector<Reference<const Alias>> members =
        target_package->get_members();

    // A Package member Alias only selects its target. Prove the resolved
    // Library Monograph before consulting that owner's source so an unrelated
    // Monograph cannot borrow a Library Source to enter expansion.
    for (Count member_index = 0; member_index < members.get_size();
         member_index++) {
      const Alias& member = members.get_data()[member_index].get();
      const Abstract& member_target = member.resolve();
      auto provider_monograph = member_target.select<Monograph>();
      if (!provider_monograph) {
        const Abstract& claimed_source =
            member.resolve_context("source"_view).resolve();
        auto spoofed_source = claimed_source.select<Types::Source>();
        if (spoofed_source) {
          report(
              Ttx::Lexical::Anchor::create(import.get_span()),
              "Non Library Package member claimed a Library Source Type."_view,
              "Bind the member to its exact Library Monograph before "
              "importing."_view);
          failed = True;
        }

        continue;
      }

      // Package lookup exposes a read only graph edge, while Library owns the
      // lifecycle of the exact Monograph proven above. Retaining that identity
      // here lets closure linking advance the real owner without widening the
      // Package contract into a mutable semantic route.
      Monograph& mutable_provider = const_cast<Monograph&>(*provider_monograph);
      Bool known_provider =
          providers.get_view().contains([&](const Reference<Monograph>& known) {
            return &known.get() == &mutable_provider;
          });
      if (!known_provider) {
        providers.insert(mutable_provider);
      }

      const Type& provider_source = provider_monograph->get_source();
      auto provider = provider_source.select<Types::Source>();
      if (!provider) {
        report(
            Ttx::Lexical::Anchor::create(import.get_span()),
            "Library Package member has no exact Source Type."_view,
            "Complete the provider Monograph before importing its Static "
            "bindings."_view);
        failed = True;
        continue;
      }

      if (!provider->is_linked()) {
        // Provider Fields already have their final identities during closure
        // discovery even though their Types remain incomplete. Those retained
        // objects prove the complete name set before either source publishes an
        // Addressable, keeping a later collision transactional.
        auto provider_fields = provider->get_fields();
        auto local_fields = source_type->get_fields();
        for (Count field_index = 0; field_index < provider_fields.get_size();
             field_index++) {
          const Field& field = provider_fields.get_data()[field_index].get();
          if (field.get_exposure() == Field::Exposure::Private) {
            continue;
          }

          Bool collision =
              local_fields.contains([&](const Reference<const Field>& local) {
                return local.get().get_name() == field.get_name();
              }) ||
              candidates.get_view().contains(
                  [&](const ImportCandidate& candidate) {
                    const Abstract& earlier =
                        get_binding_target(candidate.binding.get());
                    return earlier.is<Addressable>() &&
                           earlier.get_name() == field.get_name();
                  }) ||
              pending_field_providers.get_view().contains(
                  [&](const Reference<const Types::Source>& earlier_provider) {
                    return earlier_provider.get().get_fields().contains(
                        [&](const Reference<const Field>& earlier) {
                          return earlier.get().get_exposure() !=
                                     Field::Exposure::Private &&
                                 earlier.get().get_name() == field.get_name();
                        });
                  });

          if (collision) {
            report(
                Ttx::Lexical::Anchor::create(import.get_span()),
                "Imported source Field collides with an occupied source "
                "Addressable name."_view,
                "Rename the local Field or select another dependency."_view);
            failed = True;
          }
        }

        pending_field_providers.insert(*provider);
      }

      auto bindings = provider->get_external_static_bindings();
      for (Count binding_index = 0; binding_index < bindings.get_size();
           binding_index++) {
        const Abstract& binding = bindings.get_data()[binding_index].get();
        if (source_type->get_static_bindings().contains(
                [&](const Reference<const Abstract>& existing) {
                  return existing.get().visit<Alias>(
                      [&](const Alias& alias) {
                        return Bool(&alias.get_target() == &binding);
                      },
                      [](const Abstract&) { return False; });
                })) {
          continue;
        }

        const Abstract& target = get_binding_target(binding);
        if (target.is<Addressable>()) {
          for (Count pending = 0; pending < pending_field_providers.get_size();
               pending++) {
            auto pending_fields =
                pending_field_providers[pending].get().get_fields();
            for (Count field_index = 0; field_index < pending_fields.get_size();
                 field_index++) {
              const Field& field = pending_fields.get_data()[field_index].get();
              if (field.get_exposure() == Field::Exposure::Private ||
                  field.get_name() != target.get_name()) {
                continue;
              }

              report(
                  Ttx::Lexical::Anchor::create(import.get_span()),
                  "Imported source Field collides with another imported "
                  "Addressable name."_view,
                  "Select dependencies with distinct public Fields."_view);
              failed = True;
            }
          }
        }

        Bool incomplete = binding.visit<Function>(
            [](const Function& function) { return !function.is_complete(); },
            [](const Abstract&) { return False; });
        if (incomplete) {
          report(
              Ttx::Lexical::Anchor::create(import.get_span()),
              "Library Import exposes an incomplete Static binding."_view,
              "Complete provider grammar before linking imports."_view);
          failed = True;
          continue;
        }

        candidates.insert(
            ImportCandidate{
              .import_span = import.get_span(),
              .binding = binding,
            });
      }
    }
  }

  Managed::Vector<Reference<Alias>> aliases(domain);
  for (Count i = 0; i < candidates.get_size(); i++) {
    const Abstract& target = candidates[i].binding.get();
    Alias& alias = domain.construct<Alias>(target.get_name(), target);
    aliases.insert(alias);
  }

  // The importer owned Alias is the identity that enters source lookup. Arena
  // construction publishes nothing, so the complete candidate set can prove
  // host and collision policy before one Alias becomes observable.
  for (Count candidate_index = 0; candidate_index < candidates.get_size();
       candidate_index++) {
    const ImportCandidate& candidate = candidates[candidate_index];
    const Abstract& binding = candidate.binding.get();
    const Alias& alias = aliases[candidate_index].get();
    View::Bytes name = binding.get_name();
    if (!source_type->can_bind_static(alias)) {
      report(
          Ttx::Lexical::Anchor::create(candidate.import_span),
          "Imported Static binding collides with an occupied source name."_view,
          "Rename the local declaration or select another dependency."_view);
      failed = True;
    }

    for (Count earlier = 0; earlier < candidate_index; earlier++) {
      const Abstract& previous = candidates[earlier].binding.get();
      if (previous.get_name() != name || !bindings_collide(previous, binding)) {
        continue;
      }

      report(
          Ttx::Lexical::Anchor::create(candidate.import_span),
          "Two Library Imports publish the same Static binding name."_view,
          "Import a dependency set with distinct exposed names."_view);
      failed = True;
    }
  }

  BAIL_IF(failed);

  // The earlier complete preflight makes each bind infallible within this
  // Monograph transaction, so no candidate becomes visible beside a later
  // rejection.
  for (Count i = 0; i < aliases.get_size(); i++) {
    BAIL_IF(!source_type->bind_static(aliases[i].get(), Visibility::Private));
  }

  for (Count i = 0; i < providers.get_size(); i++) {
    Bool retained = imported_providers.get_view().contains(
        [&](const Reference<Monograph>& existing) {
          return &existing.get() == &providers[i].get();
        });
    if (!retained) {
      imported_providers.insert(providers[i]);
    }
  }

  imports_linked = True;
  return True;
}

auto Library::Language::Monograph::link_declaration_types() -> Bool {
  return get_source().visit<Types::Source>(
      [](Types::Source& source) { return source.link_types(); },
      [](Abstract&) { return False; });
}

auto Library::Language::Monograph::link_fields() -> Bool {
  return get_source().visit<Types::Source>(
      [](Types::Source& source) { return source.link_fields(); },
      [](Abstract&) { return False; });
}

auto Library::Language::Monograph::link_initializers() -> Bool {
  return get_source().visit<Types::Source>(
      [](Types::Source& source) { return source.link_initializers(); },
      [](Abstract&) { return False; });
}

auto Library::Language::Monograph::link_callable_signatures() -> Bool {
  return get_source().visit<Types::Source>(
      [](Types::Source& source) { return source.link_callable_signatures(); },
      [](Abstract&) { return False; });
}

auto Library::Language::Monograph::link_callable_bodies() -> Bool {
  return get_source().visit<Types::Source>(
      [](Types::Source& source) { return source.link_callable_bodies(); },
      [](Abstract&) { return False; });
}

auto Library::Language::Monograph::link() -> Bool {
  Managed::Vector<Reference<Monograph>> closure(domain);
  closure.insert(*this);

  // Every authenticated provider enters the queue in authored discovery order.
  // Scanning exact identities before insertion lets cycles terminate while the
  // growing queue still expands every provider Import before semantics begin.
  for (Count monograph_index = 0; monograph_index < closure.get_size();
       monograph_index++) {
    Monograph& monograph = closure[monograph_index].get();
    BAIL_IF(!monograph.link_imports());

    for (Count provider_index = 0;
         provider_index < monograph.imported_providers.get_size();
         provider_index++) {
      Monograph& provider = monograph.imported_providers[provider_index].get();
      Bool discovered =
          closure.get_view().contains([&](const Reference<Monograph>& earlier) {
            return &earlier.get() == &provider;
          });
      if (!discovered) {
        closure.insert(provider);
      }
    }
  }

  // Each complete closure phase settles before the next one starts. Recursive
  // Type linking reaches nested Enumeration storage first, so discovery order
  // cannot decide whether a consumer sees provider Types, Fields, initializers,
  // or Callable signatures before its own body begins.
  Bool failed = False;
  for (Count i = 0; i < closure.get_size(); i++) {
    failed |= !closure[i].get().link_declaration_types();
  }
  BAIL_IF(failed);

  for (Count i = 0; i < closure.get_size(); i++) {
    failed |= !closure[i].get().link_fields();
  }
  BAIL_IF(failed);

  // Provider Fields become exact only after every imported Type route is
  // available. Replaying the import transaction here adds those new exact
  // Addressables before any initializer or Callable may consume them.
  for (Count i = 0; i < closure.get_size(); i++) {
    failed |= !closure[i].get().link_imports();
  }
  BAIL_IF(failed);

  for (Count i = 0; i < closure.get_size(); i++) {
    failed |= !closure[i].get().link_initializers();
  }
  BAIL_IF(failed);

  for (Count i = 0; i < closure.get_size(); i++) {
    failed |= !closure[i].get().link_callable_signatures();
  }
  BAIL_IF(failed);

  for (Count i = 0; i < closure.get_size(); i++) {
    failed |= !closure[i].get().link_callable_bodies();
  }

  return !failed;
}

auto Library::Language::Monograph::finalize() -> Bool {
  // Source is the one root of the declaration tree, so its recursive
  // finalization reaches the same identities that linking prepared without a
  // second Monograph inventory walk.
  return get_source().visit<Types::Source>(
      [](Types::Source& source) { return source.finalize(); },
      [](Abstract&) { return False; });
}

auto Library::Language::Monograph::get_name() const -> View::Bytes {
  return "Library"_view;
}

auto Library::Language::Monograph::resolve_context(View::Bytes route) const
    -> const Abstract& {
  const Type& source = get_source();
  if (route == "source"_view) {
    return source;
  }

  return source.visit<Types::Source>(
      [&](const Types::Source& source_root) -> const Abstract& {
        return source_root.resolve_context(route);
      },
      [](const Abstract&) -> const Abstract& {
        return Invalid::get_invalid();
      });
}

auto Library::Language::Monograph::get_source() -> Type& {
  return source_type->get();
}

auto Library::Language::Monograph::get_source() const -> const Type& {
  return source_type->get();
}

auto Library::Language::Monograph::get_authored_bindings() const
    -> View::Vector<Reference<const Abstract>> {
  return authored_binding_observations;
}

auto Library::Language::Monograph::get_imports() const -> View::Vector<Import> {
  return imports;
}
