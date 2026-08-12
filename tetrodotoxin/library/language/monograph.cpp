// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/monograph.hpp"

#include "tetrodotoxin/library/language/function.hpp"
#include "tetrodotoxin/package/language/monograph.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/model/alias.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Utility;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin;

using Tetrodotoxin::Language::Visibility;
using Ttx::Model::Alias;

struct ImportCandidate {
  Ttx::Lexical::Span import_span;
  Reference<const Abstract> binding;
  Library::Language::Types::Composite::Category category;
};

static auto candidates_collide(
    const ImportCandidate& first,
    const ImportCandidate& second) -> Bool {
  return first.category == second.category &&
         first.binding.get().get_name() == second.binding.get().get_name();
}

static auto is_published(const Abstract& binding) -> Bool {
  auto authorship = binding.get_authorship();
  return authorship && authorship->is_published();
}

Library::Language::Monograph::Monograph(
    Allocator::Arena& domain,
    const Documentation& documentation,
    const Anchor& source_anchor,
    Library::Dialect& dialect,
    const Abstract& interpretation_context,
    Materializations& materializations)
    : Tetrodotoxin::Language::Monograph(domain, documentation),
      dialect(dialect),
      interpretation_context(interpretation_context),
      materializations(materializations),
      imports(domain),
      imported_providers(domain),
      source(
          Types::Source::create_synthetic(
              domain,
              documentation,
              *this,
              source_anchor)) {}

auto Library::Language::Monograph::create_authored(
    Allocator::Arena& domain,
    const Documentation& documentation,
    const Anchor& source_anchor,
    Library::Dialect& dialect,
    const Abstract& interpretation_context,
    Materializations& materializations) -> Monograph& {
  return domain.construct_from<Monograph>([&]() -> Monograph {
    return Monograph(
        domain, documentation, source_anchor, dialect, interpretation_context,
        materializations);
  });
}

auto Library::Language::Monograph::retain_import(const Import& import) -> Bool {
  BAIL_IF(declarations_imported);

  imports.insert(import);
  return True;
}

auto Library::Language::Monograph::link_imports() -> Bool {
  Bool declaration_phase = !declarations_imported;
  if (!declaration_phase && addressables_imported) {
    return True;
  }

  if (imports.is_empty()) {
    declarations_imported = True;
    addressables_imported = True;
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

  Types::Source& source = get_source();

  Managed::Vector<ImportCandidate> candidates(domain);
  Managed::Vector<Reference<Monograph>> providers(domain);
  Bool failed = False;

  auto retain_candidates = [&](const auto& bindings,
                               Types::Composite::Category category,
                               Span import_span, auto& destination) {
    for (const Reference<Abstract>& selected : bindings) {
      const Abstract& binding = selected.get();
      if (category == Types::Composite::Category::Callable) {
        auto function = binding.select<Function>();
        auto signature =
            function ? function->get_signature() : Option<const Signature&>();
        Bool invalid_callable = !function || !function->is_complete() ||
                                !signature || signature->declares_self();
        if (invalid_callable) {
          report(
              Anchor::create(import_span),
              "Library Import exposes an invalid top level Callable."_view,
              "Complete one Static Function before linking imports."_view);
          failed = True;
          continue;
        }
      }

      destination.insert(
          ImportCandidate{
            .import_span = import_span,
            .binding = binding,
            .category = category,
          });
    }
  };

  // Package order leads member order and each provider source order. Staging
  // the complete sequence keeps the importing Source unchanged until every
  // contextual source and collision has been observed.
  for (Count import_index = 0; import_index < imports.get_size();
       import_index++) {
    const Import& import = imports[import_index];
    Bool duplicate = imports.get_view()
                         .slice(0, import_index)
                         .contains([&](const Import& earlier) -> Bool {
                           return earlier.matches(import);
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
        import.get_type_reference().resolve(*source_package);
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

    // A Package member Alias is opaque except for resolution. Only an exact
    // Library Monograph result grants access to its Source; unrelated member
    // contexts are never probed for a source-shaped route.
    for (Count member_index = 0; member_index < members.get_size();
         member_index++) {
      const Alias& member = members.get_data()[member_index].get();
      const Abstract& member_target = member.resolve();
      auto provider_monograph = member_target.select<Monograph>();
      if (!provider_monograph) {
        continue;
      }

      if (declaration_phase) {
        // Package lookup exposes a read only graph edge, while Library owns the
        // lifecycle of the exact Monograph proven above. Retaining that
        // identity here lets closure linking advance the real owner without
        // widening the Package contract into a mutable semantic route.
        Monograph& mutable_provider =
            const_cast<Monograph&>(*provider_monograph);
        Bool known_provider = providers.get_view().contains(
            [&](const Reference<Monograph>& known) -> Bool {
              return Bool(&known.get() == &mutable_provider);
            });
        if (!known_provider) {
          providers.insert(mutable_provider);
        }
      }

      const Types::Source& provider = provider_monograph->get_source();
      if (declaration_phase) {
        retain_candidates(
            provider.get_callables(Visibility::Public),
            Types::Composite::Category::Callable, import.get_span(),
            candidates);
        retain_candidates(
            provider.get_types(Visibility::Public),
            Types::Composite::Category::Type, import.get_span(), candidates);

        // Provider Addressables keep their final identities while their Types
        // are still settling. Their authorship proves the complete future
        // public namespace now, but publication waits for the second phase.
        for (const Reference<Abstract>& selected :
             provider.get_addressables()) {
          if (!is_published(selected.get())) {
            continue;
          }

          candidates.insert(
              ImportCandidate{
                .import_span = import.get_span(),
                .binding = selected.get(),
                .category = Types::Composite::Category::Addressable,
              });
        }
      } else {
        retain_candidates(
            provider.get_addressables(Visibility::Public),
            Types::Composite::Category::Addressable, import.get_span(),
            candidates);
      }
    }
  }

  auto preflight = [&](auto& staged) {
    auto staged_view = staged.get_view();
    for (Count candidate_index = 0; candidate_index < staged_view.get_size();
         candidate_index++) {
      const ImportCandidate& candidate =
          staged_view.get_data()[candidate_index];
      if (!source.can_bind_static(
              candidate.binding.get(), candidate.category)) {
        report(
            Anchor::create(candidate.import_span),
            "Imported Static binding collides with its source category."_view,
            "Rename the local declaration or select another dependency."_view);
        failed = True;
      }

      for (Count earlier = 0; earlier < candidate_index; earlier++) {
        if (!candidates_collide(staged_view.get_data()[earlier], candidate)) {
          continue;
        }

        report(
            Anchor::create(candidate.import_span),
            "Two Library Imports publish one name in the same category."_view,
            "Import dependencies with distinct names in that category."_view);
        failed = True;
      }
    }
  };

  preflight(candidates);

  BAIL_IF(failed);

  // The complete phase preflight makes these category-directed binds
  // infallible. Alias construction therefore publishes directly without a
  // second staged identity collection.
  for (Count i = 0; i < candidates.get_size(); i++) {
    if (declaration_phase &&
        candidates[i].category == Types::Composite::Category::Addressable) {
      continue;
    }

    const Abstract& target = candidates[i].binding.get();
    Alias& alias = domain.construct<Alias>(target.get_name(), target);
    if (!source.bind_static(alias, candidates[i].category)) {
      report(
          Anchor::create(candidates[i].import_span),
          "A preflighted Library Import could not enter its source."_view,
          "Keep import publication within one source lifecycle phase."_view);
      return False;
    }
  }

  if (declaration_phase) {
    for (Count i = 0; i < providers.get_size(); i++) {
      imported_providers.insert(providers[i]);
    }

    declarations_imported = True;
  } else {
    addressables_imported = True;
  }

  return True;
}

auto Library::Language::Monograph::link() -> Bool {
  Managed::Vector<Reference<Monograph>> active_dependencies(domain);
  Managed::Vector<Reference<Monograph>> dependency_order(domain);
  auto contains = [](const auto& monographs, const Monograph& target) -> Bool {
    return monographs.get_view().contains(
        [&](const Reference<Monograph>& retained) -> Bool {
          return Bool(&retained.get() == &target);
        });
  };
  auto retain_provider_first = [&](auto& retain, Monograph& monograph) -> Bool {
    if (contains(dependency_order, monograph)) {
      return True;
    }
    if (contains(active_dependencies, monograph)) {
      // Import cycles have no strict dependency order. Keeping the first
      // authored edge as the break point still lets direct declarations settle;
      // an actual Alias cycle is rejected by the Alias owner during Type link.
      return True;
    }

    active_dependencies.insert(monograph);
    BAIL_IF(!monograph.link_imports());
    for (const Reference<Monograph>& provider :
         monograph.imported_providers.get_view()) {
      BAIL_IF(!retain(retain, provider.get()));
    }
    dependency_order.insert(monograph);
    return True;
  };
  BAIL_IF(!retain_provider_first(retain_provider_first, *this));

  // The same walk discovers Imports and retains authored provider order. Each
  // semantic phase then completes providers before the consumers whose opaque
  // import Aliases name them, while declarations inside each Monograph preserve
  // their own source order.
  auto complete_phase = [&](auto complete) -> Bool {
    Bool failed = False;
    for (const Reference<Monograph>& monograph : dependency_order.get_view()) {
      failed |= !complete(monograph.get());
    }
    return !failed;
  };

  BAIL_IF(!complete_phase([](Monograph& monograph) {
    return monograph.get_source().link_types();
  }));

  // A Field initializer may invoke a later-authored Callable. Signatures depend
  // only on completed declaration Types, so the complete closure settles them
  // before any Field Expression attempts selection or argument fitting.
  BAIL_IF(!complete_phase([](Monograph& monograph) {
    return monograph.get_source().link_callable_signatures();
  }));

  BAIL_IF(!complete_phase([](Monograph& monograph) {
    return monograph.get_source().link_fields();
  }));

  // Provider Fields become exact only after every imported Type route is
  // available. Replaying the import transaction here adds those new exact
  // Addressables before any remaining initializer or Callable body may consume
  // them.
  BAIL_IF(!complete_phase(
      [](Monograph& monograph) { return monograph.link_imports(); }));

  BAIL_IF(!complete_phase([](Monograph& monograph) {
    return monograph.get_source().link_initializers();
  }));

  return complete_phase([](Monograph& monograph) {
    return monograph.get_source().link_callable_bodies();
  });
}

auto Library::Language::Monograph::finalize() -> Bool {
  // Source is the one root of the declaration tree, so its recursive
  // finalization reaches the same identities that linking prepared without a
  // second Monograph inventory walk.
  return get_source().finalize();
}

auto Library::Language::Monograph::get_name() const -> View::Bytes {
  return "Library"_view;
}

auto Library::Language::Monograph::resolve_context(View::Bytes route) const
    -> const Abstract& {
  const Types::Source& source = get_source();
  if (route == "source"_view) {
    return source;
  }

  return source.resolve_context(route);
}
