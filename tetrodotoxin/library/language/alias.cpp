// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/alias.hpp"

#include "tetrodotoxin/library/language/type_reference.hpp"
#include "tetrodotoxin/library/language/types/composite.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/model/documentations/merged.hpp"
#include "ttx/model/type.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin::Library::Language;

using Ttx::Model::Type;

static auto resolve_alias(const Abstract& binding) -> const Abstract& {
  return binding.visit<Ttx::Model::Alias>(
      [](const Ttx::Model::Alias& alias) -> const Abstract& {
        return alias.resolve();
      },
      [](const Abstract& direct) -> const Abstract& { return direct; });
}

auto Alias::resolve_local_dependency(
    const Types::Composite& scope,
    View::Bytes route,
    const Type& caller_scope) -> const Abstract& {
  Tetrodotoxin::Language::Visibility visibility =
      scope.grants_private_access(caller_scope)
          ? Tetrodotoxin::Language::Visibility::Private
          : Tetrodotoxin::Language::Visibility::Public;
  for (const Reference<Abstract>& binding : scope.get_types(visibility)) {
    Abstract& candidate = binding.get();
    if (candidate.get_name() != route) {
      continue;
    }

    auto alias = candidate.select<Alias>();
    if (alias && !alias->link_target()) {
      return Invalid::get_invalid();
    }
    return candidate;
  }

  return Invalid::get_invalid();
}

auto Alias::resolve_root_dependency(
    const Types::Composite& scope,
    View::Bytes route,
    const Type& caller_scope) -> const Abstract& {
  const Types::Composite* current = &scope;
  while (current != nullptr) {
    const Abstract& local =
        resolve_local_dependency(*current, route, caller_scope);
    if (!local.is<Invalid>()) {
      return local;
    }

    auto enclosing = current->get_host().select<Types::Composite>();
    current = enclosing ? &*enclosing : nullptr;
  }

  const Abstract* source_host = &scope;
  while (auto enclosing = source_host->select<Types::Composite>()) {
    source_host = &enclosing->get_host();
  }
  auto source = source_host->select<Monograph>();
  if (!source) {
    return Invalid::get_invalid();
  }

  // Provider-first closure completion settles imported Library Alias edges
  // before this local DFS can observe them. External context and intrinsics
  // therefore remain read-only roots and never lend their lifecycle.
  const Abstract& outer =
      source->get_interpretation_context().resolve_context(route);
  if (!outer.is<Invalid>()) {
    return outer;
  }
  return source->get_dialect().resolve_intrinsic(route);
}

auto Tetrodotoxin::Library::Language::Alias::interpret(
    Allocator::Arena& domain,
    Monograph& source,
    Cursor& cursor,
    Tetrodotoxin::Language::Definition& definition) -> Option<Alias&> {
  auto transaction = cursor.branch();
  auto host = definition.get_host().select<Types::Composite>();
  BAIL_IF(!host);
  if (definition.get_name_token().get_code() != Code::Type::Type) {
    transaction.create_token_error(
        definition.get_name_token(),
        "Library Alias definitions require a Type shaped name."_view);
    return {};
  }
  if (definition.get_visibility() ==
      Tetrodotoxin::Language::Visibility::Exposed) {
    transaction.create_token_error(
        definition.get_visibility_token(),
        "Library Aliases accept only `public` or `private` visibility."_view);
    return {};
  }
  if (!definition.get_modifiers().is_empty()) {
    transaction.create_token_error(
        definition.get_modifiers().get_data()[0],
        "Library Aliases do not accept evaluation modifiers."_view);
    return {};
  }

  Token alias_token = transaction.require(
      Code::Type::Alias,
      "Library Alias definitions require the `alias` qualifier."_view);
  BAIL_IF(!alias_token);
  BAIL_IF(!transaction.require(
      Code::Type::Assign,
      "Library Alias qualifiers require `=` before their Type route."_view));

  auto target_reference = TypeReference::parse(source, transaction);
  BAIL_IF(!target_reference);
  Token terminator = transaction.require(
      Code::Type::EndStatement,
      "Library Alias definitions require one terminating `;`."_view);
  BAIL_IF(!terminator);

  BAIL_IF(!definition.complete(alias_token, terminator));

  Alias& alias = domain.construct_from<Alias>(
      [&]() -> Alias { return Alias(domain, definition, *target_reference); });
  cursor.join(transaction);
  return alias;
}

auto Alias::resolve_target_reference(
    const TypeReference& reference,
    const Type& caller_scope) -> const Abstract& {
  auto scope = caller_scope.select<Types::Composite>();
  if (!scope) {
    return Invalid::get_invalid();
  }

  const Abstract* raw =
      &resolve_root_dependency(*scope, reference.get_root(), caller_scope);
  for (Count i = 1; i < reference.get_size(); i++) {
    const Abstract& context = resolve_alias(*raw);
    if (context.is<Invalid>()) {
      return context;
    }

    raw = &context.visit<Types::Composite>(
        [&](const Types::Composite& composite) -> const Abstract& {
          return resolve_local_dependency(
              composite, reference.get_name(i), caller_scope);
        },
        [&](const Abstract& selected) -> const Abstract& {
          return selected.resolve_context(reference.get_name(i));
        });
  }

  const Abstract& selected = resolve_alias(*raw);
  if (selected.is<Invalid>()) {
    return selected;
  }

  for (Count i = 0; i < reference.get_argument_size(); i++) {
    auto nested = reference.get_argument_reference(i);
    if (!nested) {
      continue;
    }

    const Abstract& target = resolve_target_reference(*nested, caller_scope);
    if (!resolve_alias(target).is<Type>()) {
      return Invalid::get_invalid();
    }
  }

  const Abstract& result = reference.resolve_type(selected, caller_scope);
  if (result.is<Invalid>()) {
    return result;
  }

  // Bare routes retain their authored raw Alias edge. Generic application has
  // no application identity, so it binds the canonical materialized Type.
  return reference.has_arguments() ? result : *raw;
}

auto Alias::link_target() -> Bool {
  // An Alias reports through the Monograph reached from its own Definition
  // host. Another source may discover this Alias, but that lookup cannot
  // transfer lifecycle or diagnostic ownership.
  Abstract* source_host = &get_definition_host();
  while (auto enclosing = source_host->select<Types::Composite>()) {
    source_host = &enclosing->get_host();
  }
  auto source = source_host->select<Monograph>();
  BAIL_IF(!source);

  if (stage == Stage::Linked) {
    return True;
  }
  if (stage == Stage::Linking) {
    source->report(
        target_reference.get_anchor(),
        "Library Alias Type references contain a cycle."_view,
        "Redirect every Alias chain to one concrete Type identity."_view);
    return False;
  }

  auto host = get_definition().get_host().select<Types::Composite>();
  if (!host) {
    source->report(
        get_anchor(), "Library Alias has no Composite Type scope."_view,
        "Retain the Alias on the Composite that owns its Definition."_view);
    return False;
  }

  stage = Stage::Linking;
  const Abstract& target = resolve_target_reference(target_reference, *host);
  if (target.is<Invalid>()) {
    stage = Stage::Unlinked;
    source->report(
        target_reference.get_anchor(),
        "Library Alias Type reference did not resolve to one Type."_view,
        "Publish the selected Type before linking this Alias."_view);
    return False;
  }

  if (!bind_target(target)) {
    stage = Stage::Unlinked;
    source->report(
        target_reference.get_anchor(),
        "Library Alias cannot change its linked target."_view,
        "Keep one exact Type-space edge for this authored Alias."_view);
    return False;
  }

  const Documentation& local = get_definition().get_documentation();
  if (local.is_empty()) {
    documentation = target.get_documentation();
  } else {
    documentation = domain.construct<Ttx::Model::Documentations::Merged>(
        local, target.get_documentation());
  }

  stage = Stage::Linked;
  return True;
}

auto Alias::get_documentation() const -> const Documentation& {
  return documentation.visit(
      [&]() -> const Documentation& {
        return get_definition().get_documentation();
      },
      [](const Documentation& selected) -> const Documentation& {
        return selected;
      });
}
