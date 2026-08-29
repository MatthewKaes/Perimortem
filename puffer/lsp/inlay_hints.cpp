// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "puffer/lsp/inlay_hints.hpp"

#include "perimortem/core/math.hpp"

#include "perimortem/memory/managed/bytes.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "perimortem/serialization/json/blueprint.hpp"

#include "ttx/model/callable.h"

using namespace Perimortem;
using namespace Puffer;

static auto skip_space(Core::View::Bytes source, Count offset, Count end)
    -> Count {
  while (offset < end && (source[offset] == ' ' || source[offset] == '\t' ||
                          source[offset] == '\r' || source[offset] == '\n')) {
    offset++;
  }
  return offset;
}

static auto collect_arguments(
    Core::View::Bytes source,
    Ttx::Lexical::Anchor anchor,
    Memory::Managed::Vector<Count>& arguments) -> Bool {
  Ttx::Lexical::Token focus = anchor.get_token();
  Ttx::Lexical::Span span = anchor.get_span();
  BAIL_IF(!focus || !span);
  Count end = Core::Math::min(
      source.get_size(), Count(span.get_offset()) + span.get_size());
  Count offset = Count(focus.get_offset()) + focus.get_size();
  offset = skip_space(source, offset, end);
  BAIL_IF(offset >= end || source[offset] != '(');

  offset = skip_space(source, offset + 1, end);
  if (offset >= end || source[offset] == ')') {
    return True;
  }
  arguments.insert(offset);

  Count parentheses = 1;
  Count brackets = 0;
  Count braces = 0;
  Bool quoted = False;
  Bool escaped = False;
  Bool comment = False;
  for (; offset < end; offset++) {
    U8 byte = source[offset];
    if (comment) {
      if (byte == '\n') {
        comment = False;
      }
      continue;
    }
    if (quoted) {
      if (escaped) {
        escaped = False;
      } else if (byte == '\\') {
        escaped = True;
      } else if (byte == '"') {
        quoted = False;
      }
      continue;
    }
    if (byte == '"') {
      quoted = True;
      continue;
    }
    if (byte == '/' && offset + 1 < end && source[offset + 1] == '/') {
      comment = True;
      offset++;
      continue;
    }
    if (byte == '(') {
      parentheses++;
    } else if (byte == ')') {
      BAIL_IF(parentheses == 0);
      parentheses--;
      if (parentheses == 0) {
        return True;
      }
    } else if (byte == '[') {
      brackets++;
    } else if (byte == ']') {
      BAIL_IF(brackets == 0);
      brackets--;
    } else if (byte == '{') {
      braces++;
    } else if (byte == '}') {
      BAIL_IF(braces == 0);
      braces--;
    } else if (
        byte == ',' && parentheses == 1 && brackets == 0 && braces == 0) {
      Count next = skip_space(source, offset + 1, end);
      BAIL_IF(next >= end || source[next] == ')');
      arguments.insert(next);
    }
  }
  return False;
}

class ParameterVisitor {
 public:
  explicit ParameterVisitor(
      Memory::Managed::Vector<const ttx_abstract*>& entries)
      : callable{&operations}, operations{.call = retain}, entries(entries) {}

  ttx_abstract_callable callable;

 private:
  static auto retain(ttx_abstract_callable* callable, const ttx_abstract* entry)
      -> void {
    auto& self = *reinterpret_cast<ParameterVisitor*>(callable);
    self.entries.insert(entry);
  }

  ttx_abstract_callable_operations operations;
  Memory::Managed::Vector<const ttx_abstract*>& entries;
};

auto Puffer::Lsp::inlay_hints_for(
    Memory::Allocator::Arena& arena,
    Core::View::Bytes source,
    const PositionEncoding& encoding,
    const Ttx::Lexical::Associations& associations,
    const PositionEncoding::Position& start,
    const PositionEncoding::Position& end) -> Serialization::Json::Node {
  Memory::Managed::Vector<Serialization::Json::Node> hints(arena);
  // An exact Association retains the selected Callable, while its Anchor keeps
  // the authored call span. The Callable's ordered Layout is the sole semantic
  // parameter authority; the lexical span contributes only argument positions.
  for (const Ttx::Lexical::Associations::Entry& association :
       associations.get_entries()) {
    ttx_callable_view callable;
    if (!ttx_callable_prove(association.get_semantic().get_abi(), &callable)) {
      continue;
    }

    Memory::Managed::Vector<Count> arguments(arena);
    if (!collect_arguments(source, association.get_anchor(), arguments) ||
        arguments.is_empty()) {
      continue;
    }
    Memory::Managed::Vector<const ttx_abstract*> parameters(arena);
    ParameterVisitor visitor(parameters);
    ttx_layout_visit(ttx_callable_parameters(&callable), &visitor.callable);
    Count parameter_start =
        !parameters.is_empty() &&
                Core::View::Bytes(
                    ttx_abstract_name(parameters.at(0)).data,
                    ttx_abstract_name(parameters.at(0)).size) == "self"_view
            ? Count(1)
            : Count(0);
    Count parameter_count = parameters.get_size() - parameter_start;
    Count mapping_count = parameter_count == arguments.get_size()
                              ? parameter_count
                              : (parameter_count == 1 ? Count(1) : Count(0));
    for (Count index = 0; index < mapping_count; index++) {
      Count offset = arguments[index];
      if (source[offset] == '.') {
        continue;
      }
      perimortem_bytes name =
          ttx_abstract_name(parameters.at(parameter_start + index));
      Core::View::Bytes parameter_name(name.data, name.size);
      if (parameter_name.is_empty()) {
        continue;
      }
      auto position = encoding.locate(source, offset);
      if (!position || position->is_before(start) ||
          !position->is_before(end)) {
        continue;
      }

      Memory::Managed::Bytes label(arena, "."_view);
      label.concat(parameter_name);
      label.concat(" ="_view);
      hints.insert(
          Serialization::Json::Blueprint{
            {
              {"position"_view,
               {
                 {"line"_view, position->get_line()},
                 {"character"_view, position->get_character()},
               }},
              {"label"_view, label.get_view()},
              {"kind"_view, S64(2)},
              {"paddingRight"_view, True},
            }}.construct(arena));
    }
  }

  return Serialization::Json::Node(hints.get_view());
}
