// Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/foreign.hpp"

#include "tetrodotoxin/library/archive/declaration.hpp"
#include "tetrodotoxin/library/language/model/type.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/model/documentations/merged.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin;

Library::Language::Foreign::Foreign(Allocator::Arena& domain, Abstract& parent)
    : domain(domain),
      parent(parent),
      documentation(&Documentation::get_empty()),
      states(domain),
      functions(domain),
      declarations(domain) {}

auto Library::Language::Foreign::retain_authored_block(
    const Documentation& block_documentation,
    View::Bytes selected_abi,
    View::Vector<Reference<State>> selected_states,
    View::Vector<Reference<Function>> selected_functions,
    View::Vector<Reference<Abstract>> selected_declarations) -> Bool {
  if (stage != Stage::Authored || selected_abi.is_empty() ||
      (abi && *abi != selected_abi)) {
    return False;
  }

  retain_documentation(block_documentation);
  if (!abi) {
    abi = selected_abi;
  }
  for (const Reference<State>& state : selected_states) {
    states.insert(state);
  }
  for (const Reference<Function>& function : selected_functions) {
    functions.insert(function);
  }
  for (const Reference<Abstract>& declaration : selected_declarations) {
    declarations.insert(declaration);
  }
  return True;
}

auto Library::Language::Foreign::persist(Archive::Writer& writer) const
    -> Bool {
  auto record = writer.begin(Archive::Tag::Foreign);
  BAIL_IF(
      !writer.write(get_documentation()) || !abi || !writer.write(*abi) ||
      declarations.get_size() > U32(-1));

  writer.write(U32(declarations.get_size()));
  for (const Reference<Abstract>& declaration : declarations.get_view()) {
    auto state = declaration.get().select<State>();
    if (state) {
      BAIL_IF(!state->persist(writer));
      continue;
    }

    auto function = declaration.get().select<Function>();
    BAIL_IF(!function || !function->persist(writer));
  }
  return writer.finish(record);
}

auto Library::Language::Foreign::restore(
    Archive::Reader& reader,
    Allocator::Arena& arena,
    Abstract& parent) -> Option<Foreign&> {
  Foreign& foreign = arena.construct<Foreign>(arena, parent);
  BAIL_IF(!foreign.restore(reader));
  return foreign;
}

auto Library::Language::Foreign::restore(Archive::Reader& reader) -> Bool {
  auto record = reader.read_record();
  BAIL_IF(
      !record || record->get_tag() != U16(Archive::Tag::Foreign) ||
      record->is_optional());

  Archive::Reader contents(record->get_payload());
  auto restored_documentation = contents.read_documentation(domain);
  auto restored_abi = contents.read_bytes();
  auto count = contents.read_u32();
  BAIL_IF(
      !restored_documentation || !restored_abi || restored_abi->is_empty() ||
      !count);

  documentation = &*restored_documentation;
  abi = domain.proxy(*restored_abi);
  for (Count index = 0; index < *count; index++) {
    Archive::Reader probe = contents;
    auto declaration = probe.read_record();
    BAIL_IF(!declaration || declaration->is_optional());
    Archive::Tag tag = Archive::Tag(declaration->get_tag());
    if (tag == Archive::Tag::ForeignState) {
      auto state = State::restore(contents, domain, *this);
      BAIL_IF(!state);
      states.insert(*state);
      declarations.insert(*state);
    } else if (tag == Archive::Tag::ForeignFunction) {
      auto function = Function::restore(contents, domain, *this);
      BAIL_IF(!function);
      functions.insert(*function);
      declarations.insert(*function);
    } else {
      return False;
    }
  }
  BAIL_IF(!contents.is_complete());
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

auto Library::Language::Foreign::link_restored() -> Bool {
  if (!is_authored()) {
    return True;
  }
  BAIL_IF(stage != Stage::Authored);

  for (const Reference<State>& state : states.get_view()) {
    BAIL_IF(!state.get().link_restored_declaration_type());
  }
  stage = Stage::TypesLinked;

  for (const Reference<Function>& function : functions.get_view()) {
    BAIL_IF(!function.get().link_restored_declaration_signature());
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
