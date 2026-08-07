// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/monograph.hpp"

#include "tetrodotoxin/package/language/monograph.hpp"
#include "ttx/concept/invalid.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Ttx::Concept;
using namespace Ttx::Model;
using namespace Tetrodotoxin;

struct ImportCandidate {
  Ttx::Lexical::Span import_span;
  Reference<const Library::Language::Function> function;
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
      functions(domain),
      structures(domain),
      authored_functions(domain),
      public_functions(domain),
      authored_structures(domain),
      public_structures(domain) {}

auto Library::Language::Monograph::create_authored(
    Allocator::Arena& domain,
    const Documentation& documentation,
    Library::Dialect& host,
    const Abstract& interpretation_context,
    Materializations& materializations) -> Monograph& {
  return domain.construct_from<Monograph>([&]() -> Monograph {
    return Monograph(
        domain, documentation, host, interpretation_context, materializations);
  });
}

auto Library::Language::Monograph::bind_function(Function& function) -> Bool {
  const View::Bytes name = function.get_name();

  // Both mutations happen after the exact duplicate check. A rejected
  // declaration therefore preserves the first edge and its public position
  // while the failed interpretation discards the unreachable transaction.
  const Abstract& outer = interpretation_context.resolve_context(name);
  const Abstract& intrinsic = library_host.resolve_intrinsic(name);
  if (name.is_empty() || functions.contains(name) ||
      structures.contains(name) || &outer != &Invalid::get_invalid() ||
      &intrinsic != &Invalid::get_invalid()) {
    return False;
  }

  functions.launder(name, Reference<const Function>(function));
  authored_functions.insert(function);
  if (function.get_visibility() == Visibility::Public) {
    public_functions.insert(function);
  }

  return True;
}

auto Library::Language::Monograph::bind_structure(Types::Structure& structure)
    -> Bool {
  const View::Bytes name = structure.get_name();
  const Abstract& outer = interpretation_context.resolve_context(name);
  const Abstract& intrinsic = library_host.resolve_intrinsic(name);
  if (name.is_empty() || structures.contains(name) ||
      functions.contains(name) || &outer != &Invalid::get_invalid() ||
      &intrinsic != &Invalid::get_invalid()) {
    return False;
  }

  structures.launder(name, Reference<const Types::Structure>(structure));
  authored_structures.insert(structure);
  if (structure.get_visibility() == Visibility::Public) {
    public_structures.insert(structure);
  }

  return True;
}

auto Library::Language::Monograph::retain_import(const Import& import) -> void {
  imports.insert(import);
}

auto Library::Language::Monograph::link_imports() -> Bool {
  if (imports.is_empty()) {
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
    for (Count member_index = 0; member_index < members.get_size();
         member_index++) {
      const Alias& member = members.get_data()[member_index].get();
      const Abstract& provider = member.resolve();
      auto library = select_abstract<Library::Language::Monograph>(provider);
      if (!library) {
        continue;
      }

      View::Vector<Reference<const Function>> public_functions =
          library->get_public_functions();
      for (Count function_index = 0;
           function_index < public_functions.get_size(); function_index++) {
        const Function& function =
            public_functions.get_data()[function_index].get();
        if (!function.is_complete()) {
          report(
              Ttx::Lexical::Anchor::create(import.get_span()),
              "Library Import exposes an incomplete provider Function."_view,
              "Complete provider Function grammar before linking imports."_view);
          failed = True;
          continue;
        }

        ImportCandidate candidate = {
          .import_span = import.get_span(),
          .function = function,
        };
        candidates.insert(candidate);
      }
    }
  }

  // Local declarations, parent context, intrinsics, and every earlier
  // candidate participate in one exact collision domain. The complete scan
  // runs even after a failure so each rejected edge keeps the provider facts
  // that Retention cannot reconstruct.
  for (Count candidate_index = 0; candidate_index < candidates.get_size();
       candidate_index++) {
    const ImportCandidate& candidate = candidates[candidate_index];
    const Function& function = candidate.function.get();
    View::Bytes name = function.get_name();
    auto local_entry = functions.find(name);
    if (local_entry || structures.contains(name)) {
      View::Bytes message = local_entry
                                ? "Imported Function collides with one local "
                                  "Function name."_view
                                : "Imported Function collides with one local "
                                  "Structure name."_view;
      report(
          Ttx::Lexical::Anchor::create(candidate.import_span), message,
          "Rename the local declaration or select another dependency."_view);
      failed = True;
    }

    const Abstract& outer = interpretation_context.resolve_context(name);
    const Abstract& intrinsic = library_host.resolve_intrinsic(name);
    if (&outer != &Invalid::get_invalid() ||
        &intrinsic != &Invalid::get_invalid()) {
      report(
          Ttx::Lexical::Anchor::create(candidate.import_span),
          "Imported Function collides with an occupied context name."_view,
          "Choose a dependency whose public names do not shadow this "
          "Library context."_view);
      failed = True;
    }

    for (Count earlier = 0; earlier < candidate_index; earlier++) {
      const ImportCandidate& conflict = candidates[earlier];
      const Function& conflicting_function = conflict.function.get();
      if (conflicting_function.get_name() != name) {
        continue;
      }

      report(
          Ttx::Lexical::Anchor::create(candidate.import_span),
          "Two Library Imports publish the same Function name."_view,
          "Import a dependency set with distinct public Function names."_view);
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

auto Library::Language::Monograph::link() -> Bool {
  Bool failed = !link_imports();

  // Every Structure field settles against the complete reserved Type surface.
  // Function signatures follow only after those Types can answer resolution,
  // keeping declaration order irrelevant without a provisional Type graph.
  for (Count i = 0; i < authored_structures.get_size(); i++) {
    failed |= !authored_structures[i].get().link_fields();
  }

  for (Count i = 0; i < authored_structures.get_size(); i++) {
    failed |= !authored_structures[i].get().link_callable_signatures();
  }

  for (Count i = 0; i < authored_functions.get_size(); i++) {
    failed |= !authored_functions[i].get().link_signature();
  }

  for (Count i = 0; i < authored_structures.get_size(); i++) {
    failed |= !authored_structures[i].get().link_callable_bodies();
  }

  for (Count i = 0; i < authored_functions.get_size(); i++) {
    failed |= !authored_functions[i].get().link_body();
  }

  return !failed;
}

auto Library::Language::Monograph::finalize() -> Bool {
  Bool failed = False;

  // Structure owns member visibility because only that Type knows which field
  // and nested Callable edges form its public surface.
  for (Count i = 0; i < authored_structures.get_size(); i++) {
    failed |= !authored_structures[i].get().finalize();
  }

  // Root Functions bypass Structure membership, so Monograph applies the same
  // private Type boundary before Environment can publish this source.
  for (Count function_index = 0; function_index < public_functions.get_size();
       function_index++) {
    const Function& function = public_functions[function_index].get();
    auto signature = function.get_signature();
    if (!signature) {
      report(
          Ttx::Lexical::Anchor::create(
              Ttx::Lexical::Span(function.get_name_token())),
          "Public Function has no complete Signature during finalization."_view,
          "Link its exact parameter and result Types before publication."_view);
      failed = True;
      continue;
    }

    auto reject_private_structure = [&](Count index, Bool parameter) {
      auto type = parameter ? signature->get_parameter_type(index)
                            : signature->get_result_type(index);
      if (!type) {
        return;
      }

      Bool private_structure = type->visit<Types::Structure>(
          [](const Types::Structure& structure) {
            return structure.get_visibility() == Visibility::Private ? True
                                                                     : False;
          },
          [](const Abstract&) { return False; });
      if (!private_structure) {
        return;
      }

      auto type_anchor = parameter ? signature->get_parameter_type_anchor(index)
                                   : signature->get_result_type_anchor(index);
      report(
          type_anchor,
          "Public Function exposes a private local Structure Type."_view,
          "Keep the Function private or publish its exact Structure Type."_view);
      failed = True;
    };

    for (Count i = 0; i < signature->get_parameter_size(); i++) {
      reject_private_structure(i, True);
    }
    for (Count i = 0; i < signature->get_result_size(); i++) {
      reject_private_structure(i, False);
    }
  }

  // Optional fold caching stays on each Function after publication policy has
  // observed the linked signature identities.
  for (Count i = 0; i < authored_functions.get_size(); i++) {
    failed |= !authored_functions[i].get().finalize();
  }

  return !failed;
}

auto Library::Language::Monograph::get_name() const -> View::Bytes {
  return "Library"_view;
}

auto Library::Language::Monograph::resolve_context(View::Bytes route) const
    -> const Abstract& {
  // Raw local identities lead both fallbacks, including declarations whose
  // semantic edges remain incomplete. Lookup never resolves them on behalf of
  // a consumer that needs to observe that state.
  return structures.visit(
      route,
      [](const Reference<const Types::Structure>& structure)
          -> const Abstract& { return structure.get(); },
      [&]() -> const Abstract& {
        return functions.visit(
            route,
            [](const Reference<const Function>& function) -> const Abstract& {
              return function.get();
            },
            [&]() -> const Abstract& {
              const Abstract& outer =
                  interpretation_context.resolve_context(route);
              if (&outer != &Invalid::get_invalid()) {
                return outer;
              }

              return library_host.resolve_intrinsic(route);
            });
      });
}

auto Library::Language::Monograph::get_public_functions() const
    -> View::Vector<Reference<const Function>> {
  return public_functions;
}

auto Library::Language::Monograph::get_functions() const
    -> View::Vector<Reference<Function>> {
  return authored_functions;
}

auto Library::Language::Monograph::get_public_structures() const
    -> View::Vector<Reference<const Types::Structure>> {
  return public_structures;
}

auto Library::Language::Monograph::get_structures() const
    -> View::Vector<Reference<Types::Structure>> {
  return authored_structures;
}

auto Library::Language::Monograph::get_imports() const -> View::Vector<Import> {
  return imports;
}
