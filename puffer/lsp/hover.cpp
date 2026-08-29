// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "puffer/lsp/hover.hpp"

#include "perimortem/core/null_terminated.hpp"

#include "perimortem/memory/managed/bytes.hpp"

#include "perimortem/serialization/json/blueprint.hpp"
#include "perimortem/serialization/stream/textual.hpp"

#include "ttx/concept/alias.h"
#include "ttx/concept/constant.h"
#include "ttx/concept/none.h"
#include "ttx/concept/unknown.h"
#include "ttx/model/addressable.h"
#include "ttx/model/callable.h"
#include "ttx/model/layouts/named.h"
#include "ttx/model/type.h"

using namespace Perimortem;

static auto name(const ttx_abstract* semantic) -> Core::View::Bytes {
  perimortem_bytes value = ttx_abstract_name(semantic);
  return Core::View::Bytes(value.data, value.size);
}

static auto append_name(
    Serialization::Stream::Textual<Memory::Managed::Bytes>& output,
    Core::View::Bytes value) -> void {
  constexpr Core::View::Bytes hexadecimal = "0123456789ABCDEF"_view;
  Bool textual = !value.is_empty();
  for (Count index = 0; index < value.get_size(); index++) {
    textual &=
        value[index] >= 0x20 && value[index] <= 0x7e && value[index] != '`';
  }
  if (textual) {
    output << value;
    return;
  }

  output << "$["_view;
  for (Count index = 0; index < value.get_size(); index++) {
    if (index != 0) {
      output << " "_view;
    }
    output << hexadecimal.slice(value[index] >> 4, 1)
           << hexadecimal.slice(value[index] & 0x0f, 1);
  }
  output << "]"_view;
}

static auto append_type(
    Serialization::Stream::Textual<Memory::Managed::Bytes>& output,
    const ttx_abstract* semantic) -> void {
  ttx_type_view type;
  const ttx_abstract* answer = semantic;
  ttx_addressable_view addressable;
  perimortem_bool has_type = ttx_type_prove(semantic, &type);
  if (!has_type && ttx_addressable_prove(semantic, &addressable)) {
    answer = ttx_addressable_type(&addressable);
    has_type = ttx_type_prove(answer, &type);
  } else if (!has_type) {
    answer = ttx_abstract_type(semantic);
    has_type = ttx_type_prove(answer, &type);
  }
  ttx_unknown_view unknown;
  ttx_none_view none;
  if (!has_type && !ttx_unknown_prove(answer, &unknown) &&
      !ttx_none_prove(answer, &none)) {
    answer = ttx_abstract_resolve(answer);
    has_type = ttx_type_prove(answer, &type);
  }
  if (has_type) {
    append_name(output, name(type.identity));
  } else if (ttx_unknown_prove(answer, &unknown)) {
    output << "Unknown"_view;
  } else {
    output << "None"_view;
  }
}

class LayoutWriter {
 public:
  explicit LayoutWriter(
      Serialization::Stream::Textual<Memory::Managed::Bytes>& output)
      : callable{&operations}, operations{.call = write}, output(output) {}

  ttx_abstract_callable callable;

 private:
  static auto write(ttx_abstract_callable* callable, const ttx_abstract* entry)
      -> void {
    auto& self = *reinterpret_cast<LayoutWriter*>(callable);
    if (!self.first) {
      self.output << ", "_view;
    }
    self.first = False;
    append_type(self.output, entry);
  }

  ttx_abstract_callable_operations operations;
  Serialization::Stream::Textual<Memory::Managed::Bytes>& output;
  Bool first = True;
};

class NamedLayoutWriter {
 public:
  explicit NamedLayoutWriter(
      Serialization::Stream::Textual<Memory::Managed::Bytes>& output)
      : callable{&operations}, operations{.call = write}, output(output) {}

  ttx_named_abstract_callable callable;

 private:
  static auto write(
      ttx_named_abstract_callable* callable,
      perimortem_bytes entry_name,
      const ttx_abstract* entry) -> void {
    auto& self = *reinterpret_cast<NamedLayoutWriter*>(callable);
    if (!self.first) {
      self.output << ", "_view;
    }
    self.first = False;
    self.output << "."_view;
    append_name(
        self.output, Core::View::Bytes(entry_name.data, entry_name.size));
    self.output << " : "_view;
    append_type(self.output, entry);
  }

  ttx_named_abstract_callable_operations operations;
  Serialization::Stream::Textual<Memory::Managed::Bytes>& output;
  Bool first = True;
};

static auto append_layout(
    Serialization::Stream::Textual<Memory::Managed::Bytes>& output,
    const ttx_layout* layout) -> void {
  output << "["_view;
  ttx_named_layout_view named;
  if (ttx_named_layout_prove(layout, &named)) {
    NamedLayoutWriter writer(output);
    ttx_named_layout_visit(&named, &writer.callable);
  } else {
    LayoutWriter writer(output);
    ttx_layout_visit(layout, &writer.callable);
  }
  output << "]"_view;
}

static auto append_identity(
    Serialization::Stream::Textual<Memory::Managed::Bytes>& output,
    const ttx_abstract* semantic) -> Bool {
  ttx_alias_view alias;
  if (ttx_alias_prove(semantic, &alias)) {
    output << "alias "_view;
    append_name(output, name(semantic));
    const ttx_abstract* target = ttx_abstract_resolve(semantic);
    ttx_unknown_view unknown;
    ttx_none_view none;
    if (!ttx_unknown_prove(target, &unknown) &&
        !ttx_none_prove(target, &none)) {
      output << " = "_view;
      append_name(output, name(target));
    }
    return True;
  }

  ttx_callable_view callable;
  if (ttx_callable_prove(semantic, &callable)) {
    output << "func "_view;
    append_name(output, name(semantic));
    append_layout(output, ttx_callable_parameters(&callable));
    output << " -> "_view;
    append_layout(output, ttx_callable_results(&callable));
    return True;
  }

  ttx_addressable_view addressable;
  if (ttx_addressable_prove(semantic, &addressable)) {
    append_name(output, name(semantic));
    output << " : "_view;
    append_type(output, semantic);
    return True;
  }

  ttx_constant_view constant;
  if (ttx_constant_prove(semantic, &constant)) {
    output << "const "_view;
    append_name(output, name(semantic));
    return True;
  }
  ttx_type_view type;
  if (ttx_type_prove(semantic, &type)) {
    output << "Type "_view;
    append_name(output, name(semantic));
    return True;
  }
  if (name(semantic).is_empty()) {
    return False;
  }
  append_name(output, name(semantic));
  return True;
}

class DocumentationWriter {
 public:
  explicit DocumentationWriter(
      Serialization::Stream::Textual<Memory::Managed::Bytes>& output)
      : callable{&operations}, operations{.call = write}, output(output) {}

  ttx_bytes_callable callable;

 private:
  static auto write(ttx_bytes_callable* callable, perimortem_bytes line)
      -> void {
    auto& self = *reinterpret_cast<DocumentationWriter*>(callable);
    self.output << (self.first ? "\n\n"_view : "\n"_view)
                << Core::View::Bytes(line.data, line.size);
    self.first = False;
  }

  ttx_bytes_callable_operations operations;
  Serialization::Stream::Textual<Memory::Managed::Bytes>& output;
  Bool first = True;
};

auto Puffer::Lsp::semantic_hover(
    Memory::Allocator::Arena& arena,
    const ttx_abstract* semantic) -> Serialization::Json::Node {
  Memory::Managed::Bytes buffer(arena);
  Serialization::Stream::Textual<Memory::Managed::Bytes> output(buffer);
  output << "```tetrodotoxin\n"_view;
  if (!append_identity(output, semantic)) {
    return Serialization::Json::Node();
  }
  output << "\n```"_view;

  DocumentationWriter writer(output);
  ttx_documentation_visit(
      ttx_abstract_documentation(semantic), &writer.callable);

  return Serialization::Json::Blueprint{
    {
      {"contents"_view,
       {
         {"kind"_view, "markdown"_view},
         {"value"_view, buffer.get_view()},
       }},
    }}.construct(arena);
}
