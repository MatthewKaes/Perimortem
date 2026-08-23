// Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/expressions/identifier.hpp"

#include "tetrodotoxin/library/language/diagnostics.hpp"
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
      candidate.is<Language::Model::Type>() ||
              candidate.is<Language::Model::Addressable>()
          ? candidate
          : candidate.resolve();
  auto source_anchor = get_anchor();

  if (selected.is<Invalid>()) {
    auto report = cursor.create_report(source_anchor);
    report << "Identifier '"_view << name
           << "' is not available in lexical context '"_view
           << lexical_context.get_name() << "'."_view;
    report.get_hint()
        << "Declare '"_view << name
        << "' before this use or correct the authored spelling."_view;
    return False;
  }

  // A later pass may fill an unresolved name, but a successful edge is
  // permanent. Repeating the same exact link remains harmless.
  if (result && &result->get() != &selected) {
    auto report = cursor.create_report(source_anchor);
    report << "Internal semantic error: Identifier '"_view << name
           << "' changed identity from '"_view << result->get().get_name()
           << "' to '"_view << selected.get_name() << "'."_view;
    report.get_hint()
        << "The source is valid; report this unstable linking result."_view;
    return False;
  }

  result = Reference<const Abstract>(selected);
  return True;
}

auto Language::Expressions::Identifier::link_restored(
    const Abstract& lexical_context,
    Core::Option<const Abstract&>) -> Bool {
  const Abstract& candidate =
      resolve_alias(lexical_context.resolve_context(name));
  const Abstract& selected =
      candidate.is<Language::Model::Type>() ||
              candidate.is<Language::Model::Addressable>()
          ? candidate
          : candidate.resolve();
  BAIL_IF(selected.is<Invalid>());
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
