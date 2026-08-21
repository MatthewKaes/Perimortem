// Perimortem Engine
// Copyright © Matt Kaes

#include "puffer/lsp/hover.hpp"

#include "perimortem/memory/managed/bytes.hpp"

#include "perimortem/serialization/json/blueprint.hpp"
#include "perimortem/serialization/stream/textual.hpp"

#include "tetrodotoxin/library/language/access/call.hpp"
#include "tetrodotoxin/library/language/constant.hpp"
#include "tetrodotoxin/library/language/constants/bytes.hpp"
#include "tetrodotoxin/library/language/constants/enumeration.hpp"
#include "tetrodotoxin/library/language/constants/flag.hpp"
#include "tetrodotoxin/library/language/constants/option.hpp"
#include "tetrodotoxin/library/language/constants/real.hpp"
#include "tetrodotoxin/library/language/constants/signed.hpp"
#include "tetrodotoxin/library/language/constants/unsigned.hpp"
#include "tetrodotoxin/library/language/expression.hpp"
#include "tetrodotoxin/library/language/field.hpp"
#include "tetrodotoxin/library/language/flow/local.hpp"
#include "tetrodotoxin/library/language/function.hpp"
#include "tetrodotoxin/library/language/model/addressable.hpp"
#include "tetrodotoxin/library/language/model/callable.hpp"
#include "tetrodotoxin/library/language/model/type.hpp"
#include "tetrodotoxin/library/language/parameter.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/model/alias.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Serialization;
using namespace Puffer;
using namespace Tetrodotoxin::Library::Language;
using namespace Ttx::Concept;

static auto append_pack(
    Stream::Textual<Managed::Bytes>& output,
    const Model::Pack& pack,
    Count depth) -> void;

static auto append_bytes(
    Stream::Textual<Managed::Bytes>& output,
    View::Bytes value) -> void {
  constexpr Count preview_limit = 32;
  constexpr View::Bytes hexadecimal = "0123456789ABCDEF"_view;
  Count visible =
      value.get_size() < preview_limit ? value.get_size() : preview_limit;

  output << "\""_view;
  for (Count index = 0; index < visible; index++) {
    Unsigned_8 byte = value[index];
    if (byte == 0x09) {
      output << "\\t"_view;
    } else if (byte == 0x0A) {
      output << "\\n"_view;
    } else if (byte == 0x0D) {
      output << "\\r"_view;
    } else if (byte == 0x22) {
      output << "\\\""_view;
    } else if (byte == 0x5C) {
      output << "\\\\"_view;
    } else if (byte >= 0x20 && byte <= 0x7E && byte != 0x60) {
      output << View::Bytes(&byte, 1);
    } else {
      output << "\\x"_view << hexadecimal.slice(byte >> 4, 1)
             << hexadecimal.slice(byte & 0x0F, 1);
    }
  }
  if (value.get_size() > visible) {
    output << "…"_view;
  }
  output << "\""_view;
}

static auto append_constant(
    Stream::Textual<Managed::Bytes>& output,
    const Constant& constant,
    Count depth) -> void {
  if (auto option = constant.select<Constants::Option>()) {
    if (option->get_kind() == Types::Option::Kind::Absent) {
      output << "absent"_view;
      return;
    }
    output << "some("_view;
    option->get_payload().visit(
        [&]() { output << "?"_view; },
        [&](const Model::Pack& payload) {
          append_pack(output, payload, depth + 1);
        });
    output << ")"_view;
    return;
  }
  if (auto value = constant.select<Constants::Unsigned>()) {
    output << value->get_value();
    return;
  }
  if (auto value = constant.select<Constants::Signed>()) {
    output << value->get_value();
    return;
  }
  if (auto value = constant.select<Constants::Real>()) {
    output << value->get_value();
    return;
  }
  if (auto value = constant.select<Constants::Flag>()) {
    output << (value->get_value() ? "true"_view : "false"_view);
    return;
  }
  if (auto value = constant.select<Constants::Enumeration>()) {
    output << value->get_value();
    return;
  }
  if (auto value = constant.select<Constants::Bytes>()) {
    append_bytes(output, value->get_value());
    return;
  }
  output << "<constant>"_view;
}

static auto append_pack(
    Stream::Textual<Managed::Bytes>& output,
    const Model::Pack& pack,
    Count depth) -> void {
  // Recursive Options can describe deeply nested constant flow. A presentation
  // limit keeps one hover bounded without changing the retained value.
  if (depth > 8) {
    output << "<depth limit>"_view;
    return;
  }
  auto constant = pack.select<Constant>();
  if (constant) {
    append_constant(output, *constant, depth);
    return;
  }

  Count size = pack.get_layout().get_size();
  if (size == 0) {
    output << "()"_view;
    return;
  }
  if (size == 1) {
    auto produced = pack.get_produced(0);
    if (produced && &produced->producer != &pack) {
      auto selected = produced->producer.select<Model::Pack>();
      if (selected) {
        append_pack(output, *selected, depth + 1);
        return;
      }
    }
    output << "<dynamic>"_view;
    return;
  }

  output << "("_view;
  for (Count i = 0; i < size; i++) {
    if (i != 0) {
      output << ", "_view;
    }
    auto produced = pack.get_produced(i);
    if (!produced || &produced->producer == &pack) {
      output << "?"_view;
    } else {
      auto selected = produced->producer.select<Model::Pack>();
      if (selected) {
        append_pack(output, *selected, depth + 1);
      } else {
        output << "?"_view;
      }
    }
  }
  output << ")"_view;
}

static auto writability_name(Writability writability) -> View::Bytes {
  return writability == Writability::Constant ? "const"_view : "state"_view;
}

static auto semantic_subject(const Abstract& semantic) -> const Abstract& {
  auto call = semantic.select<Tetrodotoxin::Library::Language::Access::Call>();
  if (call) {
    auto callable = call->get_callable();
    if (callable) {
      return *callable;
    }
  }

  auto expression = semantic.select<Expression>();
  if (expression) {
    const Abstract& result = expression->get_result();
    if (&result != &*expression && !result.is<Invalid>()) {
      return result;
    }
  }
  return semantic;
}

static auto append_declaration(
    Stream::Textual<Managed::Bytes>& output,
    const Abstract& subject) -> Bool {
  auto alias = subject.select<Ttx::Model::Alias>();
  if (alias) {
    output << alias->get_name() << " : alias"_view;
    const Abstract& target = alias->resolve();
    if (!target.is<Invalid>() && !target.get_name().is_empty()) {
      output << " = "_view << target.get_name();
    }
    return True;
  }

  auto field = subject.select<Field>();
  if (field) {
    output << writability_name(field->get_writability()) << " "_view
           << field->get_name() << " : "_view << field->get_type().get_name();
    return True;
  }

  auto local = subject.select<Flow::Local>();
  if (local) {
    output << writability_name(local->get_writability()) << " "_view
           << local->get_name() << " : "_view << local->get_type().get_name();
    return True;
  }

  auto parameter = subject.select<Parameter>();
  if (parameter) {
    output << "parameter "_view << parameter->get_name() << " : "_view
           << parameter->get_type().get_name();
    return True;
  }

  auto function = subject.select<Function>();
  if (function) {
    output << "func "_view << function->get_name();
    return True;
  }

  auto callable = subject.select<Model::Callable>();
  if (callable) {
    output << "callable "_view << callable->get_name();
    return True;
  }

  auto type = subject.select<Model::Type>();
  if (type) {
    output << "type "_view << type->get_name();
    return True;
  }

  auto constant = subject.select<Constant>();
  if (constant) {
    output << "const : "_view << constant->get_type().get_name();
    return True;
  }

  auto addressable = subject.select<Model::Addressable>();
  if (addressable) {
    output << subject.get_name() << " : "_view
           << addressable->get_type().get_name();
    return True;
  }

  if (subject.get_name().is_empty()) {
    return False;
  }
  output << subject.get_name();
  return True;
}

static auto append_details(
    Stream::Textual<Managed::Bytes>& output,
    const Abstract& subject) -> void {
  auto alias = subject.select<Ttx::Model::Alias>();
  if (alias) {
    output << "\n\n**Kind:** Type alias"_view;
    const Abstract& target = alias->resolve();
    if (!target.is<Invalid>() && !target.get_name().is_empty()) {
      output << "  \n**Resolves to:** `"_view << target.get_name() << "`"_view;
    }
    return;
  }

  auto field = subject.select<Field>();
  if (field) {
    output << (field->get_writability() == Writability::Constant
                   ? "\n\n**Kind:** Constant field"_view
                   : "\n\n**Kind:** State field"_view)
           << "  \n**Type:** `"_view << field->get_type().get_name()
           << "`"_view;
    return;
  }

  auto local = subject.select<Flow::Local>();
  if (local) {
    output << (local->get_writability() == Writability::Constant
                   ? "\n\n**Kind:** Constant local"_view
                   : "\n\n**Kind:** State local"_view)
           << "  \n**Type:** `"_view << local->get_type().get_name()
           << "`"_view;
    return;
  }

  auto parameter = subject.select<Parameter>();
  if (parameter) {
    output << "\n\n**Kind:** Parameter  \n**Type:** `"_view
           << parameter->get_type().get_name() << "`"_view;
    return;
  }

  if (subject.is<Function>()) {
    output << "\n\n**Kind:** Function"_view;
    return;
  }
  if (subject.is<Model::Callable>()) {
    output << "\n\n**Kind:** Callable"_view;
    return;
  }
  if (subject.is<Model::Type>()) {
    output << "\n\n**Kind:** Type"_view;
    return;
  }

  auto constant = subject.select<Constant>();
  if (constant) {
    output << "\n\n**Kind:** Constant  \n**Type:** `"_view
           << constant->get_type().get_name() << "`"_view;
    return;
  }

  auto addressable = subject.select<Model::Addressable>();
  if (addressable) {
    output << "\n\n**Kind:** Addressable  \n**Type:** `"_view
           << addressable->get_type().get_name() << "`"_view;
  }
}

static auto append_value(
    Stream::Textual<Managed::Bytes>& output,
    const Abstract& subject) -> void {
  auto field = subject.select<Field>();
  if (field) {
    field->get_constant().visit(
        []() {},
        [&](const Model::Pack& constant) {
          output << "\n\n`= "_view;
          append_pack(output, constant, 0);
          output << "`"_view;
        });
    return;
  }

  auto local = subject.select<Flow::Local>();
  if (local) {
    local->get_constant().visit(
        []() {},
        [&](const Model::Pack& constant) {
          output << "\n\n`= "_view;
          append_pack(output, constant, 0);
          output << "`"_view;
        });
    return;
  }

  auto constant = subject.select<Constant>();
  if (constant) {
    output << "\n\n`= "_view;
    append_constant(output, *constant, 0);
    output << "`"_view;
  }
}

auto Lsp::semantic_hover(Allocator::Arena& arena, const Abstract& semantic)
    -> Json::Node {
  const Abstract& subject = semantic_subject(semantic);
  Managed::Bytes buffer(arena);
  Stream::Textual<Managed::Bytes> output(buffer);
  output << "```ttx\n"_view;
  if (!append_declaration(output, subject)) {
    return Json::Node();
  }
  output << "\n```"_view;
  append_details(output, subject);
  append_value(output, subject);

  const Documentation& documentation = subject.get_documentation();
  if (!documentation.is_empty()) {
    output << "\n\n---\n\n**Documentation**\n\n"_view;
    for (Count i = 0; i < documentation.line_count(); i++) {
      if (i != 0) {
        output << "\n"_view;
      }
      output << ">"_view;
      View::Bytes line = documentation.get_line(i);
      if (!line.is_empty()) {
        output << " "_view << line;
      }
    }
  }
  return Json::Blueprint{
    {
      {"contents"_view,
       {
         {"kind"_view, "markdown"_view},
         {"value"_view, buffer.get_view()},
       }},
    }}.construct(arena);
}
