// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/foreign.hpp"

#include "tetrodotoxin/language/parser/comment.hpp"
#include "tetrodotoxin/library/language/model/type.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/lexical/lexicon.hpp"
#include "ttx/model/documentations/merged.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin;

static auto is_foreign_keyword(const Cursor& cursor) -> Bool {
  return cursor.matches(Code::Type::Addressable) &&
         cursor.current().caculate_text(cursor.get_source_text()) ==
             "foreign"_view;
}

template <typename declaration_type>
static auto has_same_declaration(
    const declaration_type& left,
    const declaration_type& right,
    View::Bytes source) -> Bool {
  return left.get_anchor().get_span().caculate_text(source) ==
         right.get_anchor().get_span().caculate_text(source);
}

Library::Language::Foreign::Foreign(Allocator::Arena& domain, Abstract& parent)
    : domain(domain),
      parent(parent),
      documentation(&Documentation::get_empty()),
      states(domain),
      functions(domain) {}

auto Library::Language::Foreign::parse(
    Cursor& cursor,
    const Documentation& block_documentation) -> Bool {
  if (stage != Stage::Authored) {
    cursor.create_token_error(
        "Foreign blocks cannot enter a Library source after linking begins."_view);
    return False;
  }

  if (!is_foreign_keyword(cursor)) {
    cursor.create_token_error(
        "Library Foreign blocks require the `foreign` keyword."_view);
    return False;
  }
  cursor.consume();

  Token abi_token = cursor.require(
      Code::Type::String,
      "Foreign blocks require one quoted ABI selector."_view);
  BAIL_IF(!abi_token);
  View::Bytes quoted = abi_token.caculate_text(cursor.get_source_text());
  if (!Lexicon::validate(Code::Type::String, quoted) || quoted.get_size() < 2) {
    cursor.create_token_error(
        abi_token, "Foreign ABI selector is not a closed String."_view);
    return False;
  }

  View::Bytes parsed_abi = quoted.slice(1, quoted.get_size() - 2);
  if (parsed_abi != "C"_view) {
    cursor.create_token_error(
        abi_token,
        "Foreign supports only the exact `\"C\"` ABI selector."_view);
    return False;
  }
  if (abi && *abi != parsed_abi) {
    cursor.create_token_error(
        abi_token, "Foreign blocks in one source must use the same ABI."_view);
    return False;
  }
  View::Bytes retained_abi = abi ? *abi : parsed_abi;

  BAIL_IF(!cursor.require(
      Code::Type::ScopeStart,
      "Foreign blocks require `{` before their declarations."_view));

  // Declarations stay private to this block until its closing token succeeds.
  // A malformed later block therefore cannot alter an earlier complete block.
  Managed::Vector<Reference<State>> staged_states(domain);
  Managed::Vector<Reference<Function>> staged_functions(domain);
  while (!cursor.matches(Code::Type::ScopeEnd)) {
    if (cursor.matches(Code::Type::Terminal)) {
      cursor.create_token_error(
          "Foreign block reached the end of source before `}`."_view);
      return False;
    }

    const Documentation& declaration_documentation =
        Tetrodotoxin::Language::Parser::Comment::parse(cursor);
    // Visibility plus the following qualifier selects one declaration owner
    // without consuming either token. That owner then parses its full grammar
    // from this same Cursor.
    Code::Type visibility = cursor.get_code().get_type();
    if (visibility != Code::Type::Public && visibility != Code::Type::Private &&
        visibility != Code::Type::Expose) {
      cursor.create_token_error(
          "Foreign declarations require authored visibility."_view);
      return False;
    }
    Code::Type qualifier = cursor.peek(1).get_code().get_type();

    if (qualifier == Code::Type::State || qualifier == Code::Type::Const) {
      auto state = State::interpret(
          *this, cursor, declaration_documentation, retained_abi);
      BAIL_IF(!state);
      const State* duplicate = nullptr;
      for (const Reference<State>& retained : states.get_view()) {
        if (retained.get().get_name() == state->get_name()) {
          duplicate = &retained.get();
          break;
        }
      }
      if (duplicate == nullptr) {
        for (const Reference<State>& retained : staged_states.get_view()) {
          if (retained.get().get_name() == state->get_name()) {
            duplicate = &retained.get();
            break;
          }
        }
      }
      if (duplicate != nullptr &&
          !has_same_declaration(*duplicate, *state, cursor.get_source_text())) {
        cursor.create_expression_error(
            state->get_anchor(),
            "Repeated Foreign State changes its declaration."_view,
            "Repeat the exact declaration or choose another symbol."_view);
        return False;
      }
      if (duplicate == nullptr) {
        staged_states.insert(*state);
      }
      continue;
    }

    if (qualifier == Code::Type::Func) {
      auto function = Function::interpret(
          *this, cursor, declaration_documentation, retained_abi);
      BAIL_IF(!function);
      const Function* duplicate = nullptr;
      for (const Reference<Function>& retained : functions.get_view()) {
        if (retained.get().get_name() == function->get_name()) {
          duplicate = &retained.get();
          break;
        }
      }
      if (duplicate == nullptr) {
        for (const Reference<Function>& retained :
             staged_functions.get_view()) {
          if (retained.get().get_name() == function->get_name()) {
            duplicate = &retained.get();
            break;
          }
        }
      }
      if (duplicate != nullptr &&
          !has_same_declaration(
              *duplicate, *function, cursor.get_source_text())) {
        cursor.create_expression_error(
            function->get_anchor(),
            "Repeated Foreign Function changes its declaration."_view,
            "Repeat the exact declaration or choose another symbol."_view);
        return False;
      }
      if (duplicate == nullptr) {
        staged_functions.insert(*function);
      }
      continue;
    }

    cursor.create_token_error(
        cursor.peek(1),
        "Foreign declarations require `state`, `const`, or `func`."_view);
    return False;
  }

  BAIL_IF(!cursor.require(
      Code::Type::ScopeEnd,
      "Foreign blocks require `}` after their declarations."_view));

  // State and Callable names form separate query domains. Byte identical
  // repeats keep the first identity, while any changed declaration rejects this
  // staged block before it can alter the retained Foreign context.
  retain_documentation(block_documentation);
  if (!abi) {
    abi = retained_abi;
  }
  for (const Reference<State>& state : staged_states.get_view()) {
    states.insert(state);
  }
  for (const Reference<Function>& function : staged_functions.get_view()) {
    functions.insert(function);
  }

  return True;
}

auto Library::Language::Foreign::link_types(Cursor& cursor) -> Bool {
  if (!is_authored() || stage >= Stage::TypesLinked) {
    return True;
  }
  BAIL_IF(stage != Stage::Authored);

  // External State Types close before signatures because Functions may name
  // them through the same Source lexical context.
  Bool failed = False;
  for (const Reference<State>& state : states.get_view()) {
    failed |= !state.get().link(cursor);
  }
  BAIL_IF(failed);

  stage = Stage::TypesLinked;
  return True;
}

auto Library::Language::Foreign::link_callables(Cursor& cursor) -> Bool {
  if (!is_authored()) {
    return True;
  }
  if (stage >= Stage::CallablesLinked) {
    return True;
  }
  BAIL_IF(stage != Stage::TypesLinked);

  Bool failed = False;
  for (const Reference<Function>& function : functions.get_view()) {
    failed |= !function.get().link(cursor);
  }
  BAIL_IF(failed);

  stage = Stage::CallablesLinked;
  return True;
}

auto Library::Language::Foreign::finalize(Cursor&) -> Bool {
  if (!is_authored() || stage == Stage::Finalized) {
    return True;
  }
  BAIL_IF(stage != Stage::CallablesLinked);
  stage = Stage::Finalized;
  return True;
}

auto Library::Language::Foreign::resolve() const -> const Abstract& {
  return is_authored() ? static_cast<const Abstract&>(*this)
                       : Invalid::get_invalid();
}

auto Library::Language::Foreign::resolve_context(View::Bytes route) const
    -> const Abstract& {
  // Foreign borrows Source lexical Type lookup for declaration routes only.
  // Its State and Callable names remain contained behind explicit access and
  // call queries, so they never become bare Source names.
  auto type = parent.select<Library::Language::Model::Type>();
  return type ? type->resolve_lexical_context(route)
              : parent.resolve_context(route);
}

auto Library::Language::Foreign::resolve_access(
    const Abstract&,
    View::Bytes route) const -> const Abstract& {
  for (const Reference<State>& state : states.get_view()) {
    if (state.get().get_name() == route) {
      return state.get();
    }
  }

  return Invalid::get_invalid();
}

auto Library::Language::Foreign::resolve_call(
    const Abstract&,
    View::Bytes route) const -> const Abstract& {
  for (const Reference<Function>& function : functions.get_view()) {
    if (function.get().get_name() == route) {
      return function.get();
    }
  }

  return Invalid::get_invalid();
}

auto Library::Language::Foreign::retain_documentation(
    const Documentation& block_documentation) -> void {
  // Repeated blocks describe the same Foreign identity, so their block prose
  // composes here. Each declaration still retains only its own parsed prose.
  if (!abi) {
    documentation = &block_documentation;
    return;
  }
  if (block_documentation.is_empty()) {
    return;
  }
  if (documentation->is_empty()) {
    documentation = &block_documentation;
    return;
  }

  documentation = &domain.construct<Ttx::Model::Documentations::Merged>(
      *documentation, block_documentation);
}
