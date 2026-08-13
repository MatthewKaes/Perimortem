// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/foreign/surface.hpp"

#include "tetrodotoxin/language/parser/comment.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/lexical/lexicon.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Ttx::Model;
using namespace Tetrodotoxin::Library;

auto Language::Foreign::Surface::create(
    Allocator::Arena& domain,
    const Type& resolution_scope) -> Surface& {
  return domain.construct_from<Surface>(
      [&]() -> Surface { return Surface(domain, resolution_scope); });
}

static auto is_foreign_keyword(const Cursor& cursor) -> Bool {
  return cursor.matches(Code::Type::Addressable) &&
         cursor.current().caculate_text(cursor.get_source_text()) ==
             "foreign"_view;
}

static auto same_category(const Abstract& first, const Abstract& second)
    -> Bool {
  return (first.is<Language::Foreign::State>() &&
          second.is<Language::Foreign::State>()) ||
         (first.is<Language::Foreign::Function>() &&
          second.is<Language::Foreign::Function>());
}

auto Language::Foreign::Surface::parse(Monograph& source, Cursor& cursor)
    -> Bool {
  if (stage != Stage::Authored) {
    cursor.create_token_error(
        "Foreign blocks cannot enter a Library source after linking begins."_view);
    return False;
  }

  auto transaction = cursor.branch();
  Tetrodotoxin::Language::Parser::Comment::parse(transaction);
  if (!is_foreign_keyword(transaction)) {
    transaction.create_token_error(
        "Library Foreign blocks require the `foreign` keyword."_view);
    return False;
  }
  transaction.consume();

  Token abi_token = transaction.require(
      Code::Type::String,
      "Foreign blocks require one quoted ABI selector."_view);
  BAIL_IF(!abi_token);
  View::Bytes quoted = abi_token.caculate_text(transaction.get_source_text());
  if (!Lexicon::validate(Code::Type::String, quoted) || quoted.get_size() < 2) {
    transaction.create_token_error(
        abi_token, "Foreign ABI selector is not a closed String."_view);
    return False;
  }
  View::Bytes parsed_abi = quoted.slice(1, quoted.get_size() - 2);
  if (parsed_abi != "C"_view) {
    transaction.create_token_error(
        abi_token,
        "Foreign supports only the exact `\"C\"` ABI selector."_view);
    return False;
  }
  if (abi && *abi != parsed_abi) {
    transaction.create_token_error(
        abi_token,
        "Foreign blocks in one source must use the retained Surface ABI."_view);
    return False;
  }

  // The complete block remains one parser transaction. Arena objects from a
  // rejected declaration stay unreachable, while a successful close publishes
  // the real identities in authored order to the one source Surface.
  BAIL_IF(!transaction.require(
      Code::Type::ScopeStart,
      "Foreign blocks require `{` before their declarations."_view));

  Managed::Vector<Reference<Abstract>> staged(domain);
  while (!transaction.matches(Code::Type::ScopeEnd)) {
    auto probe = transaction.branch();
    Tetrodotoxin::Language::Parser::Comment::parse(probe);
    Code::Type visibility = probe.get_code().get_type();
    if (visibility != Code::Type::Public && visibility != Code::Type::Private &&
        visibility != Code::Type::Expose) {
      transaction.create_token_error(
          "Foreign declarations require authored visibility."_view);
      return False;
    }
    probe.consume();

    Option<Abstract&> declaration;
    if (probe.matches(Code::Type::State) || probe.matches(Code::Type::Const)) {
      auto state = State::interpret(
          domain, source, transaction, resolution_scope, *this);
      BAIL_IF(!state);
      declaration = *state;
    } else if (probe.matches(Code::Type::Func)) {
      auto function = Function::interpret(
          domain, source, transaction, resolution_scope, *this);
      BAIL_IF(!function);
      declaration = *function;
    } else {
      transaction.create_token_error(
          probe.current(),
          "Foreign declarations require `state`, `const`, or `func`."_view);
      return False;
    }

    const Abstract& candidate = *declaration;
    auto collides = [&](const Reference<Abstract>& retained) -> Bool {
      return same_category(retained.get(), candidate) &&
             retained.get().get_name() == candidate.get_name();
    };
    if (declarations.get_view().contains(collides) ||
        staged.get_view().contains(collides)) {
      auto authorship = candidate.get_authorship();
      transaction.create_expression_error(
          authorship ? authorship->get_anchor()
                     : Anchor::create(Span(transaction.peek(-1))),
          "Foreign declaration collides with its occupied category."_view,
          "Keep each State or Function symbol unique within that category."_view);
      return False;
    }
    staged.insert(*declaration);
  }

  BAIL_IF(!transaction.require(
      Code::Type::ScopeEnd,
      "Foreign blocks require `}` after their declarations."_view));
  if (!abi) {
    // ABI belongs to the complete source Surface rather than each declaration.
    // State and Function retain this Surface and borrow the one committed fact.
    abi = domain.proxy(parsed_abi);
  }
  for (const Reference<Abstract>& declaration : staged.get_view()) {
    declarations.insert(declaration);
  }
  cursor.join(transaction);
  return True;
}

auto Language::Foreign::Surface::link_types(Monograph& source) -> Bool {
  if (stage >= Stage::TypesLinked) {
    return True;
  }
  BAIL_IF(stage != Stage::Authored);

  // State Types settle after the parent Source publishes its declaration Type
  // category. Every declaration is visited so independent diagnostics remain
  // visible without admitting a partially linked Surface.
  Bool failed = False;
  for (const Reference<Abstract>& declaration : declarations.get_view()) {
    auto state = declaration.get().select<State>();
    failed |= Bool(state && !state->link(source));
  }
  BAIL_IF(failed);

  stage = Stage::TypesLinked;
  return True;
}

auto Language::Foreign::Surface::link_callables(Monograph& source) -> Bool {
  if (stage >= Stage::CallablesLinked) {
    return True;
  }
  BAIL_IF(stage != Stage::TypesLinked);

  // Callable signatures use the same completed Source Type scope as State.
  // The Surface advances only after every retained Signature accepts it.
  Bool failed = False;
  for (const Reference<Abstract>& declaration : declarations.get_view()) {
    auto function = declaration.get().select<Function>();
    failed |= Bool(function && !function->link(source));
  }
  BAIL_IF(failed);

  stage = Stage::CallablesLinked;
  return True;
}

auto Language::Foreign::Surface::finalize(Monograph&) -> Bool {
  if (stage == Stage::Finalized) {
    return True;
  }
  BAIL_IF(stage != Stage::CallablesLinked);
  stage = Stage::Finalized;
  return True;
}

auto Language::Foreign::Surface::resolve_context(View::Bytes) const
    -> const Abstract& {
  return Invalid::get_invalid();
}

auto Language::Foreign::Surface::select_state(View::Bytes name) const
    -> Option<const State&> {
  Option<const State&> selected;
  for (const Reference<Abstract>& declaration : declarations.get_view()) {
    auto state = declaration.get().select<State>();
    if (state && state->get_name() == name) {
      BAIL_IF(selected);
      selected = *state;
    }
  }
  return selected;
}

auto Language::Foreign::Surface::select_function(View::Bytes name) const
    -> Option<const Function&> {
  Option<const Function&> selected;
  for (const Reference<Abstract>& declaration : declarations.get_view()) {
    auto function = declaration.get().select<Function>();
    if (function && function->get_name() == name) {
      BAIL_IF(selected);
      selected = *function;
    }
  }
  return selected;
}
