// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/alias.hpp"

#include "tetrodotoxin/library/language/access/type.hpp"
#include "tetrodotoxin/library/language/types/composite.hpp"
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

  auto route = Access::Type::parse(transaction);
  BAIL_IF(!route);
  Token terminator = transaction.require(
      Code::Type::EndStatement,
      "Library Alias definitions require one terminating `;`."_view);
  BAIL_IF(!terminator);

  const Abstract& resolved = host->resolve_type(*route);
  auto target = resolved.select<Type>();
  if (!target) {
    transaction.create_expression_error(
        route->get_anchor(),
        "Library Alias Type route did not resolve to one stable Type."_view);
    return {};
  }

  const Documentation* documentation = &target->get_documentation();
  if (!definition.get_documentation().is_empty()) {
    documentation = &domain.construct<Ttx::Model::Documentations::Merged>(
        definition.get_documentation(), target->get_documentation());
  }

  BAIL_IF(!definition.complete(alias_token, terminator));

  Alias& alias = domain.construct_from<Alias>(
      [&]() -> Alias { return Alias(definition, *target, *documentation); });
  cursor.join(transaction);
  return alias;
}
