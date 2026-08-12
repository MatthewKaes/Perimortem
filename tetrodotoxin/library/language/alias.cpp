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

auto Tetrodotoxin::Library::Language::Alias::interpret(
    Allocator::Arena& domain,
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

  auto target_reference = TypeReference::parse(transaction);
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

static auto complete_alias(
    const Abstract& binding,
    Tetrodotoxin::Language::Monograph& source) -> Option<const Abstract&> {
  return binding.visit<Alias>(
      [&](const Alias& selected) -> Option<const Abstract&> {
        // Composite lookup is observationally const, while this ordered link
        // pass owns mutation of the exact retained Alias it selected.
        Alias& mutable_alias = const_cast<Alias&>(selected);
        BAIL_IF(!mutable_alias.link_target(source));
        return selected.resolve();
      },
      [](const Abstract& direct) -> Option<const Abstract&> {
        return direct.visit<Ttx::Model::Alias>(
            [](const Ttx::Model::Alias& alias) -> Option<const Abstract&> {
              const Abstract& resolved = alias.resolve();
              BAIL_IF(resolved.is<Invalid>());
              return resolved;
            },
            [](const Abstract& semantic) -> Option<const Abstract&> {
              return semantic;
            });
      });
}

static auto select_alias_target(
    const TypeReference& reference,
    const Types::Composite& host,
    Tetrodotoxin::Language::Monograph& source) -> Option<const Abstract&> {
  const Abstract* selected =
      &host.resolve_type_root(reference.get_root(), host);

  for (Count i = 1; i < reference.get_size(); i++) {
    auto context = complete_alias(*selected, source);
    BAIL_IF(!context);

    selected = &context->visit<Types::Composite>(
        [&](const Types::Composite& composite) -> const Abstract& {
          return composite.resolve_type(reference.get_name(i), host);
        },
        [&](const Abstract& semantic) -> const Abstract& {
          return semantic.resolve_context(reference.get_name(i));
        });
  }

  auto terminal = complete_alias(*selected, source);
  BAIL_IF(!terminal || !terminal->is<Type>());

  // Binding the raw final Type-space object preserves the authored Alias
  // chain. The terminal proof above uses only Alias::resolve() and therefore
  // never asks a staged direct Type to resolve away its reserved identity.
  return *selected;
}

auto Alias::link_target(Tetrodotoxin::Language::Monograph& source) -> Bool {
  if (stage == Stage::Linked) {
    return True;
  }
  if (stage == Stage::Linking) {
    source.report(
        target_reference.get_anchor(),
        "Library Alias Type references contain a cycle."_view,
        "Redirect every Alias chain to one concrete Type identity."_view);
    return False;
  }

  auto host = get_definition().get_host().select<Types::Composite>();
  if (!host) {
    source.report(
        get_anchor(), "Library Alias has no Composite Type scope."_view,
        "Retain the Alias on the Composite that owns its Definition."_view);
    return False;
  }

  stage = Stage::Linking;
  auto target = select_alias_target(target_reference, *host, source);
  if (!target) {
    stage = Stage::Unlinked;
    source.report(
        target_reference.get_anchor(),
        "Library Alias Type reference did not resolve to one Type."_view,
        "Publish the selected Type before linking this Alias."_view);
    return False;
  }

  if (!bind_target(*target)) {
    stage = Stage::Unlinked;
    source.report(
        target_reference.get_anchor(),
        "Library Alias cannot change its linked target."_view,
        "Keep one exact Type-space edge for this authored Alias."_view);
    return False;
  }

  const Documentation& local = get_definition().get_documentation();
  if (local.is_empty()) {
    documentation = target->get_documentation();
  } else {
    documentation = domain.construct<Ttx::Model::Documentations::Merged>(
        local, target->get_documentation());
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
