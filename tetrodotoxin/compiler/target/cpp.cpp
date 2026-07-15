// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/compiler/target/cpp.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"

#include "perimortem/serialization/stream/textual.hpp"

#include "tetrodotoxin/abi/type.hpp"
#include "ttx/member.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Serialization;
using namespace Tetrodotoxin;
using namespace Tetrodotoxin::Compiler;

static auto get_type_name(const Ttx::Type& type) -> View::Bytes {
  return type.get_name();
}

static auto supports(const Ttx::Type& type) -> Bool {
  switch (Abi::Lowering(type.resolve_attribute("abi"_view).get_unsigned())) {
  case Abi::Lowering::Void:
  case Abi::Lowering::Bool:
  case Abi::Lowering::Integer:
  case Abi::Lowering::Signed:
  case Abi::Lowering::Real:
  case Abi::Lowering::ViewBytes:
    return True;

  case Abi::Lowering::Invalid:
    return False;
  }

  return False;
}

static auto supports(Ttx::Layout layout) -> Bool {
  View::Vector<Ttx::Member> members = layout.get_members();
  for (Count i = 0; i < members.get_size(); i++) {
    if (!supports(members[i].get_type())) {
      return False;
    }
  }

  return True;
}

static auto uses_view_bytes(Ttx::Layout layout) -> Bool {
  View::Vector<Ttx::Member> members = layout.get_members();
  for (Count i = 0; i < members.get_size(); i++) {
    if (Abi::Lowering(
            members[i]
                .get_type()
                .resolve_attribute("abi"_view)
                .get_unsigned()) == Abi::Lowering::ViewBytes) {
      return True;
    }
  }

  return False;
}

static auto get_abi_result_name(const Abi::Export& export_) -> Dynamic::Bytes {
  Dynamic::Bytes name(export_.get_symbol());
  name.concat("_result"_view);
  return name;
}

static auto get_cpp_result_name(const Ttx::Function& function)
    -> Dynamic::Bytes {
  Dynamic::Bytes name;
  View::Bytes source = function.get_name();
  Bool uppercase = True;
  for (Count i = 0; i < source.get_size(); i++) {
    Bits_8 character = source[i];
    if (character == '_') {
      uppercase = True;
      continue;
    }

    if (uppercase && character >= 'a' && character <= 'z') {
      character = Bits_8(character - 'a' + 'A');
    }

    name.append(character);
    uppercase = False;
  }

  name.concat("Result"_view);
  return name;
}

static auto write_namespace(
    Stream::Textual<Dynamic::Bytes>& output,
    View::Bytes path,
    View::Bytes surface = View::Bytes()) -> void {
  output << "namespace Ttx"_view;

  Count start = 0;
  for (Count i = 0; i <= path.get_size(); i++) {
    if (i != path.get_size() && path[i] != '.') {
      continue;
    }

    output << "::"_view << path.slice(start, i - start);
    start = i + 1;
  }

  if (!surface.is_empty()) {
    output << "::"_view << surface;
  }

  output << " {\n\n"_view;
}

static auto close_namespace(
    Stream::Textual<Dynamic::Bytes>& output,
    View::Bytes path,
    View::Bytes surface = View::Bytes()) -> void {
  output << "}  // namespace Ttx"_view;

  Count start = 0;
  for (Count i = 0; i <= path.get_size(); i++) {
    if (i != path.get_size() && path[i] != '.') {
      continue;
    }

    output << "::"_view << path.slice(start, i - start);
    start = i + 1;
  }

  if (!surface.is_empty()) {
    output << "::"_view << surface;
  }

  output << "\n"_view;
}

static auto write_member_name(
    Stream::Textual<Dynamic::Bytes>& output,
    const Ttx::Member& member,
    Count index) -> void {
  View::Bytes name = member.get_name();
  output << (name.is_empty() ? "value"_view : name);
  if (name.is_empty()) {
    output << index;
  }
}

static auto write_abi_type(
    Stream::Textual<Dynamic::Bytes>& output,
    const Ttx::Type& type,
    Bool qualified = False) -> void {
  switch (Abi::Lowering(type.resolve_attribute("abi"_view).get_unsigned())) {
  case Abi::Lowering::ViewBytes:
    if (qualified) {
      output << "Ttx::Abi::"_view;
    }

    output << "ViewBytes"_view;
    return;

  case Abi::Lowering::Bool:
  case Abi::Lowering::Integer:
  case Abi::Lowering::Signed:
    output << "Bits_64"_view;
    return;

  case Abi::Lowering::Real:
    output << "Real_64"_view;
    return;

  case Abi::Lowering::Void:
  case Abi::Lowering::Invalid:
    output << "void"_view;
    return;
  }

  output << "void"_view;
}

static auto write_cpp_parameters(
    Stream::Textual<Dynamic::Bytes>& output,
    const Ttx::Function& function) -> void {
  View::Vector<Ttx::Member> parameters =
      function.get_parameters().get_members();
  for (Count i = 0; i < parameters.get_size(); i++) {
    if (i != 0) {
      output << ", "_view;
    }

    output << get_type_name(parameters[i].get_type()) << " "_view;
    write_member_name(output, parameters[i], i);
  }
}

static auto write_abi_parameters(
    Stream::Textual<Dynamic::Bytes>& output,
    const Ttx::Function& function) -> void {
  View::Vector<Ttx::Member> parameters =
      function.get_parameters().get_members();
  for (Count i = 0; i < parameters.get_size(); i++) {
    if (i != 0) {
      output << ", "_view;
    }

    write_abi_type(output, parameters[i].get_type());
    output << " "_view;
    write_member_name(output, parameters[i], i);
  }
}

static auto write_argument(
    Stream::Textual<Dynamic::Bytes>& output,
    const Ttx::Member& parameter,
    Count index) -> void {
  const Ttx::Type& type = parameter.get_type();
  switch (Abi::Lowering(type.resolve_attribute("abi"_view).get_unsigned())) {
  case Abi::Lowering::ViewBytes:
    output << "Ttx::Abi::ViewBytes{"_view;
    write_member_name(output, parameter, index);
    output << ".get_data(), "_view;
    write_member_name(output, parameter, index);
    output << ".get_size()}"_view;
    return;

  case Abi::Lowering::Bool:
    output << "Bits_64("_view;
    write_member_name(output, parameter, index);
    output << ".value"_view;
    break;

  case Abi::Lowering::Integer:
  case Abi::Lowering::Signed:
    output << "Bits_64("_view;
    write_member_name(output, parameter, index);
    break;

  case Abi::Lowering::Real:
    output << "Real_64("_view;
    write_member_name(output, parameter, index);
    break;

  case Abi::Lowering::Void:
  case Abi::Lowering::Invalid:
    return;
  }

  output << ")"_view;
}

static auto write_arguments(
    Stream::Textual<Dynamic::Bytes>& output,
    const Ttx::Function& function) -> void {
  View::Vector<Ttx::Member> parameters =
      function.get_parameters().get_members();
  for (Count i = 0; i < parameters.get_size(); i++) {
    if (i != 0) {
      output << ", "_view;
    }

    write_argument(output, parameters[i], i);
  }
}

static auto write_abi_return_type(
    Stream::Textual<Dynamic::Bytes>& output,
    const Abi::Export& export_,
    Bool qualified = False) -> void {
  View::Vector<Ttx::Member> results =
      export_.get_function().get_result().get_members();
  if (results.get_size() > 1) {
    if (qualified) {
      output << "Ttx::Abi::"_view;
    }

    output << get_abi_result_name(export_);
    return;
  }

  if (results.get_size() == 1 &&
      Abi::Lowering(
          results[0].get_type().resolve_attribute("abi"_view).get_unsigned()) !=
          Abi::Lowering::Void) {
    write_abi_type(output, results[0].get_type(), qualified);
    return;
  }

  output << "void"_view;
}

static auto write_cpp_return_type(
    Stream::Textual<Dynamic::Bytes>& output,
    const Ttx::Function& function) -> void {
  View::Vector<Ttx::Member> results = function.get_result().get_members();
  if (results.get_size() > 1) {
    output << get_cpp_result_name(function);
  } else if (
      results.get_size() == 1 &&
      Abi::Lowering(
          results[0].get_type().resolve_attribute("abi"_view).get_unsigned()) !=
          Abi::Lowering::Void) {
    output << get_type_name(results[0].get_type());
  } else {
    output << "void"_view;
  }
}

static auto write_abi_result(
    Stream::Textual<Dynamic::Bytes>& output,
    const Abi::Export& export_) -> void {
  View::Vector<Ttx::Member> results =
      export_.get_function().get_result().get_members();
  if (results.get_size() <= 1) {
    return;
  }

  output << "struct "_view << get_abi_result_name(export_) << " {\n"_view;
  for (Count i = 0; i < results.get_size(); i++) {
    output << "  "_view;
    write_abi_type(output, results[i].get_type());
    output << " "_view;
    write_member_name(output, results[i], i);
    output << ";\n"_view;
  }

  output << "};\n\n"_view;
}

static auto write_cpp_result(
    Stream::Textual<Dynamic::Bytes>& output,
    const Ttx::Function& function) -> void {
  View::Vector<Ttx::Member> results = function.get_result().get_members();
  if (results.get_size() <= 1) {
    return;
  }

  output << "struct "_view << get_cpp_result_name(function) << " {\n"_view;
  for (Count i = 0; i < results.get_size(); i++) {
    output << "  "_view << get_type_name(results[i].get_type()) << " "_view;
    write_member_name(output, results[i], i);
    output << ";\n"_view;
  }

  output << "};\n\n"_view;
}

static auto write_raw_call(
    Stream::Textual<Dynamic::Bytes>& output,
    const Abi::Export& export_) -> void {
  output << "Ttx::Abi::"_view << export_.get_symbol() << "("_view;
  write_arguments(output, export_.get_function());
  output << ")"_view;
}

static auto write_result_value(
    Stream::Textual<Dynamic::Bytes>& output,
    const Ttx::Member& result,
    Count index) -> void {
  const Ttx::Type& type = result.get_type();
  output << get_type_name(type) << "(result."_view;
  write_member_name(output, result, index);
  Abi::Lowering lowering =
      Abi::Lowering(type.resolve_attribute("abi"_view).get_unsigned());
  if (lowering == Abi::Lowering::ViewBytes) {
    output << ".data, result."_view;
    write_member_name(output, result, index);
    output << ".size"_view;
  } else if (lowering == Abi::Lowering::Bool) {
    output << " != 0"_view;
  }

  output << ")"_view;
}

static auto write_export(
    Stream::Textual<Dynamic::Bytes>& output,
    const Abi::Export& export_) -> void {
  const Ttx::Function& function = export_.get_function();
  if (!supports(function.get_parameters()) ||
      !supports(function.get_result())) {
    output << "// `"_view << function.get_name()
           << "` is public in TTX, but at least one parameter or result "
              "type\n"
              "// has no C++ representation for this ABI target.\n\n"_view;
    return;
  }

  write_cpp_result(output, function);
  output << "inline auto "_view << function.get_name() << "("_view;
  write_cpp_parameters(output, function);
  output << ") -> "_view;
  write_cpp_return_type(output, function);
  output << " {\n"_view;

  View::Vector<Ttx::Member> results = function.get_result().get_members();
  if (results.is_empty() || (results.get_size() == 1 &&
                             Abi::Lowering(
                                 results[0]
                                     .get_type()
                                     .resolve_attribute("abi"_view)
                                     .get_unsigned()) == Abi::Lowering::Void)) {
    output << "  "_view;
    write_raw_call(output, export_);
    output << ";\n}\n\n"_view;
    return;
  }

  if (results.get_size() > 1) {
    output << "  "_view;
    write_abi_return_type(output, export_, True);
    output << " result = "_view;
    write_raw_call(output, export_);
    output << ";\n  return {\n"_view;
    for (Count i = 0; i < results.get_size(); i++) {
      output << "    "_view;
      write_result_value(output, results[i], i);
      output << ",\n"_view;
    }

    output << "  };\n}\n\n"_view;
    return;
  }

  const Ttx::Type& result = results[0].get_type();
  Abi::Lowering lowering =
      Abi::Lowering(result.resolve_attribute("abi"_view).get_unsigned());
  if (lowering == Abi::Lowering::ViewBytes) {
    output << "  Ttx::Abi::ViewBytes result = "_view;
    write_raw_call(output, export_);
    output << ";\n  return "_view << get_type_name(result)
           << "(result.data, result.size);\n}\n\n"_view;
    return;
  }

  output << "  return "_view << get_type_name(result) << "("_view;
  write_raw_call(output, export_);
  if (lowering == Abi::Lowering::Bool) {
    output << " != 0"_view;
  }

  output << ");\n}\n\n"_view;
}

auto Target::Cpp::build_header(const Program& program) -> Dynamic::Bytes {
  Dynamic::Bytes header;
  Stream::Textual<Dynamic::Bytes> output(header);
  output << "#pragma once\n\n"
            "#include \"perimortem/core/perimortem.hpp\"\n"
            "#include \"perimortem/core/view/bytes.hpp\"\n\n"
            "// This C++ header is generated from the public TTX interface. "
            "The raw\n"
            "// declarations use C linkage and plain ABI carriers. The inline "
            "functions\n"
            "// marshal those values into the authored C++ types and "
            "namespaces.\n"
            "// Addressable receivers remain explicit.\n\n"
            "using Void = void;\n\n"_view;

  View::Vector<Abi::Export> exports = program.get_exports();
  Bool has_declarations = False;
  Bool has_view_bytes = False;
  for (Count i = 0; i < exports.get_size(); i++) {
    const Ttx::Function& function = exports[i].get_function();
    if (!supports(function.get_parameters()) ||
        !supports(function.get_result())) {
      continue;
    }

    has_declarations = True;
    if (uses_view_bytes(function.get_parameters()) ||
        uses_view_bytes(function.get_result())) {
      has_view_bytes = True;
    }
  }

  if (has_declarations) {
    output << "namespace Ttx::Abi {\n\n"_view;
  }

  if (has_view_bytes) {
    output << "// The machine ABI carries a byte view as two integer "
              "components. The\n"
              "// public wrapper reconstructs View::Bytes after the call, so "
              "its C++ class\n"
              "// representation never becomes part of the C-linkage "
              "declaration.\n"
              "struct ViewBytes {\n"
              "  const Bits_8* data;\n"
              "  Count size;\n"
              "};\n\n"_view;
  }

  for (Count i = 0; i < exports.get_size(); i++) {
    Bool declared = False;
    for (Count k = 0; k < i; k++) {
      if (exports[k].get_symbol() == exports[i].get_symbol()) {
        declared = True;
        break;
      }
    }

    if (declared) {
      continue;
    }

    const Ttx::Function& function = exports[i].get_function();
    if (!supports(function.get_parameters()) ||
        !supports(function.get_result())) {
      continue;
    }

    write_abi_result(output, exports[i]);
    output << "extern \"C\" "_view;
    write_abi_return_type(output, exports[i]);
    output << " "_view << exports[i].get_symbol() << "("_view;
    write_abi_parameters(output, function);
    output << ");\n"_view;
  }

  if (has_declarations) {
    output << "\n}  // namespace Ttx::Abi\n\n"_view;
  }

  View::Vector<Abi::Type> types = program.get_types();
  for (Count i = 0; i < types.get_size(); i++) {
    write_namespace(output, types[i].get_path());
    output << "using Representation = "_view
           << get_type_name(types[i].get_type()) << ";\n"_view;
    output << "\n"_view;
    close_namespace(output, types[i].get_path());
    output << "\n"_view;
  }

  for (Count i = 0; i < exports.get_size();) {
    write_namespace(output, exports[i].get_path());

    Count end = i;
    while (end < exports.get_size() &&
           exports[end].get_path() == exports[i].get_path()) {
      write_export(output, exports[end]);
      end++;
    }

    close_namespace(output, exports[i].get_path());
    output << "\n"_view;
    i = end;
  }

  return header;
}
