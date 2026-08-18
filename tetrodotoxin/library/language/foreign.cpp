// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/foreign.hpp"

#include "tetrodotoxin/language/parser/comment.hpp"
#include "tetrodotoxin/library/language/model/type.hpp"
#include "tetrodotoxin/library/llvm/builder.hpp"
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
      Option<const State&> duplicate;
      for (const Reference<State>& retained : states.get_view()) {
        if (retained.get().get_name() == state->get_name()) {
          duplicate = retained.get();
          break;
        }
      }
      if (!duplicate) {
        for (const Reference<State>& retained : staged_states.get_view()) {
          if (retained.get().get_name() == state->get_name()) {
            duplicate = retained.get();
            break;
          }
        }
      }
      if (duplicate &&
          !has_same_declaration(*duplicate, *state, cursor.get_source_text())) {
        cursor.create_expression_error(
            state->get_anchor(),
            "Repeated Foreign State changes its declaration."_view,
            "Repeat the exact declaration or choose another symbol."_view);
        return False;
      }
      if (!duplicate) {
        staged_states.insert(*state);
      }
      continue;
    }

    if (qualifier == Code::Type::Func) {
      auto function = Function::interpret(
          *this, cursor, declaration_documentation, retained_abi);
      BAIL_IF(!function);
      Option<const Function&> duplicate;
      for (const Reference<Function>& retained : functions.get_view()) {
        if (retained.get().get_name() == function->get_name()) {
          duplicate = retained.get();
          break;
        }
      }
      if (!duplicate) {
        for (const Reference<Function>& retained :
             staged_functions.get_view()) {
          if (retained.get().get_name() == function->get_name()) {
            duplicate = retained.get();
            break;
          }
        }
      }
      if (duplicate && !has_same_declaration(
                           *duplicate, *function, cursor.get_source_text())) {
        cursor.create_expression_error(
            function->get_anchor(),
            "Repeated Foreign Function changes its declaration."_view,
            "Repeat the exact declaration or choose another symbol."_view);
        return False;
      }
      if (!duplicate) {
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

auto Library::Language::Foreign::State::reserve_declaration(
    Llvm::Program& program) const -> Bool {
  Bool type_reserved = Model::Addressable::reserve_declaration(program);
  if (!type_reserved) {
    return False;
  }

  const auto& globals = program.get_globals();
  Bool writable = Bool(
      get_definition().get_visibility() ==
      Tetrodotoxin::Language::Visibility::Public);
  auto reserved =
      globals.reserve_foreign(program, *this, abi, get_name(), writable);
  return reserved ? True : False;
}

auto Library::Language::Foreign::State::complete_declaration(
    Llvm::Program& program) const -> Bool {
  Bool type_completed = Model::Addressable::complete_declaration(program);
  if (!type_completed) {
    return False;
  }

  const auto& globals = program.get_globals();
  if (!globals.complete(program, *this)) {
    return False;
  }

  return program.get_debug().global(program, *this, definition, False, False);
}

auto Library::Language::Foreign::Function::reserve_declaration(
    Llvm::Program& program) const -> Bool {
  const auto& functions = program.get_functions();
  auto reserved = functions.reserve_foreign(program, *this, abi, get_symbol());
  if (!reserved) {
    return False;
  }

  return !*reserved || Model::Callable::reserve_declaration(program);
}

auto Library::Language::Foreign::Function::complete_declaration(
    Llvm::Program& program) const -> Bool {
  const auto& functions = program.get_functions();
  Bool signature_completed = Model::Callable::complete_declaration(program);
  return signature_completed && functions.complete(program, *this);
}

auto Library::Language::Foreign::reserve(Llvm::Program& program) const -> Bool {
  for (const Reference<State>& state : states.get_view()) {
    if (!state.get().reserve_declaration(program)) {
      return False;
    }
  }

  for (const Reference<Function>& function : functions.get_view()) {
    if (!function.get().reserve_declaration(program)) {
      return False;
    }
  }

  return True;
}

auto Library::Language::Foreign::complete(Llvm::Program& program) const
    -> Bool {
  for (const Reference<State>& state : states.get_view()) {
    if (!state.get().complete_declaration(program)) {
      return False;
    }
  }

  for (const Reference<Function>& function : functions.get_view()) {
    if (!function.get().complete_declaration(program)) {
      return False;
    }
  }

  return True;
}

auto Library::Language::Foreign::lower(Llvm::Program&) const -> Bool {
  // Foreign only defines the ABI so no body is actually lowered.
  return True;
}
