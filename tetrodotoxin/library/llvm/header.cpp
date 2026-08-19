// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/llvm/header.hpp"

#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/diagnostics/log.hpp"

#include "perimortem/memory/managed/bytes.hpp"
#include "perimortem/memory/managed/map.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "perimortem/serialization/stream/textual.hpp"

#include "perimortem/abi/memory/dynamic/object.hpp"
#include "ttx/model/addressable.hpp"
#include "ttx/model/callable.hpp"
#include "ttx/model/type.hpp"

using namespace Perimortem;
using namespace Perimortem::Serialization;
using namespace Tetrodotoxin::Library;

using HeaderStream = Stream::Textual<Memory::Managed::Bytes>;
using HeaderTypes = Memory::Managed::Vector<const Ttx::Model::Type*>;
using HeaderTypeSet = Memory::Managed::Map<const Ttx::Model::Type*, Bool>;

static auto fail_header(Core::View::Bytes message) -> Bool {
  Core::Diagnostics::Log::error(message);
  return False;
}

static auto require_result_type(const Ttx::Concept::Layout& layout, Count index)
    -> Core::Option<const Ttx::Model::Type&> {
  auto entry = layout.get_abstract(index);
  return entry ? entry->resolve().select<Ttx::Model::Type>()
               : Core::Option<const Ttx::Model::Type&>();
}

static auto require_parameter(const Ttx::Concept::Layout& layout, Count index)
    -> Core::Option<const Ttx::Model::Addressable&> {
  auto entry = layout.get_abstract(index);
  return entry ? entry->select<Ttx::Model::Addressable>()
               : Core::Option<const Ttx::Model::Addressable&>();
}

static auto collect_type(
    HeaderTypes& ordered,
    HeaderTypeSet& collected,
    const Llvm::Carriers& carriers,
    const Ttx::Model::Type& type) -> Bool {
  if (collected.contains(&type)) {
    return True;
  }

  auto kind = carriers.get_kind(type);
  if (!kind) {
    return fail_header(
        "The C header cannot find a completed carrier for a published Type."_view);
  }

  collected.insert(&type, True);
  switch (*kind) {
  case Llvm::Carriers::Kind::Value:
  case Llvm::Carriers::Kind::Object:
    ordered.insert(&type);
    return True;

  case Llvm::Carriers::Kind::Structure: {
    auto fields = carriers.get_fields(type);
    if (!fields) {
      return fail_header(
          "The C header cannot find the completed Structure fields."_view);
    }

    for (Count index = 0; index < fields->get_size(); index++) {
      auto field = require_parameter(*fields, index);
      if (!field ||
          !collect_type(ordered, collected, carriers, field->get_type())) {
        return fail_header(
            "The C header found a Structure field without a completed carrier."_view);
      }
    }

    break;
  }

  case Llvm::Carriers::Kind::Result: {
    auto value = carriers.get_element(type);
    auto error = carriers.get_error(type);
    if (!value || !error ||
        !collect_type(ordered, collected, carriers, *value) ||
        !collect_type(ordered, collected, carriers, *error)) {
      return fail_header(
          "The C header found Result without both completed alternatives."_view);
    }

    break;
  }

  case Llvm::Carriers::Kind::Enumeration:
  case Llvm::Carriers::Kind::Fixed:
  case Llvm::Carriers::Kind::Option:
  case Llvm::Carriers::Kind::Range:
  case Llvm::Carriers::Kind::View:
  case Llvm::Carriers::Kind::Access: {
    auto element = carriers.get_element(type);
    if (!element || !collect_type(ordered, collected, carriers, *element)) {
      return fail_header(
          "The C header found a carrier without its completed element Type."_view);
    }

    break;
  }

  case Llvm::Carriers::Kind::Context:
    return fail_header(
        "The C header cannot publish a contextual Type as a C value."_view);
  }

  ordered.insert(&type);
  return True;
}

static auto collect_callable(
    HeaderTypes& ordered,
    HeaderTypeSet& collected,
    const Llvm::Carriers& carriers,
    const Ttx::Model::Callable& callable) -> Bool {
  const Ttx::Concept::Layout& results = callable.get_results();
  for (Count index = 0; index < results.get_size(); index++) {
    auto type = require_result_type(results, index);
    if (!type || !collect_type(ordered, collected, carriers, *type)) {
      return fail_header(
          "The C header found a Callable result without a completed carrier."_view);
    }
  }

  const Ttx::Concept::Layout& parameters = callable.get_parameters();
  for (Count index = 0; index < parameters.get_size(); index++) {
    auto parameter = require_parameter(parameters, index);
    if (!parameter ||
        !collect_type(ordered, collected, carriers, parameter->get_type())) {
      return fail_header(
          "The C header found a Callable parameter without a completed carrier."_view);
    }
  }

  return True;
}

static auto write_encoded_name(HeaderStream& output, Core::View::Bytes value)
    -> void {
  constexpr auto hex = "0123456789abcdef"_view;
  for (Count index = 0; index < value.get_size(); index++) {
    Unsigned_8 byte = value[index];
    Bool alphanumeric = Bool(
        (byte >= 'a' && byte <= 'z') || (byte >= 'A' && byte <= 'Z') ||
        (byte >= '0' && byte <= '9'));
    if (alphanumeric) {
      output << Core::View::Bytes(&byte, 1);
    } else {
      Core::Static::Vector<Unsigned_8, 3> encoded = {{
        '_',
        hex[byte >> 4],
        hex[byte & 15],
      }};
      output << Core::View::Bytes(encoded.get_data(), encoded.get_size());
    }
  }
}

static auto write_type_name(
    HeaderStream& output,
    const Llvm::Carriers& carriers,
    const Ttx::Model::Type& type) -> Bool {
  auto kind = carriers.get_kind(type);
  if (!kind) {
    return fail_header(
        "The C header cannot name a Type without its completed carrier."_view);
  }

  switch (*kind) {
  case Llvm::Carriers::Kind::Value: {
    auto width = carriers.get_width(type);
    if (!width) {
      return fail_header(
          "The C header cannot name a scalar without its physical width."_view);
    }

    if (*width == 1 && !carriers.is_real(type)) {
      output << "bool"_view;
    } else if (carriers.is_real(type) && *width == 32) {
      output << "float"_view;
    } else if (carriers.is_real(type) && *width == 64) {
      output << "double"_view;
    } else if (carriers.is_real(type)) {
      return fail_header(
          "The C header cannot name the selected Real carrier width."_view);
    } else if (carriers.is_signed(type)) {
      output << "int"_view << *width << "_t"_view;
    } else {
      output << "uint"_view << *width << "_t"_view;
    }

    return True;
  }

  case Llvm::Carriers::Kind::Enumeration: {
    auto storage = carriers.get_element(type);
    return storage && write_type_name(output, carriers, *storage);
  }

  case Llvm::Carriers::Kind::Context:
    return fail_header(
        "The C header cannot name a contextual Type as a C value."_view);

  case Llvm::Carriers::Kind::Fixed:
  case Llvm::Carriers::Kind::Option:
  case Llvm::Carriers::Kind::Result:
  case Llvm::Carriers::Kind::Range:
  case Llvm::Carriers::Kind::View:
  case Llvm::Carriers::Kind::Access:
  case Llvm::Carriers::Kind::Structure:
  case Llvm::Carriers::Kind::Object:
    output << "ttx_"_view;
    write_encoded_name(output, type.get_name());
    return True;
  }
}

static auto write_type_definition(
    HeaderStream& output,
    const Llvm::Carriers& carriers,
    const Ttx::Model::Type& type,
    Bool& uses_objects) -> Bool {
  auto kind = carriers.get_kind(type);
  if (!kind) {
    return fail_header(
        "The C header cannot define a Type without its completed carrier."_view);
  }

  switch (*kind) {
  case Llvm::Carriers::Kind::Value:
  case Llvm::Carriers::Kind::Enumeration:
    return True;

  case Llvm::Carriers::Kind::Object:
    output << "typedef struct ttx_"_view;
    write_encoded_name(output, type.get_name());
    output << "_object *ttx_"_view;
    write_encoded_name(output, type.get_name());
    output << ";\n\n"_view;
    uses_objects = True;
    return True;

  case Llvm::Carriers::Kind::Structure: {
    output << "typedef struct ttx_"_view;
    write_encoded_name(output, type.get_name());
    output << " {\n"_view;

    auto fields = carriers.get_fields(type);
    if (!fields) {
      return fail_header(
          "The C header cannot define a Structure without its fields."_view);
    }

    for (Count index = 0; index < fields->get_size(); index++) {
      auto field = require_parameter(*fields, index);
      auto name = fields->get_name(index);
      if (!field) {
        return fail_header(
            "The C header found a Structure field without an Addressable."_view);
      }

      output << "  "_view;
      if (!write_type_name(output, carriers, field->get_type())) {
        return False;
      }

      output << " "_view;
      write_encoded_name(output, name ? *name : field->get_name());
      output << ";\n"_view;
    }

    break;
  }

  case Llvm::Carriers::Kind::Fixed: {
    output << "typedef struct ttx_"_view;
    write_encoded_name(output, type.get_name());
    output << " {\n"_view;

    auto element = carriers.get_element(type);
    auto extent = carriers.get_extent(type);
    if (!element || !extent) {
      return fail_header(
          "The C header cannot define Fixed without its element and extent."_view);
    }

    output << "  "_view;
    if (!write_type_name(output, carriers, *element)) {
      return False;
    }

    output << " values["_view << *extent << "];\n"_view;
    break;
  }

  case Llvm::Carriers::Kind::View:
  case Llvm::Carriers::Kind::Access: {
    output << "typedef struct ttx_"_view;
    write_encoded_name(output, type.get_name());
    output << " {\n"_view;

    auto element = carriers.get_element(type);
    if (!element) {
      return fail_header(
          "The C header cannot define contiguous storage without its element."_view);
    }

    output << "  "_view;
    if (*kind == Llvm::Carriers::Kind::View) {
      output << "const "_view;
    }

    if (!write_type_name(output, carriers, *element)) {
      return False;
    }

    output << " *data;\n  uint64_t size;\n"_view;
    break;
  }

  case Llvm::Carriers::Kind::Range: {
    output << "typedef struct ttx_"_view;
    write_encoded_name(output, type.get_name());
    output << " {\n"_view;

    auto element = carriers.get_element(type);
    if (!element) {
      return fail_header(
          "The C header cannot define Range without its element."_view);
    }

    output << "  "_view;
    if (!write_type_name(output, carriers, *element)) {
      return False;
    }

    output << " start;\n  "_view;
    if (!write_type_name(output, carriers, *element)) {
      return False;
    }

    output << " end;\n"_view;
    break;
  }

  case Llvm::Carriers::Kind::Option: {
    output << "typedef struct ttx_"_view;
    write_encoded_name(output, type.get_name());
    output << " {\n"_view;

    auto element = carriers.get_element(type);
    if (!element) {
      return fail_header(
          "The C header cannot define Option without its element."_view);
    }

    output << "  "_view;
    if (!write_type_name(output, carriers, *element)) {
      return False;
    }

    output << " value;\n  bool set;\n"_view;
    break;
  }

  case Llvm::Carriers::Kind::Result: {
    output << "typedef struct ttx_"_view;
    write_encoded_name(output, type.get_name());
    output << " {\n  union {\n    "_view;

    auto value = carriers.get_element(type);
    auto error = carriers.get_error(type);
    if (!value || !error || !write_type_name(output, carriers, *value)) {
      return fail_header(
          "The C header cannot define Result without its value Type."_view);
    }

    output << " value;\n    "_view;
    if (!write_type_name(output, carriers, *error)) {
      return False;
    }

    output << " error;\n  };\n  bool value_selected;\n"_view;
    break;
  }

  case Llvm::Carriers::Kind::Context:
    return fail_header(
        "The C header found an unsupported physical carrier."_view);
  }

  output << "} ttx_"_view;
  write_encoded_name(output, type.get_name());
  output << ";\n\n"_view;
  return True;
}

static auto write_result_name(HeaderStream& output, Core::View::Bytes symbol)
    -> void {
  output << "ttx_results_"_view;
  write_encoded_name(output, symbol);
}

static auto write_result_definition(
    HeaderStream& output,
    const Llvm::Carriers& carriers,
    const Ttx::Model::Callable& callable,
    Core::View::Bytes symbol) -> Bool {
  const Ttx::Concept::Layout& results = callable.get_results();
  if (results.get_size() < 2) {
    return True;
  }

  output << "typedef struct "_view;
  write_result_name(output, symbol);
  output << " {\n"_view;
  for (Count index = 0; index < results.get_size(); index++) {
    auto type = require_result_type(results, index);
    if (!type) {
      return fail_header(
          "The C header found a result without an exact Type."_view);
    }

    output << "  "_view;
    if (!write_type_name(output, carriers, *type)) {
      return False;
    }

    output << " "_view;
    auto name = results.get_name(index);
    if (name) {
      write_encoded_name(output, *name);
    } else {
      output << "value"_view << index;
    }

    output << ";\n"_view;
  }

  output << "} "_view;
  write_result_name(output, symbol);
  output << ";\n\n"_view;
  return True;
}

static auto write_signature(
    HeaderStream& output,
    const Llvm::Carriers& carriers,
    const Ttx::Model::Callable& callable,
    Core::View::Bytes symbol) -> Bool {
  const Ttx::Concept::Layout& results = callable.get_results();
  if (results.is_empty()) {
    output << "void"_view;
  } else if (results.get_size() == 1) {
    auto result = require_result_type(results, 0);
    if (!result || !write_type_name(output, carriers, *result)) {
      return fail_header(
          "The C header found a result without an exact carrier."_view);
    }
  } else {
    write_result_name(output, symbol);
  }

  output << " "_view << symbol << "("_view;
  const Ttx::Concept::Layout& parameters = callable.get_parameters();
  if (parameters.is_empty()) {
    output << "void"_view;
  }

  for (Count index = 0; index < parameters.get_size(); index++) {
    if (index != 0) {
      output << ", "_view;
    }

    auto parameter = require_parameter(parameters, index);
    if (!parameter ||
        !write_type_name(output, carriers, parameter->get_type())) {
      return fail_header(
          "The C header found a parameter without an exact carrier."_view);
    }

    output << " "_view;
    auto name = parameters.get_name(index);
    if (name) {
      write_encoded_name(output, *name);
    } else {
      output << "value"_view << index;
    }
  }

  output << ");\n"_view;
  return True;
}

auto Llvm::Header::create(
    Memory::Allocator::Arena& arena,
    const Llvm::Carriers& carriers,
    const Llvm::Functions& functions,
    const Llvm::Globals& globals,
    Core::View::Vector<Llvm::Export> exports) -> Core::Option<Llvm::Header> {
  HeaderTypes ordered(arena);
  HeaderTypeSet collected(arena);

  for (const Llvm::Export& exported : exports) {
    if (!collect_callable(
            ordered, collected, carriers, exported.get_callable())) {
      return {};
    }
  }

  for (const Ttx::Concept::Reference<const Ttx::Model::Addressable>& retained :
       globals.get_foreign_addressables()) {
    const Ttx::Model::Addressable& addressable = retained.get();
    if (!collect_type(ordered, collected, carriers, addressable.get_type())) {
      return {};
    }
  }

  for (const Ttx::Concept::Reference<const Ttx::Model::Callable>& retained :
       functions.get_foreign_callables()) {
    const Ttx::Model::Callable& callable = retained.get();
    if (!collect_callable(ordered, collected, carriers, callable)) {
      return {};
    }
  }

  Memory::Managed::Bytes buffer(arena);
  HeaderStream output(buffer);
  output
      << "#pragma once\n\n#include <stdbool.h>\n#include <stdint.h>\n\n"_view;
  Bool uses_objects = False;
  for (const Ttx::Model::Type* type : ordered.get_view()) {
    if (!write_type_definition(output, carriers, *type, uses_objects)) {
      return {};
    }
  }

  for (const Llvm::Export& exported : exports) {
    if (!write_result_definition(
            output, carriers, exported.get_callable(), exported.get_symbol())) {
      return {};
    }
  }

  for (const Ttx::Concept::Reference<const Ttx::Model::Callable>& retained :
       functions.get_foreign_callables()) {
    const Ttx::Model::Callable& callable = retained.get();
    auto symbol = functions.find_symbol(callable);
    if (!symbol ||
        !write_result_definition(output, carriers, callable, *symbol)) {
      return {};
    }
  }

  output << "#ifdef __cplusplus\nextern \"C\" {\n#endif\n\n"_view;
  if (uses_objects) {
    output << "void "_view << Abi::Memory::Dynamic::Object::retain_symbol
           << "(void *value);\nvoid "_view
           << Abi::Memory::Dynamic::Object::release_symbol
           << "(void *value);\n"_view;
  }

  for (const Llvm::Export& exported : exports) {
    if (!write_signature(
            output, carriers, exported.get_callable(), exported.get_symbol())) {
      return {};
    }
  }

  for (const Ttx::Concept::Reference<const Ttx::Model::Addressable>& retained :
       globals.get_foreign_addressables()) {
    const Ttx::Model::Addressable& addressable = retained.get();
    auto symbol = globals.find_symbol(addressable);
    if (!symbol) {
      return {};
    }

    output << "extern "_view;
    if (!globals.permits_foreign_write(addressable)) {
      output << "const "_view;
    }

    if (!write_type_name(output, carriers, addressable.get_type())) {
      return {};
    }

    output << " "_view << *symbol << ";\n"_view;
  }

  for (const Ttx::Concept::Reference<const Ttx::Model::Callable>& retained :
       functions.get_foreign_callables()) {
    const Ttx::Model::Callable& callable = retained.get();
    auto symbol = functions.find_symbol(callable);
    if (!symbol || !write_signature(output, carriers, callable, *symbol)) {
      return {};
    }
  }

  output << "\n#ifdef __cplusplus\n}\n#endif\n"_view;
  return Llvm::Header(buffer.get_view());
}
