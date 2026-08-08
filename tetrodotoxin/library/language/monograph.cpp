// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/monograph.hpp"

#include "tetrodotoxin/library/language/function.hpp"
#include "tetrodotoxin/library/language/types/enumeration.hpp"
#include "tetrodotoxin/library/language/types/structure.hpp"
#include "tetrodotoxin/package/language/monograph.hpp"
#include "ttx/concept/invalid.hpp"
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

template <typename selected_type>
static auto select_abstract(const Abstract& value)
    -> Option<const selected_type&> {
  return value.visit<selected_type>(
      [](const selected_type& selected) -> Option<const selected_type&> {
        return selected;
      },
      [](const Abstract&) -> Option<const selected_type&> { return {}; });
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
  auto& source = Library::Language::Types::Structure::create_synthetic(
      domain, documentation, monograph, materializations);
  monograph.source_structure = Reference<Type>(source);
  return monograph;
}

auto Library::Language::Monograph::bind_static(
    Abstract& binding,
    Visibility visibility) -> Bool {
  auto source = get_source().visit<Types::Structure>(
      [](Types::Structure& structure) -> Option<Types::Structure&> {
        return structure;
      },
      [](Abstract&) -> Option<Types::Structure&> { return {}; });
  if (!source || !source->bind_static(binding, visibility)) {
    return False;
  }

  authored_bindings.insert(binding);
  authored_binding_observations.insert(binding);
  return True;
}

auto Library::Language::Monograph::retain_import(const Import& import) -> Bool {
  if (imports_linked) {
    return False;
  }

  imports.insert(import);
  return True;
}

auto Library::Language::Monograph::link_imports() -> Bool {
  if (imports_linked) {
    return True;
  }

  if (imports.is_empty()) {
    imports_linked = True;
    return True;
  }

  const Abstract& source_context = interpretation_context.resolve();
  auto source_package =
      select_abstract<Package::Language::Monograph>(source_context);
  if (!source_package) {
    for (Count i = 0; i < imports.get_size(); i++) {
      report(
          Ttx::Lexical::Anchor::create(imports[i].get_span()),
          "Library Import source context is not a Package Monograph."_view,
          "Interpret this Library source inside its owning Package."_view);
    }

    return False;
  }

  auto source_structure = get_source().visit<Types::Structure>(
      [](Types::Structure& structure) -> Option<Types::Structure&> {
        return structure;
      },
      [](Abstract&) -> Option<Types::Structure&> { return {}; });
  if (!source_structure) {
    return False;
  }

  Managed::Vector<ImportCandidate> candidates(domain);
  Bool failed = False;

  // Package order leads member order and each provider source order. Staging
  // the complete sequence keeps the importing Structure unchanged until every
  // contextual source and collision has been observed.
  for (Count import_index = 0; import_index < imports.get_size();
       import_index++) {
    const Import& import = imports[import_index];
    View::Bytes route = import.get_route();
    Bool duplicate = False;
    for (Count earlier = 0; earlier < import_index; earlier++) {
      duplicate |= imports[earlier].get_route() == route;
    }

    if (duplicate) {
      report(
          Ttx::Lexical::Anchor::create(import.get_span()),
          "Library source repeats one exact Import route."_view,
          "Keep one authored Import for each Package member route."_view);
      failed = True;
      continue;
    }

    const Abstract& selected = source_package->resolve_context(route).resolve();
    auto target_package =
        select_abstract<Package::Language::Monograph>(selected);
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
    // Monograph cannot borrow a Library Structure to enter expansion.
    for (Count member_index = 0; member_index < members.get_size();
         member_index++) {
      const Alias& member = members.get_data()[member_index].get();
      const Abstract& member_target = member.resolve();
      auto provider_monograph = select_abstract<Monograph>(member_target);
      if (!provider_monograph) {
        const Abstract& claimed_source =
            member.resolve_context("source"_view).resolve();
        auto spoofed_source = select_abstract<Types::Structure>(claimed_source);
        if (spoofed_source && spoofed_source->is_source()) {
          report(
              Ttx::Lexical::Anchor::create(import.get_span()),
              "Non Library Package member claimed a Library source "
              "Structure."_view,
              "Bind the member to its exact Library Monograph before "
              "importing."_view);
          failed = True;
        }

        continue;
      }

      const Type& provider_source = provider_monograph->get_source();
      auto provider = select_abstract<Types::Structure>(provider_source);
      if (!provider || !provider->is_source()) {
        report(
            Ttx::Lexical::Anchor::create(import.get_span()),
            "Library Package member has no exact source Structure."_view,
            "Complete the provider Monograph before importing its Static "
            "bindings."_view);
        failed = True;
        continue;
      }

      auto bindings = provider->get_external_static_bindings();
      for (Count binding_index = 0; binding_index < bindings.get_size();
           binding_index++) {
        const Abstract& binding = bindings.get_data()[binding_index].get();
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

  // The source Structure owns local and environmental collisions. Candidate
  // order adds the import set dimension without copying that lookup policy back
  // onto Monograph.
  for (Count candidate_index = 0; candidate_index < candidates.get_size();
       candidate_index++) {
    const ImportCandidate& candidate = candidates[candidate_index];
    View::Bytes name = candidate.binding.get().get_name();
    if (!source_structure->can_bind_static(name)) {
      report(
          Ttx::Lexical::Anchor::create(candidate.import_span),
          "Imported Static binding collides with an occupied source name."_view,
          "Rename the local declaration or select another dependency."_view);
      failed = True;
    }

    for (Count earlier = 0; earlier < candidate_index; earlier++) {
      if (candidates[earlier].binding.get().get_name() != name) {
        continue;
      }

      report(
          Ttx::Lexical::Anchor::create(candidate.import_span),
          "Two Library Imports publish the same Static binding name."_view,
          "Import a dependency set with distinct exposed names."_view);
      failed = True;
    }
  }

  if (failed) {
    return False;
  }

  Managed::Vector<Reference<Alias>> aliases(domain);
  for (Count i = 0; i < candidates.get_size(); i++) {
    const Abstract& target = candidates[i].binding.get();
    Alias& alias = domain.construct<Alias>(target.get_name(), target);
    aliases.insert(alias);
  }

  // Alias construction cannot disturb source lookup. The earlier complete
  // name preflight makes each following bind infallible within this Monograph
  // transaction, so no candidate can become visible beside a later rejection.
  for (Count i = 0; i < aliases.get_size(); i++) {
    if (!source_structure->bind_static(aliases[i].get(), Visibility::Private)) {
      return False;
    }
  }

  imports_linked = True;
  return True;
}

auto Library::Language::Monograph::link() -> Bool {
  if (!link_imports()) {
    return False;
  }

  // Storage and Field phases remain global across the source. A later
  // declaration may therefore supply the exact Type of an earlier Field without
  // changing identity or introducing a discovery graph.
  Bool failed = False;
  for (Count i = 0; i < authored_bindings.get_size(); i++) {
    failed |= !authored_bindings[i].get().visit<Types::Enumeration>(
        [](Types::Enumeration& enumeration) {
          return enumeration.link_storage();
        },
        [](Abstract&) { return True; });
  }
  if (failed) {
    return False;
  }

  // Fields resolve after every scalar storage identity has settled. Waiting for
  // the complete phase keeps initializers from observing a partial Addressable
  // inventory when one Structure fails.
  for (Count i = 0; i < authored_bindings.get_size(); i++) {
    failed |= !authored_bindings[i].get().visit<Types::Structure>(
        [](Types::Structure& structure) { return structure.link_fields(); },
        [](Abstract&) { return True; });
  }
  if (failed) {
    return False;
  }

  // Initializers may use linked Fields from any authored Structure. Callable
  // linking remains closed until every initializer has accepted those edges.
  for (Count i = 0; i < authored_bindings.get_size(); i++) {
    failed |= !authored_bindings[i].get().visit<Types::Structure>(
        [](Types::Structure& structure) {
          return structure.link_initializers();
        },
        [](Abstract&) { return True; });
  }
  if (failed) {
    return False;
  }

  // The source Structure owns root Functions, so it joins the same signature
  // barrier as authored Structures. Its success also seals Static binding and
  // makes the completed import set permanent across retries.
  for (Count i = 0; i < authored_bindings.get_size(); i++) {
    failed |= !authored_bindings[i].get().visit<Types::Structure>(
        [](Types::Structure& structure) {
          return structure.link_callable_signatures();
        },
        [](Abstract&) { return True; });
  }
  failed |= !get_source().visit<Types::Structure>(
      [](Types::Structure& structure) {
        return structure.link_callable_signatures();
      },
      [](Abstract&) { return False; });
  if (failed) {
    return False;
  }

  // Bodies run only after every local signature has published stable Parameter
  // and result Layouts. The source participates again because root Functions
  // are ordinary hosted members rather than a second Monograph inventory.
  for (Count i = 0; i < authored_bindings.get_size(); i++) {
    failed |= !authored_bindings[i].get().visit<Types::Structure>(
        [](Types::Structure& structure) {
          return structure.link_callable_bodies();
        },
        [](Abstract&) { return True; });
  }
  failed |= !get_source().visit<Types::Structure>(
      [](Types::Structure& structure) {
        return structure.link_callable_bodies();
      },
      [](Abstract&) { return False; });

  return !failed;
}

auto Library::Language::Monograph::finalize() -> Bool {
  // Enumeration storage reaches its terminal semantic state before composite
  // owners validate publication. A failed enumeration therefore cannot leave a
  // later Structure looking finalized beside an incomplete scalar dependency.
  Bool failed = False;
  for (Count i = 0; i < authored_bindings.get_size(); i++) {
    failed |= !authored_bindings[i].get().visit<Types::Enumeration>(
        [](Types::Enumeration& enumeration) { return enumeration.finalize(); },
        [](Abstract&) { return True; });
  }
  if (failed) {
    return False;
  }

  // Authored Structures and the synthetic source share one publication phase.
  // Calling the source owner here lets it validate and finalize root Functions
  // through the same member rules without duplicating them on Monograph.
  for (Count i = 0; i < authored_bindings.get_size(); i++) {
    failed |= !authored_bindings[i].get().visit<Types::Structure>(
        [](Types::Structure& structure) { return structure.finalize(); },
        [](Abstract&) { return True; });
  }
  failed |= !get_source().visit<Types::Structure>(
      [](Types::Structure& structure) { return structure.finalize(); },
      [](Abstract&) { return False; });

  return !failed;
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

  return source.visit<Types::Structure>(
      [&](const Types::Structure& structure) -> const Abstract& {
        return structure.resolve_context(route);
      },
      [](const Abstract&) -> const Abstract& {
        return Invalid::get_invalid();
      });
}

auto Library::Language::Monograph::get_source() -> Type& {
  return source_structure->get();
}

auto Library::Language::Monograph::get_source() const -> const Type& {
  return source_structure->get();
}

auto Library::Language::Monograph::get_authored_bindings() const
    -> View::Vector<Reference<const Abstract>> {
  return authored_binding_observations;
}

auto Library::Language::Monograph::get_imports() const -> View::Vector<Import> {
  return imports;
}
