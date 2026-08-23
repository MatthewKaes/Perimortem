// Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/archive/declaration.hpp"
#include "tetrodotoxin/library/language/foreign.hpp"

#include "ttx/concept/invalid.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Ttx::Model;
using namespace Tetrodotoxin::Library;

using Tetrodotoxin::Language::Visibility;

auto Language::Foreign::Function::persist(Archive::Writer& writer) const
    -> Bool {
  auto record = writer.begin(Archive::Tag::ForeignFunction);
  Archive::Declaration declaration(definition);
  BAIL_IF(
      !declaration.write(writer) || !writer.write(abi) ||
      !signature.persist(writer) || !writer.finish(record));
  return True;
}

auto Language::Foreign::Function::restore(
    Archive::Reader& reader,
    Allocator::Arena& arena,
    Foreign& host) -> Option<Function&> {
  auto record = reader.read_record();
  BAIL_IF(
      !record || record->get_tag() != U16(Archive::Tag::ForeignFunction) ||
      record->is_optional());

  Archive::Reader contents(record->get_payload());
  auto declaration = Archive::Declaration::read(contents, arena);
  auto abi = contents.read_bytes();
  auto signature = Signature::restore(contents, arena, host);
  BAIL_IF(
      !declaration || !abi || abi->is_empty() || !signature ||
      !contents.is_complete());

  auto& definition = declaration->create_definition(arena, host);
  return arena.construct_from<Function>([&]() -> Function {
    return Function(definition, *signature, arena.proxy(*abi));
  });
}

auto Language::Foreign::Function::create_authored(
    Allocator::Arena& domain,
    Tetrodotoxin::Language::Definition& definition,
    Signature& signature,
    View::Bytes abi) -> Function& {
  return domain.construct_from<Function>(
      [&]() -> Function { return Function(definition, signature, abi); });
}

auto Language::Foreign::Function::link(Cursor& cursor) -> Bool {
  if (linked) {
    return True;
  }
  BAIL_IF(!signature.link(cursor));
  linked = True;
  return True;
}

auto Language::Foreign::Function::link_restored_declaration_signature()
    -> Bool {
  BAIL_IF(!signature.link_restored());
  linked = True;
  return True;
}

auto Language::Foreign::Function::resolve() const -> const Abstract& {
  return linked ? static_cast<const Abstract&>(*this) : Invalid::get_invalid();
}

auto Language::Foreign::Function::resolve_context(View::Bytes) const
    -> const Abstract& {
  return Invalid::get_invalid();
}
