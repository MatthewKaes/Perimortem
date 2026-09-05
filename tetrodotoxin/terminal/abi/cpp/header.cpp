// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/terminal/abi/cpp/header.hpp"

#include "perimortem/core/diagnostics/log.hpp"

#include "perimortem/memory/managed/bytes.hpp"

#include "perimortem/serialization/stream/textual.hpp"

#include "tetrodotoxin/library/language/function.hpp"
#include "tetrodotoxin/library/language/model/callable.hpp"
#include "tetrodotoxin/library/language/types/structure.hpp"
#include "tetrodotoxin/terminal/abi/publication.hpp"
#include "tetrodotoxin/terminal/abi/representation/type_name.hpp"
#include "ttx/ffi/cpp/addressable.hpp"

using namespace Perimortem;
using namespace Perimortem::Serialization;

static auto fail_cxx(Core::View::Bytes message) -> Bool {
  Core::Diagnostics::Log::error(message);
  return False;
}

static auto write_cpp_path(
    Stream::Textual<Memory::Managed::Bytes>& output,
    Core::View::Bytes path,
    U8 separator) -> void {
  Count start = 0;
  for (Count index = 0; index <= path.get_size(); index++) {
    Bool end = index == path.get_size();
    Bool selected = !end && path[index] == separator;
    Bool route_separator = separator == ':' && selected &&
                           index + 1 < path.get_size() &&
                           path[index + 1] == ':';
    if (!end && !route_separator && !(separator != ':' && selected)) {
      continue;
    }
    if (start != 0) {
      output << "::"_view;
    }
    output << path.slice(start, index - start);
    if (route_separator) {
      index++;
    }
    start = index + 1;
  }
}

static auto route_leaf(Core::View::Bytes route) -> Core::View::Bytes {
  Count start = 0;
  for (Count index = 0; index + 1 < route.get_size(); index++) {
    if (route[index] == ':' && route[index + 1] == ':') {
      start = index + 2;
      index++;
    }
  }
  return route.slice(start);
}

static auto route_parent(Core::View::Bytes route) -> Core::View::Bytes {
  Count end = 0;
  for (Count index = 0; index + 1 < route.get_size(); index++) {
    if (route[index] == ':' && route[index + 1] == ':') {
      end = index;
      index++;
    }
  }
  return end == 0 ? Core::View::Bytes() : route.slice(0, end);
}

static auto path_leaf(Core::View::Bytes path) -> Core::View::Bytes {
  Count start = 0;
  for (Count index = 0; index < path.get_size(); index++) {
    if (path[index] == '/') {
      start = index + 1;
    }
  }
  return path.slice(start);
}

static auto find_binding(
    const Tetrodotoxin::Terminal::Abi::Unit& unit,
    const Ttx::Model::Domain& type)
    -> Core::Option<const Tetrodotoxin::Terminal::Abi::Unit::TypeBinding&> {
  return unit.find_type(type);
}

static auto write_raw_type_name(
    Memory::Allocator::Arena& arena,
    Stream::Textual<Memory::Managed::Bytes>& output,
    const Tetrodotoxin::Terminal::Abi::Representation::Type& types,
    const Tetrodotoxin::Terminal::Abi::Unit& unit,
    const Ttx::Model::Domain& type,
    Core::View::Bytes inherited_member = {}) -> Bool {
  auto kind = types.get_kind(type);
  if (!kind) {
    return fail_cxx(
        "The C++ API cannot find one completed native carrier."_view);
  }

  if (*kind == Tetrodotoxin::Terminal::Abi::Representation::Type::Kind::Value) {
    auto width = types.get_width(type);
    if (!width) {
      return False;
    }
    if (*width == 1 && !types.is_real(type)) {
      output << "bool"_view;
    } else if (types.is_real(type) && *width == 32) {
      output << "float"_view;
    } else if (types.is_real(type) && *width == 64) {
      output << "double"_view;
    } else if (types.is_signed(type)) {
      output << "int"_view << *width << "_t"_view;
    } else {
      output << "uint"_view << *width << "_t"_view;
    }
    return True;
  }

  if (*kind ==
      Tetrodotoxin::Terminal::Abi::Representation::Type::Kind::Enumeration) {
    auto storage = types.get_element(type);
    return storage && write_raw_type_name(arena, output, types, unit, *storage);
  }
  if (*kind ==
      Tetrodotoxin::Terminal::Abi::Representation::Type::Kind::Implementation) {
    output << "struct perimortem_implementation"_view;
    return True;
  }
  if (*kind ==
      Tetrodotoxin::Terminal::Abi::Representation::Type::Kind::Option) {
    auto element = types.get_element(type);
    if (element && types.is_object(*element)) {
      return write_raw_type_name(arena, output, types, unit, *element);
    }
  }

  auto name = Tetrodotoxin::Terminal::Abi::Representation::TypeName::create(
      arena, unit, type, unit.get_package(),
      inherited_member.is_empty() ? unit.get_member() : inherited_member);
  if (!name) {
    return False;
  }
  output << "struct "_view << name->get_value();
  if (*kind ==
          Tetrodotoxin::Terminal::Abi::Representation::Type::Kind::Object ||
      *kind == Tetrodotoxin::Terminal::Abi::Representation::Type::Kind::
                   ObjectStorage) {
    output << "_object*"_view;
  }
  return True;
}

static auto require_type(const Ttx::Concept::Abstract& answer)
    -> Core::Option<const Ttx::Model::Domain&> {
  auto direct = answer.select<Ttx::Model::Domain>();
  return direct ? direct : answer.resolve().select<Ttx::Model::Domain>();
}

static auto require_kind(
    const Tetrodotoxin::Terminal::Abi::Representation::Type& types,
    const Ttx::Concept::Abstract& answer)
    -> Core::Option<Tetrodotoxin::Terminal::Abi::Representation::Type::Kind> {
  auto type = require_type(answer);
  return type ? types.get_kind(*type)
              : Core::Option<
                    Tetrodotoxin::Terminal::Abi::Representation::Type::Kind>();
}

static auto require_element(
    const Tetrodotoxin::Terminal::Abi::Representation::Type& types,
    const Ttx::Concept::Abstract& answer)
    -> Core::Option<const Tetrodotoxin::Library::Language::Model::Type&> {
  auto type = require_type(answer);
  return type ? types.get_element(*type)
              : Core::Option<
                    const Tetrodotoxin::Library::Language::Model::Type&>();
}

static auto write_cpp_type(
    Stream::Textual<Memory::Managed::Bytes>& output,
    const Tetrodotoxin::Terminal::Abi::Representation::Type& types,
    const Tetrodotoxin::Terminal::Abi::Unit& unit,
    const Ttx::Concept::Abstract& answer) -> Bool {
  auto selected = require_type(answer);
  BAIL_IF(!selected);
  const Ttx::Model::Domain& type = *selected;
  auto kind = types.get_kind(type);
  if (!kind) {
    return False;
  }

  if (*kind == Tetrodotoxin::Terminal::Abi::Representation::Type::Kind::Value) {
    auto width = types.get_width(type);
    if (!width) {
      return False;
    }
    if (types.is_flag(type)) {
      output << "bool"_view;
    } else if (types.is_real(type)) {
      if (*width == 32) {
        output << "float"_view;
      } else if (*width == 64) {
        output << "double"_view;
      } else {
        return False;
      }
    } else if (types.is_signed(type)) {
      output << "std::int"_view << *width << "_t"_view;
    } else {
      output << "std::uint"_view << *width << "_t"_view;
    }
    return True;
  }

  if (*kind ==
      Tetrodotoxin::Terminal::Abi::Representation::Type::Kind::Enumeration) {
    auto storage = types.get_element(type);
    return storage && write_cpp_type(output, types, unit, *storage);
  }

  if (*kind == Tetrodotoxin::Terminal::Abi::Representation::Type::Kind::View) {
    auto element = types.get_element(type);
    if (!element) {
      return False;
    }
    auto width = types.get_width(*element);
    if (width && *width == 8 && !types.is_real(*element) &&
        !types.is_signed(*element)) {
      output << "std::span<const std::uint8_t>"_view;
      return True;
    }
    output << "std::span<const "_view;
    if (!write_cpp_type(output, types, unit, *element)) {
      return False;
    }
    output << ">"_view;
    return True;
  }

  if (*kind ==
      Tetrodotoxin::Terminal::Abi::Representation::Type::Kind::Access) {
    auto element = types.get_element(type);
    if (!element) {
      return False;
    }
    auto width = types.get_width(*element);
    if (width && *width == 8 && !types.is_real(*element) &&
        !types.is_signed(*element)) {
      output << "std::span<std::uint8_t>"_view;
      return True;
    }
    output << "std::span<"_view;
    if (!write_cpp_type(output, types, unit, *element)) {
      return False;
    }
    output << ">"_view;
    return True;
  }

  if (*kind ==
      Tetrodotoxin::Terminal::Abi::Representation::Type::Kind::Implementation) {
    output << "Tetrodotoxin::Implementation"_view;
    return True;
  }

  if (*kind ==
      Tetrodotoxin::Terminal::Abi::Representation::Type::Kind::ObjectStorage) {
    output << "std::uint8_t*"_view;
    return True;
  }

  if (*kind ==
      Tetrodotoxin::Terminal::Abi::Representation::Type::Kind::Object) {
    auto binding = find_binding(unit, type);
    if (!binding) {
      return False;
    }
    output << "::"_view;
    write_cpp_path(output, binding->get_package(), '.');
    output << "::"_view;
    write_cpp_path(output, binding->get_route(), ':');
    return True;
  }

  if (*kind !=
      Tetrodotoxin::Terminal::Abi::Representation::Type::Kind::Structure) {
    return fail_cxx(
        "The first generated C++ API supports scalar, View, Implementation, "
        "Object, and "
        "Structure types."_view);
  }

  auto binding = find_binding(unit, type);
  if (!binding) {
    return False;
  }
  output << "::"_view;
  write_cpp_path(output, binding->get_package(), '.');
  output << "::"_view;
  write_cpp_path(output, binding->get_route(), ':');
  return True;
}

static auto write_access_for_view(
    Stream::Textual<Memory::Managed::Bytes>& output,
    const Tetrodotoxin::Terminal::Abi::Representation::Type& types,
    const Tetrodotoxin::Terminal::Abi::Unit& unit,
    const Ttx::Model::Domain& view) -> Bool {
  auto element = types.get_element(view);
  if (!element) {
    return False;
  }
  auto width = types.get_width(*element);
  if (width && *width == 8 && !types.is_real(*element) &&
      !types.is_signed(*element)) {
    output << "std::span<std::uint8_t>"_view;
    return True;
  }
  output << "std::span<"_view;
  if (!write_cpp_type(output, types, unit, *element)) {
    return False;
  }
  output << ">"_view;
  return True;
}

static auto require_parameter(const Ttx::Concept::Layout& layout, Count index)
    -> Core::Option<const Ttx::Model::Addressable&> {
  auto entry = layout.get_abstract(index);
  return entry ? entry->select<Ttx::Model::Addressable>()
               : Core::Option<const Ttx::Model::Addressable&>();
}

static auto require_result_type(const Ttx::Concept::Layout& layout, Count index)
    -> Core::Option<const Ttx::Model::Domain&> {
  auto entry = layout.get_abstract(index);
  auto addressable = entry ? entry->select<Ttx::Model::Addressable>()
                           : Core::Option<const Ttx::Model::Addressable&>();
  return addressable ? require_type(addressable->get_domain())
         : entry     ? require_type(*entry)
                     : Core::Option<const Ttx::Model::Domain&>();
}

static auto has_prefix(Core::View::Bytes value, Core::View::Bytes prefix)
    -> Bool {
  return value.get_size() >= prefix.get_size() &&
         value.slice(0, prefix.get_size()) == prefix;
}

static auto explicit_parameter_start(
    const Tetrodotoxin::Library::Language::Function& function) -> Count {
  return function.declares_self() ? 1 : 0;
}

static auto explicit_parameter_count(
    const Tetrodotoxin::Library::Language::Function& function) -> Count {
  return function.get_parameters().get_size() -
         explicit_parameter_start(function);
}

static auto single_parameter_type(
    const Tetrodotoxin::Library::Language::Function& function)
    -> Core::Option<const Ttx::Model::Domain&> {
  if (explicit_parameter_count(function) != 1) {
    return {};
  }
  auto parameter = require_parameter(
      function.get_parameters(), explicit_parameter_start(function));
  return parameter ? require_type(parameter->get_domain())
                   : Core::Option<const Ttx::Model::Domain&>();
}

static auto single_result_type(
    const Tetrodotoxin::Library::Language::Function& function)
    -> Core::Option<const Ttx::Model::Domain&> {
  return function.get_results().get_size() == 1
             ? require_result_type(function.get_results(), 0)
             : Core::Option<const Ttx::Model::Domain&>();
}

static auto is_factory(
    const Tetrodotoxin::Library::Language::Function& function,
    const Ttx::Model::Domain& host) -> Bool {
  auto result = single_result_type(function);
  return Bool(
      !function.declares_self() && single_parameter_type(function) && result &&
      &*result == &host);
}

// Observer spellings carry the const promise expected by a native C++ value.
// TTX keeps one Self receiver shape, so projection records that promise at the
// language boundary without changing Function identity.
static auto is_const_projection(
    const Tetrodotoxin::Library::Language::Function& function) -> Bool {
  Core::View::Bytes name = function.get_name();
  return Bool(
      function.declares_self() &&
      ((has_prefix(name, "get_"_view) && name != "get_access"_view) ||
       has_prefix(name, "is_"_view) || name == "at"_view ||
       name == "slice"_view || name == "hash"_view || name == "equals"_view));
}

static auto find_function(
    Core::View::Vector<Tetrodotoxin::Terminal::Abi::Export> exports,
    const Ttx::Model::Domain& host,
    Core::View::Bytes name,
    Bool self)
    -> Core::Option<const Tetrodotoxin::Library::Language::Function&> {
  for (const Tetrodotoxin::Terminal::Abi::Export& exported : exports) {
    auto function = exported.get_callable()
                        .select<Tetrodotoxin::Library::Language::Function>();
    if (function && &function->get_host() == &host &&
        function->get_name() == name && function->declares_self() == self &&
        Tetrodotoxin::Terminal::Abi::is_publicly_reachable(
            function->get_definition())) {
      return *function;
    }
  }
  return {};
}

static auto returns_contiguous(
    const Tetrodotoxin::Terminal::Abi::Representation::Type& types,
    const Tetrodotoxin::Library::Language::Function& function,
    Tetrodotoxin::Terminal::Abi::Representation::Type::Kind kind)
    -> Core::Option<const Ttx::Model::Domain&> {
  auto result = single_result_type(function);
  auto selected =
      result ? types.get_kind(*result)
             : Core::Option<
                   Tetrodotoxin::Terminal::Abi::Representation::Type::Kind>();
  return selected && *selected == kind
             ? Core::Option<const Ttx::Model::Domain&>(*result)
             : Core::Option<const Ttx::Model::Domain&>();
}

static auto find_storage_field(
    const Tetrodotoxin::Terminal::Abi::Representation::Type& types,
    const Ttx::Model::Domain& host,
    const Ttx::Model::Domain& element)
    -> Core::Option<const Ttx::Model::Addressable&> {
  auto fields = types.get_fields(host);
  if (!fields) {
    return {};
  }
  for (Count index = 0; index < fields->get_size(); index++) {
    auto field = require_parameter(*fields, index);
    Core::Option<Tetrodotoxin::Terminal::Abi::Representation::Type::Kind> kind;
    Core::Option<const Tetrodotoxin::Library::Language::Model::Type&> selected;
    if (field) {
      kind = require_kind(types, field->get_domain());
      selected = require_element(types, field->get_domain());
    }
    if (field && kind &&
        *kind == Tetrodotoxin::Terminal::Abi::Representation::Type::Kind::
                     ObjectStorage &&
        selected && &*selected == &element) {
      return *field;
    }
  }
  return {};
}

static auto write_documentation(
    Stream::Textual<Memory::Managed::Bytes>& output,
    const Ttx::Concept::Documentation& documentation,
    Core::View::Bytes indentation) -> void {
  for (Count index = 0; index < documentation.line_count(); index++) {
    output << indentation << "//"_view;
    Core::View::Bytes line = documentation.get_line(index);
    if (!line.is_empty()) {
      output << " "_view << line;
    }
    output << "\n"_view;
  }
}

static auto write_parameters(
    Stream::Textual<Memory::Managed::Bytes>& output,
    const Tetrodotoxin::Terminal::Abi::Representation::Type& types,
    const Tetrodotoxin::Terminal::Abi::Unit& unit,
    const Ttx::Model::Callable& callable) -> Bool {
  const Ttx::Concept::Layout& parameters = callable.get_parameters();
  Count start =
      callable.is<Tetrodotoxin::Library::Language::Model::Callable>() &&
              static_cast<
                  const Tetrodotoxin::Library::Language::Model::Callable&>(
                  callable)
                  .declares_self()
          ? 1
          : 0;
  for (Count index = start; index < parameters.get_size(); index++) {
    if (index != start) {
      output << ", "_view;
    }
    auto parameter = require_parameter(parameters, index);
    if (!parameter ||
        !write_cpp_type(output, types, unit, parameter->get_domain())) {
      return False;
    }
    output << " "_view;
    auto name = parameters.get_name(index);
    output << (name ? *name : parameter->get_name());
  }
  return True;
}

static auto write_declaration_parameters(
    Stream::Textual<Memory::Managed::Bytes>& output,
    const Tetrodotoxin::Terminal::Abi::Representation::Type& types,
    const Tetrodotoxin::Terminal::Abi::Unit& unit,
    const Ttx::Model::Callable& callable) -> Bool {
  const Ttx::Concept::Layout& parameters = callable.get_parameters();
  auto library =
      callable.select<Tetrodotoxin::Library::Language::Model::Callable>();
  Count start = library && library->declares_self() ? 1 : 0;
  if (parameters.get_size() - start < 2) {
    return write_parameters(output, types, unit, callable);
  }

  output << "\n"_view;
  for (Count index = start; index < parameters.get_size(); index++) {
    output << "      "_view;
    auto parameter = require_parameter(parameters, index);
    if (!parameter ||
        !write_cpp_type(output, types, unit, parameter->get_domain())) {
      return False;
    }
    output << " "_view;
    auto name = parameters.get_name(index);
    output << (name ? *name : parameter->get_name());
    if (index + 1 != parameters.get_size()) {
      output << ",\n"_view;
    }
  }
  return True;
}

static auto write_return_type(
    Stream::Textual<Memory::Managed::Bytes>& output,
    const Tetrodotoxin::Terminal::Abi::Representation::Type& types,
    const Tetrodotoxin::Terminal::Abi::Unit& unit,
    const Tetrodotoxin::Library::Language::Function& function) -> Bool {
  if (function.get_self_result()) {
    output << route_leaf(function.get_host().get_name()) << "&"_view;
    return True;
  }
  const Ttx::Concept::Layout& results = function.get_results();
  if (results.is_empty()) {
    output << "void"_view;
    return True;
  }
  if (results.get_size() != 1) {
    return fail_cxx(
        "The first generated C++ API requires zero or one Callable result."_view);
  }
  auto type = require_result_type(results, 0);
  if (type && &*type == &function.get_host()) {
    output << function.get_host().get_name();
    return True;
  }
  return type && write_cpp_type(output, types, unit, *type);
}

static auto has_object_fields(
    const Tetrodotoxin::Terminal::Abi::Representation::Type& types,
    const Ttx::Model::Domain& type) -> Bool {
  auto fields = types.get_fields(type);
  if (!fields) {
    return False;
  }
  for (Count index = 0; index < fields->get_size(); index++) {
    auto field = require_parameter(*fields, index);
    auto kind =
        field ? require_kind(types, field->get_domain())
              : Core::Option<
                    Tetrodotoxin::Terminal::Abi::Representation::Type::Kind>();
    if (kind &&
        (*kind ==
             Tetrodotoxin::Terminal::Abi::Representation::Type::Kind::Object ||
         *kind == Tetrodotoxin::Terminal::Abi::Representation::Type::Kind::
                      ObjectStorage)) {
      return True;
    }
  }
  return False;
}

static auto write_factory_declarations(
    Stream::Textual<Memory::Managed::Bytes>& output,
    const Tetrodotoxin::Terminal::Abi::Representation::Type& types,
    const Tetrodotoxin::Terminal::Abi::Unit& unit,
    const Ttx::Model::Domain& host,
    Core::View::Bytes class_name,
    Core::View::Vector<Tetrodotoxin::Terminal::Abi::Export> exports) -> Bool {
  // A one input Static factory already describes value conversion. Constructors
  // and assignment make that meaning natural to C++ while the named factory
  // remains available to callers that prefer it.
  for (const Tetrodotoxin::Terminal::Abi::Export& exported : exports) {
    auto function = exported.get_callable()
                        .select<Tetrodotoxin::Library::Language::Function>();
    if (!function || &function->get_host() != &host ||
        !Tetrodotoxin::Terminal::Abi::is_publicly_reachable(
            function->get_definition()) ||
        !is_factory(*function, host)) {
      continue;
    }
    auto parameter = single_parameter_type(*function);
    if (!parameter) {
      return False;
    }
    output << "  "_view << class_name << "("_view;
    if (!write_cpp_type(output, types, unit, *parameter)) {
      return False;
    }
    output << " value);\n  auto operator=("_view;
    if (!write_cpp_type(output, types, unit, *parameter)) {
      return False;
    }
    output << " value) -> "_view << class_name << "&;\n"_view;
  }
  return True;
}

static auto write_projection_declarations(
    Stream::Textual<Memory::Managed::Bytes>& output,
    const Tetrodotoxin::Terminal::Abi::Representation::Type& types,
    const Tetrodotoxin::Terminal::Abi::Unit& unit,
    const Ttx::Model::Domain& host,
    Core::View::Bytes class_name,
    Core::View::Vector<Tetrodotoxin::Terminal::Abi::Export> exports) -> Bool {
  auto get_view = find_function(exports, host, "get_view"_view, True);
  auto view =
      get_view && explicit_parameter_count(*get_view) == 0
          ? returns_contiguous(
                types, *get_view,
                Tetrodotoxin::Terminal::Abi::Representation::Type::Kind::View)
          : Core::Option<const Ttx::Model::Domain&>();
  auto at = find_function(exports, host, "at"_view, True);
  auto at_parameter =
      at ? single_parameter_type(*at) : Core::Option<const Ttx::Model::Domain&>();
  auto at_result =
      at ? single_result_type(*at) : Core::Option<const Ttx::Model::Domain&>();
  if (at_parameter && at_result) {
    output << "  auto operator[]("_view;
    if (!write_cpp_type(output, types, unit, *at_parameter)) {
      return False;
    }
    output << " index) const -> "_view;
    if (!write_cpp_type(output, types, unit, *at_result)) {
      return False;
    }
    output << " { return at(index); }\n"_view;
  }

  auto append = find_function(exports, host, "append"_view, True);
  // A trailing count describes repetition, so the one value overload chooses
  // the ordinary single element case without inventing another TTX Callable.
  if (append && explicit_parameter_count(*append) == 2) {
    Count start = explicit_parameter_start(*append);
    auto value = require_parameter(append->get_parameters(), start);
    auto count = require_parameter(append->get_parameters(), start + 1);
    auto count_name = append->get_parameters().get_name(start + 1);
    if (value && count && count_name && *count_name == "count"_view) {
      output << "  auto append("_view;
      if (!write_cpp_type(output, types, unit, value->get_domain())) {
        return False;
      }
      output << " value) -> void { append(value, "_view;
      if (!write_cpp_type(output, types, unit, count->get_domain())) {
        return False;
      }
      output << "(1)); }\n"_view;
    }
  }

  auto reserve = find_function(exports, host, "reserve"_view, True);
  auto reserve_parameter = reserve ? single_parameter_type(*reserve)
                                   : Core::Option<const Ttx::Model::Domain&>();
  if (reserve_parameter) {
    output << "  auto ensure_capacity("_view;
    if (!write_cpp_type(output, types, unit, *reserve_parameter)) {
      return False;
    }
    output << " required_size) -> void { reserve(required_size); }\n"_view;
  }

  auto detach = find_function(exports, host, "detach"_view, True);
  auto get_size = find_function(exports, host, "get_size"_view, True);
  Core::Option<const Tetrodotoxin::Library::Language::Model::Type&> element;
  Core::Option<const Ttx::Model::Addressable&> storage;
  if (view) {
    element = types.get_element(*view);
  }
  if (element) {
    storage = find_storage_field(types, host, *element);
  }
  // A detachable Object buffer and its logical size form the familiar writable
  // C++ view. Detachment still crosses the generated TTX Function before the
  // private carrier address becomes observable.
  if (view && detach && get_size && storage &&
      explicit_parameter_count(*detach) == 0 &&
      explicit_parameter_count(*get_size) == 0) {
    output << "  auto get_access() -> "_view;
    if (!write_access_for_view(output, types, unit, *view)) {
      return False;
    }
    output << " {\n"
              "    detach();\n"
              "    return "_view;
    if (!write_access_for_view(output, types, unit, *view)) {
      return False;
    }
    output << "(reinterpret_cast<"_view;
    if (!write_cpp_type(output, types, unit, *element)) {
      return False;
    }
    output << " *>("_view << storage->get_name()
           << "), std::size_t(get_size()));\n"
              "  }\n"
              "  operator "_view;
    if (!write_access_for_view(output, types, unit, *view)) {
      return False;
    }
    output << "() { return get_access(); }\n"_view;
  }
  return True;
}

static auto write_namespace_declaration(
    Stream::Textual<Memory::Managed::Bytes>& output,
    const Tetrodotoxin::Terminal::Abi::Representation::Type& types,
    const Tetrodotoxin::Terminal::Abi::Unit& unit,
    const Tetrodotoxin::Terminal::Abi::Unit::TypeBinding& binding,
    Core::View::Vector<Tetrodotoxin::Terminal::Abi::Export> exports) -> Bool {
  if (binding.get_semantic()
          .is<Tetrodotoxin::Library::Language::Types::Structure>()) {
    return True;
  }

  Bool opened = False;
  for (const Tetrodotoxin::Terminal::Abi::Export& exported : exports) {
    auto function = exported.get_callable()
                        .select<Tetrodotoxin::Library::Language::Function>();
    if (!function ||
        &function->get_definition().get_host() != &binding.get_semantic() ||
        !Tetrodotoxin::Terminal::Abi::is_publicly_reachable(
            function->get_definition())) {
      continue;
    }

    if (!opened) {
      output << "namespace "_view;
      write_cpp_path(output, binding.get_package(), '.');
      output << "::"_view;
      write_cpp_path(output, binding.get_route(), ':');
      output << " {\n\n"_view;
      opened = True;
    }

    write_documentation(output, function->get_documentation(), {});
    output << "auto "_view << function->get_name() << "("_view;
    if (!write_declaration_parameters(output, types, unit, *function)) {
      return False;
    }
    output << ") -> "_view;
    if (!write_return_type(output, types, unit, *function)) {
      return False;
    }
    output << ";\n"_view;
  }

  if (opened) {
    output << "\n}  // namespace "_view;
    write_cpp_path(output, binding.get_package(), '.');
    output << "::"_view;
    write_cpp_path(output, binding.get_route(), ':');
    output << "\n\n"_view;
  }

  return True;
}

static auto write_class_declaration(
    Stream::Textual<Memory::Managed::Bytes>& output,
    const Tetrodotoxin::Terminal::Abi::Representation::Type& types,
    const Tetrodotoxin::Terminal::Abi::Unit& unit,
    const Tetrodotoxin::Terminal::Abi::Unit::TypeBinding& binding,
    Core::View::Vector<Tetrodotoxin::Terminal::Abi::Export> exports) -> Bool {
  auto structure =
      binding.get_semantic()
          .select<Tetrodotoxin::Library::Language::Types::Structure>();
  if (!structure || !Tetrodotoxin::Terminal::Abi::is_publicly_reachable(
                        structure->get_definition())) {
    return True;
  }

  Core::View::Bytes parent = route_parent(binding.get_route());
  output << "namespace "_view;
  write_cpp_path(output, binding.get_package(), '.');
  if (!parent.is_empty()) {
    output << "::"_view;
    write_cpp_path(output, parent, ':');
  }
  output << " {\n\n"_view;
  write_documentation(output, structure->get_documentation(), {});
  Core::View::Bytes class_name = route_leaf(binding.get_route());
  output << "class "_view << class_name << " {\n public:\n"_view;
  Bool object = types.is_object(*structure);
  Bool object_fields = has_object_fields(types, *structure);
  output << "  "_view << class_name
         << (object ? "();\n"_view : "() = default;\n"_view);
  if (object || object_fields) {
    output << "  "_view << class_name << "(const "_view << class_name
           << "& other);\n  "_view << class_name << "("_view << class_name
           << "&& other) noexcept;\n  auto operator=(const "_view << class_name
           << "& other) -> "_view << class_name << "&;\n  auto operator=("_view
           << class_name << "&& other) noexcept -> "_view << class_name
           << "&;\n  ~"_view << class_name << "();\n\n"_view;
  }
  if (!object) {
    if (!write_factory_declarations(
            output, types, unit, *structure, class_name, exports)) {
      return False;
    }

    output << "\n"_view;
  }

  for (const Tetrodotoxin::Terminal::Abi::Export& exported : exports) {
    auto function = exported.get_callable()
                        .select<Tetrodotoxin::Library::Language::Function>();
    if (!function || &function->get_definition().get_host() != &*structure ||
        !Tetrodotoxin::Terminal::Abi::is_publicly_reachable(
            function->get_definition())) {
      continue;
    }
    write_documentation(output, function->get_documentation(), "  "_view);
    output << "  "_view;
    if (!function->declares_self()) {
      output << "static "_view;
    }
    output << "auto "_view << function->get_name() << "("_view;
    if (!write_declaration_parameters(output, types, unit, *function)) {
      return False;
    }
    output << ")"_view;
    if (is_const_projection(*function)) {
      output << " const"_view;
    }
    output << " -> "_view;
    if (!write_return_type(output, types, unit, *function)) {
      return False;
    }
    output << ";\n"_view;
  }

  if (!write_projection_declarations(
          output, types, unit, *structure, class_name, exports)) {
    return False;
  }

  if (object) {
    output << "\n  static auto adopt(std::uint8_t* selected) -> "_view
           << class_name << " { return "_view << class_name
           << "(selected); }\n"
              "  auto get_object() const -> std::uint8_t* { return object; }\n"
              "\n private:\n  explicit "_view
           << class_name
           << "(std::uint8_t* selected) : object(selected) {}\n"
              "\n  std::uint8_t* object = nullptr;\n"
              "};\n\n}  // namespace "_view;
    write_cpp_path(output, binding.get_package(), '.');
    if (!parent.is_empty()) {
      output << "::"_view;
      write_cpp_path(output, parent, ':');
    }
    output << "\n\n"_view;
    return True;
  }

  output << "\n private:\n  "_view << class_name << "("_view;
  auto fields = types.get_fields(*structure);
  if (!fields) {
    return False;
  }
  for (Count index = 0; index < fields->get_size(); index++) {
    if (index != 0) {
      output << ", "_view;
    }
    auto field = require_parameter(*fields, index);
    if (!field || !write_cpp_type(output, types, unit, field->get_domain())) {
      return False;
    }
    output << " selected_"_view << field->get_name();
  }
  output << ");\n\n"_view;
  for (Count index = 0; index < fields->get_size(); index++) {
    auto field = require_parameter(*fields, index);
    if (!field) {
      return False;
    }
    output << "  "_view;
    if (!write_cpp_type(output, types, unit, field->get_domain())) {
      return False;
    }
    output << " "_view << field->get_name() << "{};\n"_view;
  }
  output << "};\n\n}  // namespace "_view;
  write_cpp_path(output, binding.get_package(), '.');
  if (!parent.is_empty()) {
    output << "::"_view;
    write_cpp_path(output, parent, ':');
  }
  output << "\n\n"_view;
  return True;
}

static auto write_raw_argument(
    Memory::Allocator::Arena& arena,
    Stream::Textual<Memory::Managed::Bytes>& output,
    const Tetrodotoxin::Terminal::Abi::Representation::Type& types,
    const Tetrodotoxin::Terminal::Abi::Unit& unit,
    const Ttx::Concept::Abstract& answer,
    Core::View::Bytes name) -> Bool {
  auto type = require_type(answer);
  BAIL_IF(!type);
  auto kind = types.get_kind(*type);
  if (!kind) {
    return False;
  }
  if (*kind == Tetrodotoxin::Terminal::Abi::Representation::Type::Kind::Value ||
      *kind == Tetrodotoxin::Terminal::Abi::Representation::Type::Kind::
                   Enumeration) {
    output << name;
    return True;
  }
  if (*kind == Tetrodotoxin::Terminal::Abi::Representation::Type::Kind::View) {
    output << "{"_view << name << ".data(), "_view << name << ".size()}"_view;
    return True;
  }
  if (*kind ==
      Tetrodotoxin::Terminal::Abi::Representation::Type::Kind::Access) {
    output << "{"_view << name << ".data(), "_view << name << ".size()}"_view;
    return True;
  }
  if (*kind ==
      Tetrodotoxin::Terminal::Abi::Representation::Type::Kind::Implementation) {
    output << "{"_view << name << ".get_object(), "_view << name
           << ".get_projection()}"_view;
    return True;
  }
  if (*kind ==
      Tetrodotoxin::Terminal::Abi::Representation::Type::Kind::Object) {
    output << "reinterpret_cast<"_view;
    if (!write_raw_type_name(arena, output, types, unit, *type)) {
      return False;
    }
    output << ">("_view << name << ".get_object())"_view;
    return True;
  }
  if (*kind ==
      Tetrodotoxin::Terminal::Abi::Representation::Type::Kind::Structure) {
    output << "*reinterpret_cast<const "_view;
    if (!write_raw_type_name(arena, output, types, unit, *type)) {
      return False;
    }
    output << " *>(&"_view << name << ")"_view;
    return True;
  }
  return False;
}

static auto write_call_arguments(
    Memory::Allocator::Arena& arena,
    Stream::Textual<Memory::Managed::Bytes>& output,
    const Tetrodotoxin::Terminal::Abi::Representation::Type& types,
    const Tetrodotoxin::Terminal::Abi::Unit& unit,
    const Tetrodotoxin::Library::Language::Function& function) -> Bool {
  const Ttx::Concept::Layout& parameters = function.get_parameters();
  Count start = function.declares_self() ? 1 : 0;
  if (start != 0) {
    if (types.is_object(function.get_host())) {
      output << "reinterpret_cast<"_view;
      if (!write_raw_type_name(
              arena, output, types, unit, function.get_host())) {
        return False;
      }
      output << ">(object)"_view;
    } else if (is_const_projection(function)) {
      output << "const_cast<"_view;
      if (!write_raw_type_name(
              arena, output, types, unit, function.get_host())) {
        return False;
      }
      output << " *>(reinterpret_cast<const "_view;
      if (!write_raw_type_name(
              arena, output, types, unit, function.get_host())) {
        return False;
      }
      output << " *>(this))"_view;
    } else {
      output << "reinterpret_cast<"_view;
      if (!write_raw_type_name(
              arena, output, types, unit, function.get_host())) {
        return False;
      }
      output << " *>(this)"_view;
    }
  }
  for (Count index = start; index < parameters.get_size(); index++) {
    if (index != start || start != 0) {
      output << ", "_view;
    }
    auto parameter = require_parameter(parameters, index);
    auto name = parameters.get_name(index);
    if (!parameter || !write_raw_argument(
                          arena, output, types, unit, parameter->get_domain(),
                          name ? *name : parameter->get_name())) {
      return False;
    }
  }
  return True;
}

static auto write_adopted_result(
    Stream::Textual<Memory::Managed::Bytes>& output,
    const Tetrodotoxin::Terminal::Abi::Representation::Type& types,
    const Tetrodotoxin::Terminal::Abi::Unit& unit,
    const Ttx::Model::Domain& type,
    Core::View::Bytes variable) -> Bool {
  auto kind = types.get_kind(type);
  if (!kind) {
    return False;
  }
  if (*kind == Tetrodotoxin::Terminal::Abi::Representation::Type::Kind::Value ||
      *kind == Tetrodotoxin::Terminal::Abi::Representation::Type::Kind::
                   Enumeration) {
    output << "return "_view << variable << ";\n"_view;
    return True;
  }
  if (*kind == Tetrodotoxin::Terminal::Abi::Representation::Type::Kind::View) {
    output << "return "_view;
    if (!write_cpp_type(output, types, unit, type)) {
      return False;
    }
    output << "("_view << variable << ".data, "_view << variable
           << ".size);\n"_view;
    return True;
  }
  if (*kind ==
      Tetrodotoxin::Terminal::Abi::Representation::Type::Kind::Implementation) {
    output << "return Tetrodotoxin::Implementation::adopt("
              "static_cast<std::uint8_t *>("_view
           << variable << ".object), "_view << variable
           << ".projection);\n"_view;
    return True;
  }
  if (*kind ==
      Tetrodotoxin::Terminal::Abi::Representation::Type::Kind::Object) {
    output << "return "_view;
    if (!write_cpp_type(output, types, unit, type)) {
      return False;
    }
    output << "::adopt(reinterpret_cast<std::uint8_t *>("_view << variable
           << "));\n"_view;
    return True;
  }
  if (*kind !=
      Tetrodotoxin::Terminal::Abi::Representation::Type::Kind::Structure) {
    return False;
  }
  auto fields = types.get_fields(type);
  if (!fields) {
    return False;
  }
  output << "return "_view;
  if (!write_cpp_type(output, types, unit, type)) {
    return False;
  }
  output << "("_view;
  for (Count index = 0; index < fields->get_size(); index++) {
    if (index != 0) {
      output << ", "_view;
    }
    auto field = require_parameter(*fields, index);
    if (!field) {
      return False;
    }
    output << variable << "."_view << field->get_name();
  }
  output << ");\n"_view;
  return True;
}

static auto write_method_definition(
    Memory::Allocator::Arena& arena,
    Stream::Textual<Memory::Managed::Bytes>& output,
    const Tetrodotoxin::Terminal::Abi::Representation::Type& types,
    const Tetrodotoxin::Terminal::Abi::Unit& unit,
    const Tetrodotoxin::Terminal::Abi::Unit::TypeBinding& binding,
    const Tetrodotoxin::Terminal::Abi::Export& exported) -> Bool {
  auto function = exported.get_callable()
                      .select<Tetrodotoxin::Library::Language::Function>();
  if (!function ||
      &function->get_definition().get_host() != &binding.get_semantic() ||
      !Tetrodotoxin::Terminal::Abi::is_publicly_reachable(
          function->get_definition())) {
    return True;
  }

  output << "auto "_view;
  write_cpp_path(output, binding.get_package(), '.');
  output << "::"_view;
  write_cpp_path(output, binding.get_route(), ':');
  output << "::"_view << function->get_name() << "("_view;
  if (!write_parameters(output, types, unit, *function)) {
    return False;
  }
  output << ")"_view;
  if (is_const_projection(*function)) {
    output << " const"_view;
  }
  output << " -> "_view;
  if (!write_return_type(output, types, unit, *function)) {
    return False;
  }
  output << " {\n  "_view;

  const Ttx::Concept::Layout& results = function->get_results();
  if (function->get_self_result()) {
    output << exported.get_symbol() << "("_view;
    if (!write_call_arguments(arena, output, types, unit, *function)) {
      return False;
    }
    output << ");\n  return *this;\n}\n\n"_view;
    return True;
  }
  if (results.is_empty()) {
    output << exported.get_symbol() << "("_view;
    if (!write_call_arguments(arena, output, types, unit, *function)) {
      return False;
    }
    output << ");\n}\n\n"_view;
    return True;
  }

  auto result = require_result_type(results, 0);
  if (!result) {
    return False;
  }
  auto kind = types.get_kind(*result);
  if (!kind) {
    return False;
  }
  if (*kind == Tetrodotoxin::Terminal::Abi::Representation::Type::Kind::Value ||
      *kind == Tetrodotoxin::Terminal::Abi::Representation::Type::Kind::
                   Enumeration) {
    output << "return "_view << exported.get_symbol() << "("_view;
    if (!write_call_arguments(arena, output, types, unit, *function)) {
      return False;
    }
    output << ");\n}\n\n"_view;
    return True;
  }

  output << "auto result = "_view << exported.get_symbol() << "("_view;
  if (!write_call_arguments(arena, output, types, unit, *function)) {
    return False;
  }
  output << ");\n  "_view;
  if (!write_adopted_result(output, types, unit, *result, "result"_view)) {
    return False;
  }
  output << "}\n\n"_view;
  return True;
}

static auto write_factory_definitions(
    Stream::Textual<Memory::Managed::Bytes>& output,
    const Tetrodotoxin::Terminal::Abi::Representation::Type& types,
    const Tetrodotoxin::Terminal::Abi::Unit& unit,
    const Tetrodotoxin::Terminal::Abi::Unit::TypeBinding& binding,
    Core::View::Vector<Tetrodotoxin::Terminal::Abi::Export> exports) -> Bool {
  if (types.is_object(binding.get_semantic())) {
    return True;
  }

  Core::View::Bytes class_name = route_leaf(binding.get_route());
  for (const Tetrodotoxin::Terminal::Abi::Export& exported : exports) {
    auto function = exported.get_callable()
                        .select<Tetrodotoxin::Library::Language::Function>();
    if (!function || &function->get_host() != &binding.get_semantic() ||
        !Tetrodotoxin::Terminal::Abi::is_publicly_reachable(
            function->get_definition()) ||
        !is_factory(*function, binding.get_semantic())) {
      continue;
    }
    auto parameter = single_parameter_type(*function);
    if (!parameter) {
      return False;
    }

    write_cpp_path(output, binding.get_package(), '.');
    output << "::"_view;
    write_cpp_path(output, binding.get_route(), ':');
    output << "::"_view << class_name << "("_view;
    if (!write_cpp_type(output, types, unit, *parameter)) {
      return False;
    }
    output << " value) : "_view << class_name << "("_view
           << function->get_name() << "(value)) {}\n\n"_view;

    output << "auto "_view;
    write_cpp_path(output, binding.get_package(), '.');
    output << "::"_view;
    write_cpp_path(output, binding.get_route(), ':');
    output << "::operator=("_view;
    if (!write_cpp_type(output, types, unit, *parameter)) {
      return False;
    }
    output << " value) -> "_view << class_name << "& {\n  return *this = "_view
           << function->get_name() << "(value);\n}\n\n"_view;
  }
  return True;
}

static auto find_publication(
    Core::View::Vector<Tetrodotoxin::Terminal::Abi::Publication> publications,
    const Ttx::Concept::Abstract& semantic) -> Core::Option<Core::View::Bytes> {
  for (const Tetrodotoxin::Terminal::Abi::Publication& publication :
       publications) {
    if (&publication.get_semantic() == &semantic) {
      return publication.get_symbol();
    }
  }

  return {};
}

static auto write_construction_declarations(
    Memory::Allocator::Arena& arena,
    Stream::Textual<Memory::Managed::Bytes>& output,
    const Tetrodotoxin::Terminal::Abi::Representation::Type& types,
    const Tetrodotoxin::Terminal::Abi::Unit& unit,
    Core::View::Vector<Tetrodotoxin::Terminal::Abi::Publication> publications)
    -> Bool {
  Bool wrote = False;
  for (const Tetrodotoxin::Terminal::Abi::Publication& publication :
       publications) {
    auto type = publication.get_semantic().select<Ttx::Model::Domain>();
    if (!type || !types.is_object(*type) || !unit.find_type(*type)) {
      continue;
    }

    auto fields = types.get_fields(*type);
    if (!fields) {
      return False;
    }

    output << "extern \"C\" "_view;
    wrote = True;
    if (!write_raw_type_name(arena, output, types, unit, *type)) {
      return False;
    }
    output << " "_view << publication.get_symbol() << "("_view;
    for (Count index = 0; index < fields->get_size(); ++index) {
      if (index != 0) {
        output << ", "_view;
      }

      auto field = require_parameter(*fields, index);
      auto field_type = field ? require_type(field->get_domain())
                              : Core::Option<const Ttx::Model::Domain&>();
      if (!field || !field_type ||
          !write_raw_type_name(arena, output, types, unit, *field_type)) {
        return False;
      }
      output << " value"_view << index << ", bool value"_view << index
             << "_set"_view;
    }
    if (fields->is_empty()) {
      output << "void"_view;
    }
    output << ");\nextern \"C\" "_view;
    if (!write_raw_type_name(arena, output, types, unit, *type)) {
      return False;
    }
    output << " "_view << publication.get_symbol()
           << "__default(void) {\n  return "_view << publication.get_symbol()
           << "("_view;
    for (Count index = 0; index < fields->get_size(); ++index) {
      if (index != 0) {
        output << ", "_view;
      }
      output << "{}, false"_view;
    }
    output << ");\n}\n"_view;
  }

  if (wrote) {
    output << "\n"_view;
  }
  return True;
}

static auto write_lifecycle_definitions(
    Memory::Allocator::Arena& arena,
    Stream::Textual<Memory::Managed::Bytes>& output,
    const Tetrodotoxin::Terminal::Abi::Representation::Type& types,
    const Tetrodotoxin::Terminal::Abi::Unit& unit,
    const Tetrodotoxin::Terminal::Abi::Unit::TypeBinding& binding,
    Core::View::Vector<Tetrodotoxin::Terminal::Abi::Publication> publications)
    -> Bool {
  const Ttx::Model::Domain& type = binding.get_semantic();
  auto structure =
      type.select<Tetrodotoxin::Library::Language::Types::Structure>();
  if (!structure || !Tetrodotoxin::Terminal::Abi::is_publicly_reachable(
                        structure->get_definition())) {
    return True;
  }
  auto fields = types.get_fields(type);
  if (!fields) {
    return False;
  }

  output << "static_assert(sizeof("_view;
  write_cpp_path(output, binding.get_package(), '.');
  output << "::"_view;
  write_cpp_path(output, binding.get_route(), ':');
  output << ") == sizeof("_view;
  if (!write_raw_type_name(arena, output, types, unit, type)) {
    return False;
  }
  output << "));\n\n"_view;

  output << "static_assert(alignof("_view;
  write_cpp_path(output, binding.get_package(), '.');
  output << "::"_view;
  write_cpp_path(output, binding.get_route(), ':');
  output << ") == alignof("_view;
  if (!write_raw_type_name(arena, output, types, unit, type)) {
    return False;
  }
  output << "));\n\n"_view;

  if (types.is_object(type)) {
    Core::View::Bytes qualified_package = binding.get_package();
    Core::View::Bytes qualified_route = binding.get_route();
    Core::View::Bytes class_name = route_leaf(qualified_route);
    auto construction = find_publication(publications, type);
    if (!construction) {
      return False;
    }

    write_cpp_path(output, qualified_package, '.');
    output << "::"_view;
    write_cpp_path(output, qualified_route, ':');
    output << "::"_view << class_name << "() {\n  object = "_view
           << "reinterpret_cast<std::uint8_t *>("_view << *construction
           << "__default());\n}\n\n"_view;

    write_cpp_path(output, qualified_package, '.');
    output << "::"_view;
    write_cpp_path(output, qualified_route, ':');
    output << "::"_view << class_name << "(const "_view;
    write_cpp_path(output, qualified_package, '.');
    output << "::"_view;
    write_cpp_path(output, qualified_route, ':');
    output << "& other) : object(other.object) {\n"
              "  perimortem_core_object_retain(object);\n"
              "}\n\n"_view;

    write_cpp_path(output, qualified_package, '.');
    output << "::"_view;
    write_cpp_path(output, qualified_route, ':');
    output << "::"_view << class_name << "("_view;
    write_cpp_path(output, qualified_package, '.');
    output << "::"_view;
    write_cpp_path(output, qualified_route, ':');
    output << "&& other) noexcept : object(other.object) {\n"
              "  other.object = nullptr;\n"
              "}\n\n"_view;

    output << "auto "_view;
    write_cpp_path(output, qualified_package, '.');
    output << "::"_view;
    write_cpp_path(output, qualified_route, ':');
    output << "::operator=(const "_view;
    write_cpp_path(output, qualified_package, '.');
    output << "::"_view;
    write_cpp_path(output, qualified_route, ':');
    output << "& other) -> "_view << class_name
           << "& {\n"
              "  if (this == &other) {\n"
              "    return *this;\n"
              "  }\n\n"
              "  perimortem_core_object_retain(other.object);\n"
              "  perimortem_core_object_release(object);\n"
              "  object = other.object;\n"
              "  return *this;\n"
              "}\n\n"_view;

    output << "auto "_view;
    write_cpp_path(output, qualified_package, '.');
    output << "::"_view;
    write_cpp_path(output, qualified_route, ':');
    output << "::operator=("_view;
    write_cpp_path(output, qualified_package, '.');
    output << "::"_view;
    write_cpp_path(output, qualified_route, ':');
    output << "&& other) noexcept -> "_view << class_name
           << "& {\n"
              "  if (this == &other) {\n"
              "    return *this;\n"
              "  }\n\n"
              "  perimortem_core_object_release(object);\n"
              "  object = other.object;\n"
              "  other.object = nullptr;\n"
              "  return *this;\n"
              "}\n\n"_view;

    write_cpp_path(output, qualified_package, '.');
    output << "::"_view;
    write_cpp_path(output, qualified_route, ':');
    output << "::~"_view << class_name
           << "() { perimortem_core_object_release(object); }\n\n"_view;
    return True;
  }

  output << ""_view;
  write_cpp_path(output, binding.get_package(), '.');
  output << "::"_view;
  write_cpp_path(output, binding.get_route(), ':');
  output << "::"_view << route_leaf(binding.get_route()) << "("_view;
  for (Count index = 0; index < fields->get_size(); index++) {
    if (index != 0) {
      output << ", "_view;
    }
    auto field = require_parameter(*fields, index);
    if (!field || !write_cpp_type(output, types, unit, field->get_domain())) {
      return False;
    }
    output << " selected_"_view << field->get_name();
  }
  output << ")"_view;
  if (!fields->is_empty()) {
    output << "\n    : "_view;
    for (Count index = 0; index < fields->get_size(); index++) {
      if (index != 0) {
        output << ",\n      "_view;
      }
      auto field = require_parameter(*fields, index);
      output << field->get_name() << "(selected_"_view << field->get_name()
             << ")"_view;
    }
  }
  output << " {}\n\n"_view;

  if (!has_object_fields(types, type)) {
    return True;
  }

  Core::View::Bytes qualified_package = binding.get_package();
  Core::View::Bytes qualified_route = binding.get_route();
  output << ""_view;
  write_cpp_path(output, qualified_package, '.');
  output << "::"_view;
  write_cpp_path(output, qualified_route, ':');
  output << "::"_view << route_leaf(qualified_route) << "(const "_view;
  write_cpp_path(output, qualified_package, '.');
  output << "::"_view;
  write_cpp_path(output, qualified_route, ':');
  output << "& other)"_view;
  if (!fields->is_empty()) {
    output << "\n    : "_view;
    for (Count index = 0; index < fields->get_size(); index++) {
      if (index != 0) {
        output << ", "_view;
      }
      auto field = require_parameter(*fields, index);
      output << field->get_name() << "(other."_view << field->get_name()
             << ")"_view;
    }
  }
  output << " {\n"_view;
  for (Count index = 0; index < fields->get_size(); index++) {
    auto field = require_parameter(*fields, index);
    auto kind =
        field ? require_kind(types, field->get_domain())
              : Core::Option<
                    Tetrodotoxin::Terminal::Abi::Representation::Type::Kind>();
    if (kind &&
        (*kind ==
             Tetrodotoxin::Terminal::Abi::Representation::Type::Kind::Object ||
         *kind == Tetrodotoxin::Terminal::Abi::Representation::Type::Kind::
                      ObjectStorage)) {
      output << "  perimortem_core_object_retain("_view << field->get_name()
             << ");\n"_view;
    }
  }
  output << "}\n\n"_view;

  output << ""_view;
  write_cpp_path(output, qualified_package, '.');
  output << "::"_view;
  write_cpp_path(output, qualified_route, ':');
  output << "::"_view << route_leaf(qualified_route) << "("_view;
  write_cpp_path(output, qualified_package, '.');
  output << "::"_view;
  write_cpp_path(output, qualified_route, ':');
  output << "&& other) noexcept"_view;
  if (!fields->is_empty()) {
    output << "\n    : "_view;
    for (Count index = 0; index < fields->get_size(); index++) {
      if (index != 0) {
        output << ", "_view;
      }
      auto field = require_parameter(*fields, index);
      output << field->get_name() << "(other."_view << field->get_name()
             << ")"_view;
    }
  }
  output << " {\n"_view;
  for (Count index = 0; index < fields->get_size(); index++) {
    auto field = require_parameter(*fields, index);
    auto kind =
        field ? require_kind(types, field->get_domain())
              : Core::Option<
                    Tetrodotoxin::Terminal::Abi::Representation::Type::Kind>();
    if (kind &&
        (*kind ==
             Tetrodotoxin::Terminal::Abi::Representation::Type::Kind::Object ||
         *kind == Tetrodotoxin::Terminal::Abi::Representation::Type::Kind::
                      ObjectStorage)) {
      output << "  other."_view << field->get_name() << " = nullptr;\n"_view;
    } else {
      output << "  other."_view << field->get_name() << " = {};\n"_view;
    }
  }
  output << "}\n\n"_view;

  output << "auto "_view;
  write_cpp_path(output, qualified_package, '.');
  output << "::"_view;
  write_cpp_path(output, qualified_route, ':');
  output << "::operator=(const "_view;
  write_cpp_path(output, qualified_package, '.');
  output << "::"_view;
  write_cpp_path(output, qualified_route, ':');
  output << "& other) -> "_view << route_leaf(qualified_route) << "& {\n"_view
         << "  if (this == &other) {\n    return *this;\n  }\n"_view;
  for (Count index = 0; index < fields->get_size(); index++) {
    auto field = require_parameter(*fields, index);
    auto kind =
        field ? require_kind(types, field->get_domain())
              : Core::Option<
                    Tetrodotoxin::Terminal::Abi::Representation::Type::Kind>();
    if (kind &&
        (*kind ==
             Tetrodotoxin::Terminal::Abi::Representation::Type::Kind::Object ||
         *kind == Tetrodotoxin::Terminal::Abi::Representation::Type::Kind::
                      ObjectStorage)) {
      output << "  perimortem_core_object_release("_view << field->get_name()
             << ");\n"_view;
    }
    output << "  "_view << field->get_name() << " = other."_view
           << field->get_name() << ";\n"_view;
    if (kind &&
        (*kind ==
             Tetrodotoxin::Terminal::Abi::Representation::Type::Kind::Object ||
         *kind == Tetrodotoxin::Terminal::Abi::Representation::Type::Kind::
                      ObjectStorage)) {
      output << "  perimortem_core_object_retain("_view << field->get_name()
             << ");\n"_view;
    }
  }
  output << "  return *this;\n}\n\n"_view;

  output << "auto "_view;
  write_cpp_path(output, qualified_package, '.');
  output << "::"_view;
  write_cpp_path(output, qualified_route, ':');
  output << "::operator=("_view;
  write_cpp_path(output, qualified_package, '.');
  output << "::"_view;
  write_cpp_path(output, qualified_route, ':');
  output << "&& other) noexcept -> "_view << route_leaf(qualified_route)
         << "& {\n  if (this == &other) {\n    return *this;\n  }\n"_view;
  for (Count index = 0; index < fields->get_size(); index++) {
    auto field = require_parameter(*fields, index);
    auto kind =
        field ? require_kind(types, field->get_domain())
              : Core::Option<
                    Tetrodotoxin::Terminal::Abi::Representation::Type::Kind>();
    if (kind &&
        (*kind ==
             Tetrodotoxin::Terminal::Abi::Representation::Type::Kind::Object ||
         *kind == Tetrodotoxin::Terminal::Abi::Representation::Type::Kind::
                      ObjectStorage)) {
      output << "  perimortem_core_object_release("_view << field->get_name()
             << ");\n"_view;
    }
    output << "  "_view << field->get_name() << " = other."_view
           << field->get_name() << ";\n  other."_view << field->get_name()
           << " = {};\n"_view;
  }
  output << "  return *this;\n}\n\n"_view;

  output << ""_view;
  write_cpp_path(output, qualified_package, '.');
  output << "::"_view;
  write_cpp_path(output, qualified_route, ':');
  output << "::~"_view << route_leaf(qualified_route) << "() {\n"_view;
  for (Count index = 0; index < fields->get_size(); index++) {
    auto field = require_parameter(*fields, index);
    auto kind =
        field ? require_kind(types, field->get_domain())
              : Core::Option<
                    Tetrodotoxin::Terminal::Abi::Representation::Type::Kind>();
    if (kind &&
        (*kind ==
             Tetrodotoxin::Terminal::Abi::Representation::Type::Kind::Object ||
         *kind == Tetrodotoxin::Terminal::Abi::Representation::Type::Kind::
                      ObjectStorage)) {
      output << "  perimortem_core_object_release("_view << field->get_name()
             << ");\n"_view;
    }
  }
  output << "}\n\n"_view;
  return True;
}

static auto uses_implementation(
    const Tetrodotoxin::Terminal::Abi::Representation::Type& types,
    const Tetrodotoxin::Terminal::Abi::Unit& unit) -> Bool {
  for (const Tetrodotoxin::Terminal::Abi::Unit::TypeBinding& binding :
       unit.get_types()) {
    auto kind = types.get_kind(binding.get_semantic());
    if (kind && *kind == Tetrodotoxin::Terminal::Abi::Representation::Type::
                             Kind::Implementation) {
      return True;
    }
  }

  return False;
}

auto Tetrodotoxin::Terminal::Abi::Cpp::Header::create(
    Memory::Allocator::Arena& arena,
    const Tetrodotoxin::Terminal::Abi::Representation::Type& types,
    const Tetrodotoxin::Terminal::Abi::Unit& unit,
    Core::View::Vector<Tetrodotoxin::Terminal::Abi::Export> exports,
    Core::View::Vector<Tetrodotoxin::Terminal::Abi::Publication> publications)
    -> Core::Option<Tetrodotoxin::Terminal::Abi::Cpp::Header> {
  if (!unit.produces_cpp_api()) {
    return Header({}, {});
  }
  if (!unit.is_package_member()) {
    fail_cxx(
        "The first generated C++ API requires one Package member unit."_view);
    return {};
  }

  Memory::Managed::Bytes header(arena);
  Stream::Textual<Memory::Managed::Bytes> header_output(header);
  header_output << R"(// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

)"_view;

  if (uses_implementation(types, unit)) {
    header_output << R"(#include "perimortem/core/implementation.h"

#ifndef TETRODOTOXIN_GENERATED_IMPLEMENTATION
#define TETRODOTOXIN_GENERATED_IMPLEMENTATION

namespace Tetrodotoxin {

// Implementation gives generated C++ callers ordinary value ownership over
// the explicit C carrier. The C ABI remains authoritative while this Terminal
// product supplies the copy and move operations expected by C++ values.
class Implementation {
 public:
  Implementation() = default;

  Implementation(const Implementation& source) {
    perimortem_core_implementation_copy(&source.value, &value);
  }

  Implementation(Implementation&& source) noexcept : value(source.value) {
    source.value = {};
  }

  auto operator=(const Implementation& source) -> Implementation& {
    perimortem_core_implementation_assign(&value, &source.value);
    return *this;
  }

  auto operator=(Implementation&& source) noexcept -> Implementation& {
    perimortem_core_implementation_move(&value, &source.value);
    return *this;
  }

  ~Implementation() { perimortem_core_implementation_release(&value); }

  static auto adopt(
      std::uint8_t* object,
      const struct perimortem_projection* projection) -> Implementation {
    Implementation result;
    perimortem_core_implementation_adopt(object, projection, &result.value);
    return result;
  }

  auto is_empty() const -> bool {
    return perimortem_core_implementation_is_empty(&value);
  }

  auto is_valid() const -> bool {
    return perimortem_core_implementation_is_valid(&value);
  }

  auto get_object() const -> std::uint8_t* { return value.object; }

  auto get_projection() const -> const struct perimortem_projection* {
    return value.projection;
  }

 private:
  struct perimortem_implementation value = {};
};

}  // namespace Tetrodotoxin

#endif

)"_view;
  }

  Memory::Managed::Bytes source(arena);
  Stream::Textual<Memory::Managed::Bytes> source_output(source);
  source_output
      << "// # Tetrodotoxin\n"
         "// Copyright (c) 2023-present Matt Kaes and contributors\n\n"
         "#include \""_view
      << path_leaf(unit.get_cpp_header()) << "\"\n#include \""_view
      << unit.get_c_header() << "\"\n\n"_view;
  if (!write_construction_declarations(
          arena, source_output, types, unit, publications)) {
    return {};
  }

  for (const Tetrodotoxin::Terminal::Abi::Unit::TypeBinding& binding :
       unit.get_types()) {
    if (binding.get_package() != unit.get_package() ||
        binding.get_member() != unit.get_member()) {
      continue;
    }
    if (!write_namespace_declaration(
            header_output, types, unit, binding, exports) ||
        !write_class_declaration(
            header_output, types, unit, binding, exports) ||
        !write_lifecycle_definitions(
            arena, source_output, types, unit, binding, publications) ||
        !write_factory_definitions(
            source_output, types, unit, binding, exports)) {
      return {};
    }
    for (const Tetrodotoxin::Terminal::Abi::Export& exported : exports) {
      if (!write_method_definition(
              arena, source_output, types, unit, binding, exported)) {
        return {};
      }
    }
  }

  return Header(header.get_view(), source.get_view());
}
