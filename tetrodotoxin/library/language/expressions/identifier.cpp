// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/expressions/identifier.hpp"

#include "tetrodotoxin/language/import.hpp"
#include "tetrodotoxin/library/language/diagnostics.hpp"
#include "tetrodotoxin/library/language/flow/block.hpp"
#include "tetrodotoxin/library/language/model/addressable.hpp"
#include "ttx/concept/none.hpp"
#include "ttx/concept/unknown.hpp"
#include "tetrodotoxin/language/binding.hpp"

using namespace Perimortem;
using namespace Ttx::Concept;
using namespace Ttx::Model;
using namespace Tetrodotoxin::Library;

static auto resolve_alias(const Abstract& binding) -> const Abstract& {
  return binding.visit<Tetrodotoxin::Language::Import>(
      [](const Tetrodotoxin::Language::Import& import) -> const Abstract& {
        return import.resolve();
      },
      [](const Abstract& candidate) -> const Abstract& {
        return candidate.visit<Tetrodotoxin::Language::Binding>(
            [](const Tetrodotoxin::Language::Binding& alias) -> const Abstract& {
              return alias.resolve();
            },
            [](const Abstract& direct) -> const Abstract& { return direct; });
      });
}

auto Language::Expressions::Identifier::link(
    Ttx::Lexical::Cursor& cursor,
    const Abstract& lexical_context,
    Core::Option<const Abstract&> access_scope) -> Bool {
  (void)token;
  (void)access_scope;
  const Abstract& candidate =
      resolve_alias(lexical_context.resolve_concept(name));
  const Abstract& selected = candidate.is<Language::Model::Type>() ||
                                     candidate.is<Ttx::Model::Addressable>()
                                 ? candidate
                                 : candidate.resolve();
  auto source_anchor = get_anchor();

  if (selected.is<Unknown>() || selected.is<None>()) {
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
  if (result && *result != &selected) {
    auto report = cursor.create_report(source_anchor);
    report << "Internal semantic error: Identifier '"_view << name
           << "' changed identity from '"_view << (**result).get_name()
           << "' to '"_view << selected.get_name() << "'."_view;
    report.get_hint()
        << "The source is valid; report this unstable linking result."_view;
    return False;
  }

  result = &selected;
  if (source_anchor) {
    cursor.get_associations().create(*source_anchor, selected);
  }
  return True;
}

auto Language::Expressions::Identifier::link_restored(
    const Abstract& lexical_context,
    Core::Option<const Abstract&>) -> Bool {
  const Abstract& candidate =
      resolve_alias(lexical_context.resolve_concept(name));
  const Abstract& selected = candidate.is<Language::Model::Type>() ||
                                     candidate.is<Ttx::Model::Addressable>()
                                 ? candidate
                                 : candidate.resolve();
  BAIL_IF(selected.is<Unknown>() || selected.is<None>());
  result = &selected;
  return True;
}

auto Language::Expressions::Identifier::get_documentation() const
    -> const Documentation& {
  return result.visit(
      []() -> const Documentation& { return Documentation::get_empty(); },
      [](const Abstract* selected) -> const Documentation& {
        return selected->get_documentation();
      });
}

auto Language::Expressions::Identifier::get_type() const -> const Abstract& {
  // A malformed postfix may keep this Identifier outside the normal body
  // completion pass. Its authored Block can still requery the declaration
  // visible at this Token, which preserves editor observations without caching
  // a source answer or forcing unrelated completion phases to run.
  const Abstract& direct = result ? **result : resolve_authored();
  auto pack = Language::Model::Pack::from(direct);
  if (pack) {
    return pack->get_type();
  }
  return direct.visit<Language::Model::Type>(
      [](const Language::Model::Type&) -> const Abstract& {
        return Unknown::get_unknown();
      },
      [](const Abstract& addressable) -> const Abstract& {
        return addressable.visit<Ttx::Model::Addressable>(
            [](const Ttx::Model::Addressable& selected) -> const Abstract& {
              return selected.get_domain();
            },
            [](const Abstract&) -> const Abstract& {
              return Unknown::get_unknown();
            });
      });
}

auto Language::Expressions::Identifier::get_result() const -> const Abstract& {
  return result.visit(
      []() -> const Abstract& { return Unknown::get_unknown(); },
      [](const Abstract* selected) -> const Abstract& { return *selected; });
}

auto Language::Expressions::Identifier::resolve_concept(
    Core::View::Bytes query) const -> const Abstract& {
  if (query == "fold"_view) {
    const Abstract& selected = get_result();
    return selected.is<Unknown>() ? selected : selected.resolve_concept(query);
  }
  const Abstract& inherited = Expression::resolve_concept(query);
  if (!inherited.is<Unknown>()) {
    return inherited;
  }

  const Abstract& authored = resolve_authored();
  return authored.is<Unknown>() || authored.is<None>()
             ? authored
             : authored.resolve_concept(query);
}

auto Language::Expressions::Identifier::visit_concepts(
    ttx_named_abstract_callable* visitor) const -> void {
  Expression::visit_concepts(visitor);
  visit_concept(visitor, "fold"_view, resolve_concept("fold"_view));
}

auto Language::Expressions::Identifier::resolve_authored() const
    -> const Abstract& {
  if (result) {
    return **result;
  }

  const Abstract& context = *lexical_context;
  auto block = context.select<Language::Flow::Block>();
  const Abstract& candidate =
      block && token
          ? block->resolve_authored_context(name, Count(token.get_offset()))
          : context.resolve_concept(name);
  return resolve_alias(candidate);
}
