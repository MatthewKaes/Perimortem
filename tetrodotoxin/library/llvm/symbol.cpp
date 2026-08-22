// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/llvm/symbol.hpp"

#include "perimortem/memory/managed/bytes.hpp"

#include "tetrodotoxin/library/language/field.hpp"
#include "tetrodotoxin/library/language/function.hpp"
#include "tetrodotoxin/library/language/types/composite.hpp"
#include "tetrodotoxin/library/language/types/enumeration.hpp"
#include "tetrodotoxin/library/language/types/source.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin::Library;
using namespace Ttx;

static auto append_encoded_name(
    Memory::Managed::Bytes& output,
    Core::View::Bytes value) -> void {
  constexpr auto digits = "0123456789abcdef"_view;

  for (Count index = 0; index < value.get_size(); index++) {
    U8 byte = value[index];
    Bool alphanumeric = Bool(
        (byte >= 'a' && byte <= 'z') || (byte >= 'A' && byte <= 'Z') ||
        (byte >= '0' && byte <= '9'));
    if (alphanumeric) {
      output.append(byte);
    } else {
      output.append('_');
      output.append(digits[byte >> 4]);
      output.append(digits[byte & 15]);
    }
  }
}

static auto append_symbol_path(
    Memory::Managed::Bytes& output,
    const Concept::Abstract& value,
    Count path_start) -> void {
  auto function = value.select<Language::Function>();
  if (function) {
    append_symbol_path(
        output, function->get_definition().get_host(), path_start);
    if (output.get_size() != path_start) {
      output.concat("__"_view);
    }

    append_encoded_name(output, function->get_name());
    return;
  }

  auto field = value.select<Language::Field>();
  if (field) {
    append_symbol_path(output, field->get_definition().get_host(), path_start);
    if (output.get_size() != path_start) {
      output.concat("__"_view);
    }

    append_encoded_name(output, field->get_name());
    return;
  }

  auto composite = value.select<Language::Types::Composite>();
  if (composite) {
    if (composite->is<Language::Types::Source>()) {
      return;
    }

    append_symbol_path(
        output, composite->get_definition().get_host(), path_start);
    if (output.get_size() != path_start) {
      output.concat("__"_view);
    }

    append_encoded_name(output, composite->get_name());
    return;
  }

  auto enumeration = value.select<Language::Types::Enumeration>();
  if (enumeration) {
    append_symbol_path(
        output, enumeration->get_definition().get_host(), path_start);
    if (output.get_size() != path_start) {
      output.concat("__"_view);
    }

    append_encoded_name(output, enumeration->get_name());
    return;
  }

  append_encoded_name(output, value.get_name());
}

auto Tetrodotoxin::Library::Llvm::Symbol::validate(Core::View::Bytes value)
    -> Bool {
  if (value.is_empty()) {
    return False;
  }

  for (Count index = 0; index < value.get_size(); index++) {
    U8 byte = value[index];
    Bool letter =
        Bool((byte >= 'a' && byte <= 'z') || (byte >= 'A' && byte <= 'Z'));
    Bool valid = Bool(
        letter || byte == '_' || (index != 0 && byte >= '0' && byte <= '9'));
    if (!valid) {
      return False;
    }
  }

  return True;
}

Tetrodotoxin::Library::Llvm::Symbol::Symbol(
    Memory::Allocator::Arena& arena,
    const Concept::Abstract& semantic,
    Kind kind,
    Unit unit) {
  Memory::Managed::Bytes output(arena);
  switch (kind) {
  case Kind::Path:
    break;
  case Kind::FunctionStatic:
  case Kind::FunctionSelf:
  case Kind::Construction:
    output.concat("TTX_FUNC_"_view);
    break;
  case Kind::Address:
    output.concat("TTX_ADDR_"_view);
    break;
  case Kind::OptionType:
    output.concat("ttx.option."_view);
    break;
  case Kind::ResultType:
    output.concat("ttx.result."_view);
    break;
  case Kind::StructureType:
    output.concat("ttx.struct."_view);
    break;
  case Kind::ObjectType:
    output.concat("ttx.object."_view);
    break;
  case Kind::ObjectFinalizer:
    output.concat("__ttx_object_finalize_"_view);
    break;
  case Kind::ObjectDescriptor:
    output.concat("__ttx_object_descriptor_"_view);
    break;
  }

  Count path_start = output.get_size();
  Bool published_identity = kind == Kind::FunctionStatic ||
                            kind == Kind::FunctionSelf ||
                            kind == Kind::Construction || kind == Kind::Address;
  if (published_identity && unit.is_package_member()) {
    append_encoded_name(output, unit.get_package());
    output.concat("__"_view);
    append_encoded_name(output, unit.get_member());
    output.concat("__"_view);
    path_start = output.get_size();
  }
  append_symbol_path(output, semantic, path_start);
  if (kind == Kind::FunctionStatic) {
    output.concat("_static"_view);
  } else if (kind == Kind::FunctionSelf) {
    output.concat("_self"_view);
  } else if (kind == Kind::Construction) {
    output.concat("__construct_static"_view);
  }

  value = output.get_view();
}
