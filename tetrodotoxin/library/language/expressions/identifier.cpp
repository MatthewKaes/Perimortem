// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/expressions/identifier.hpp"

#include "tetrodotoxin/library/language/model/addressable.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/model/alias.hpp"

using namespace Perimortem;
using namespace Ttx::Concept;
using namespace Ttx::Model;
using namespace Tetrodotoxin::Library;

static auto resolve_alias(const Abstract& binding) -> const Abstract& {
  return binding.visit<Ttx::Model::Alias>(
      [](const Ttx::Model::Alias& alias) -> const Abstract& {
        return alias.resolve();
      },
      [](const Abstract& direct) -> const Abstract& { return direct; });
}

auto Language::Expressions::Identifier::link(
    Ttx::Lexical::Cursor& cursor,
    const Abstract& lexical_context,
    Core::Option<const Abstract&> access_scope) -> Bool {
  (void)token;
  (void)access_scope;
  const Abstract& candidate =
      resolve_alias(lexical_context.resolve_context(name));
  const Abstract& selected =
      candidate.is<Language::Model::Type>() ? candidate : candidate.resolve();
  auto source_anchor = get_anchor();

  if (selected.is<Invalid>()) {
    cursor.create_expression_error(
        source_anchor,
        "Expression Identifier did not resolve in its lexical context."_view,
        "Publish the named semantic object before linking this use."_view);
    return False;
  }

  // A later pass may fill an unresolved name, but a successful edge is
  // permanent. Repeating the same exact link remains harmless.
  if (result && &result->get() != &selected) {
    cursor.create_expression_error(
        source_anchor,
        "Expression Identifier cannot change its linked result."_view,
        "Keep one exact semantic object bound to this authored Token."_view);
    return False;
  }

  result = Reference<const Abstract>(selected);
  return True;
}

auto Language::Expressions::Identifier::get_documentation() const
    -> const Documentation& {
  return result.visit(
      []() -> const Documentation& { return Documentation::get_empty(); },
      [](const Reference<const Abstract>& selected) -> const Documentation& {
        return selected.get().get_documentation();
      });
}

auto Language::Expressions::Identifier::get_type() const -> const Abstract& {
  return result.visit(
      []() -> const Abstract& { return Invalid::get_invalid(); },
      [&](const Reference<const Abstract>& selected) -> const Abstract& {
        return selected.get().visit<Language::Model::Type>(
            [](const Language::Model::Type&) -> const Abstract& {
              return Invalid::get_invalid();
            },
            [](const Abstract& addressable) -> const Abstract& {
              return addressable.visit<Language::Model::Addressable>(
                  [](const Language::Model::Addressable& selected)
                      -> const Abstract& { return selected.get_type(); },
                  [](const Abstract&) -> const Abstract& {
                    return Invalid::get_invalid();
                  });
            });
      });
}

auto Language::Expressions::Identifier::get_result() const -> const Abstract& {
  return result.visit(
      []() -> const Abstract& { return Invalid::get_invalid(); },
      [](const Reference<const Abstract>& selected) -> const Abstract& {
        return selected.get();
      });
}
