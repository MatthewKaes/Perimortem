// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/foreign.hpp"

#include "tetrodotoxin/library/language/model/type.hpp"
#include "ttx/bootstrap/concept/unknown.hpp"
#include "ttx/bootstrap/model/documentations/merged.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin;

Library::Language::Foreign::Foreign(Allocator::Arena& domain, Abstract& parent)
    : domain(domain),
      parent(parent),
      static_authority(domain.construct<Types::Static>(domain)),
      documentation(&Documentation::get_empty()),
      states(domain),
      functions(domain),
      declarations(domain) {}

auto Library::Language::Foreign::retain_block(
    const Documentation& block_documentation,
    View::Bytes selected_abi,
    View::Vector<State*> selected_states,
    View::Vector<Function*> selected_functions,
    View::Vector<Abstract*> selected_declarations) -> Bool {
  if (stage != Stage::Authored || selected_abi.is_empty() ||
      (abi && *abi != selected_abi)) {
    return False;
  }

  retain_documentation(block_documentation);
  if (!abi) {
    abi = selected_abi;
  }
  for (State* state : selected_states) {
    BAIL_IF(
        !static_authority.bind(*state, state->get_definition().is_published()));
    states.insert(state);
  }
  for (Function* function : selected_functions) {
    BAIL_IF(!static_authority.bind(
        *function, function->get_definition().is_published()));
    functions.insert(function);
  }
  for (Abstract* declaration : selected_declarations) {
    declarations.insert(declaration);
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
  for (State* state : states.get_view()) {
    failed |= !state->link(cursor);
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
  for (Function* function : functions.get_view()) {
    failed |= !function->link(cursor);
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

auto Library::Language::Foreign::link_restored() -> Bool {
  if (!is_authored()) {
    return True;
  }
  BAIL_IF(stage != Stage::Authored);

  for (State* state : states.get_view()) {
    BAIL_IF(!state->link_restored_declaration_type());
  }
  stage = Stage::TypesLinked;

  for (Function* function : functions.get_view()) {
    BAIL_IF(!function->link_restored_declaration_signature());
  }
  stage = Stage::CallablesLinked;
  return True;
}

auto Library::Language::Foreign::finalize_restored() -> Bool {
  if (!is_authored()) {
    return True;
  }
  BAIL_IF(stage != Stage::CallablesLinked);
  stage = Stage::Finalized;
  return True;
}

auto Library::Language::Foreign::resolve() const -> const Abstract& {
  return is_authored() ? static_cast<const Abstract&>(*this)
                       : Unknown::get_unknown();
}

auto Library::Language::Foreign::resolve_concept(View::Bytes route) const
    -> const Abstract& {
  if (route == "static"_view) {
    return static_authority;
  }

  // Foreign borrows Source lexical Type lookup for declaration routes only.
  // Its State and Callable names remain contained behind explicit access and
  // call queries, so they never become bare Source names.
  auto type = parent.select<Library::Language::Model::Type>();
  return type ? type->resolve_lexical_context(route)
              : parent.resolve_concept(route);
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
