// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/terminal/spirv/module/types.hpp"

#include "tetrodotoxin/library/language/model/addressable.hpp"
#include "tetrodotoxin/library/language/model/types/flag.hpp"
#include "tetrodotoxin/library/language/model/types/real.hpp"
#include "tetrodotoxin/library/language/model/types/signed.hpp"
#include "tetrodotoxin/library/language/model/types/unsigned.hpp"
#include "tetrodotoxin/library/language/types/structure.hpp"

using namespace Perimortem;
using namespace Ttx::Concept;
using namespace Tetrodotoxin;
using namespace Tetrodotoxin::Terminal::Spirv;

Module::Types::Types(Ids& ids)
    : ids(ids), void_id(ids.take()), function_id(ids.take()) {}

auto Module::Types::select(const Abstract& semantic)
    -> Core::Option<const Library::Language::Model::Type&> {
  auto addressable = semantic.select<Library::Language::Model::Addressable>();
  if (addressable) {
    return addressable->get_type();
  }
  return semantic.resolve().select<Library::Language::Model::Type>();
}

auto Module::Types::get_id(const Library::Language::Model::Type& type) const
    -> Core::Option<U32> {
  for (const Entry& entry : entries.get_view()) {
    if (&entry.type.get() == &type) {
      return entry.id;
    }
  }
  return {};
}

auto Module::Types::collect(const Library::Language::Model::Type& type)
    -> Bool {
  if (get_id(type)) {
    return True;
  }
  for (const Library::Language::Model::Type* active : visiting.get_view()) {
    BAIL_IF(active == &type);
  }

  auto value = type.select<Library::Language::Model::Types::Value>();
  if (value) {
    BAIL_IF(value->get_width() != 32);
    entries.insert(Entry(type, ids.take()));
    return True;
  }

  auto structure = type.select<Library::Language::Types::Structure>();
  BAIL_IF(!structure || structure->get_layout().is_empty());
  visiting.insert(&type);
  const Ttx::Concept::Layout& layout = structure->get_layout();
  for (Count index = 0; index < layout.get_size(); index++) {
    auto semantic = layout.get_abstract(index);
    auto member = semantic
                      ? select(*semantic)
                      : Core::Option<const Library::Language::Model::Type&>();
    BAIL_IF(!member || !collect(*member));
  }
  visiting.remove(visiting.get_size() - 1);
  entries.insert(Entry(type, ids.take()));
  return True;
}

auto Module::Types::collect_pointer(
    const Library::Language::Model::Type& type,
    Assembler::SpirV::StorageClass storage) -> Bool {
  BAIL_IF(!collect(type));
  if (get_pointer_id(type, storage)) {
    return True;
  }
  pointers.insert(Pointer(type, storage, ids.take()));
  return True;
}

auto Module::Types::get_pointer_id(
    const Library::Language::Model::Type& type,
    Assembler::SpirV::StorageClass storage) const -> Core::Option<U32> {
  for (const Pointer& pointer : pointers.get_view()) {
    if (&pointer.type.get() == &type && pointer.storage == storage) {
      return pointer.id;
    }
  }
  return {};
}

auto Module::Types::emit(Assembler::SpirV& assembler) const -> Bool {
  assembler.type_void(void_id);
  for (const Entry& entry : entries.get_view()) {
    const Library::Language::Model::Type& type = entry.type.get();
    if (type.is<Library::Language::Model::Types::Flag>()) {
      assembler.type_bool(entry.id);
      continue;
    }
    auto real = type.select<Library::Language::Model::Types::Real>();
    if (real) {
      assembler.type_float(entry.id, U32(real->get_width()));
      continue;
    }
    auto signed_type = type.select<Library::Language::Model::Types::Signed>();
    if (signed_type) {
      assembler.type_int(entry.id, U32(signed_type->get_width()), True);
      continue;
    }
    auto unsigned_type =
        type.select<Library::Language::Model::Types::Unsigned>();
    if (unsigned_type) {
      assembler.type_int(entry.id, U32(unsigned_type->get_width()), False);
      continue;
    }

    auto structure = type.select<Library::Language::Types::Structure>();
    BAIL_IF(!structure);
    Memory::Dynamic::Vector<U32> members;
    const Ttx::Concept::Layout& layout = structure->get_layout();
    for (Count index = 0; index < layout.get_size(); index++) {
      auto semantic = layout.get_abstract(index);
      auto member = semantic
                        ? select(*semantic)
                        : Core::Option<const Library::Language::Model::Type&>();
      auto id = member ? get_id(*member) : Core::Option<U32>();
      BAIL_IF(!id);
      members.insert(*id);
    }
    assembler.type_struct(entry.id, members.get_view());
  }

  for (const Pointer& pointer : pointers.get_view()) {
    auto type_id = get_id(pointer.type.get());
    BAIL_IF(!type_id);
    assembler.type_pointer(pointer.id, pointer.storage, *type_id);
  }
  assembler.type_function(function_id, void_id);
  return True;
}
