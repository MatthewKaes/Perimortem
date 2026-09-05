// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "puffer/lsp/inlay_hints.hpp"

#include <algorithm>
#include <cstddef>
#include <vector>

#include "perimortem/core/math.hpp"

#include "perimortem/memory/managed/bytes.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "perimortem/serialization/json/blueprint.hpp"

#include "ttx/query.hpp"

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

struct Parameter {
  std::vector<uint8_t> path;
  ttx_abstract producer;
};

struct ParameterVisitor {
  ttx_layout_entry_sink_ops operations;
  std::vector<Parameter> entries;
  bool completed;
};

template <typename Owner, typename Self>
static auto select_owner(Self* self) -> Owner& {
  return *reinterpret_cast<Owner*>(self);
}

static void TTX_CALL retain_parameter(
    ttx_layout_entry_sink self,
    ttx_borrowed_bytes path,
    ttx_abstract producer) {
  auto& visitor = select_owner<ParameterVisitor>(self.self);
  visitor.entries.push_back({
    .path = path.size == 0
                ? std::vector<uint8_t>()
                : std::vector<uint8_t>(path.data, path.data + path.size),
    .producer = producer,
  });
}

static void TTX_CALL parameters_completed(ttx_layout_entry_sink self) {
  select_owner<ParameterVisitor>(self.self).completed = true;
}

struct EnumerableCapture {
  ttx_enumerable_result_ops operations;
  bool answered;
  ttx_enumerable enumerable;
};

static void TTX_CALL enumerable_rejected(ttx_enumerable_result self) {
  auto& capture = select_owner<EnumerableCapture>(self.self);
  capture.answered = true;
  capture.enumerable = {};
}

static void TTX_CALL enumerable_satisfied(
    ttx_enumerable_result self,
    ttx_enumerable enumerable) {
  auto& capture = select_owner<EnumerableCapture>(self.self);
  capture.answered = true;
  capture.enumerable = enumerable;
}

static auto parameters(ttx_layout layout) -> std::vector<Parameter> {
  EnumerableCapture enumerable = {
    .operations =
        {
          .header =
              {
                .size = sizeof(ttx_enumerable_result_ops),
                .abi_major = TTX_ABI_MAJOR,
                .abi_minor = TTX_ABI_MINOR,
              },
          .rejected = enumerable_rejected,
          .satisfied = enumerable_satisfied,
        },
    .answered = false,
    .enumerable = {},
  };
  const ttx_enumerable_result result = {
    .operations = &enumerable.operations,
    .self = reinterpret_cast<ttx_enumerable_result_self*>(&enumerable),
  };
  layout.operations->enumerable(layout, result);
  if (!enumerable.answered || enumerable.enumerable.operations == nullptr) {
    return {};
  }
  ParameterVisitor visitor = {
    .operations =
        {
          .header =
              {
                .size = sizeof(ttx_layout_entry_sink_ops),
                .abi_major = TTX_ABI_MAJOR,
                .abi_minor = TTX_ABI_MINOR,
              },
          .entry = retain_parameter,
          .completed = parameters_completed,
        },
    .entries = {},
    .completed = false,
  };
  const ttx_layout_entry_sink sink = {
    .operations = &visitor.operations,
    .self = reinterpret_cast<ttx_layout_entry_sink_self*>(&visitor),
  };
  enumerable.enumerable.operations->visit(enumerable.enumerable, sink);
  if (!visitor.completed) {
    return {};
  }
  std::sort(
      visitor.entries.begin(), visitor.entries.end(),
      [](const Parameter& left, const Parameter& right) {
        return left.path < right.path;
      });
  return std::move(visitor.entries);
}

static auto name(ttx_abstract candidate) -> Core::View::Bytes {
  const ttx_borrowed_bytes value = candidate.operations->name(candidate);
  return Core::View::Bytes(value.data, value.size);
}

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
    const Ttx::CallableObservation callable =
        Ttx::resolve_callable(association.get_semantic().get_handle());
    if (callable.state != Ttx::Observation::Resolved) {
      continue;
    }

    Memory::Managed::Vector<Count> arguments(arena);
    if (!collect_arguments(source, association.get_anchor(), arguments) ||
        arguments.is_empty()) {
      continue;
    }
    std::vector<Parameter> parameter_entries =
        parameters(callable.callable.operations->parameters(callable.callable));
    Count parameter_start =
        !parameter_entries.empty() &&
                name(parameter_entries[0].producer) == "self"_view
            ? Count(1)
            : Count(0);
    Count parameter_count = parameter_entries.size() - parameter_start;
    Count mapping_count = parameter_count == arguments.get_size()
                              ? parameter_count
                              : (parameter_count == 1 ? Count(1) : Count(0));
    for (Count index = 0; index < mapping_count; index++) {
      Count offset = arguments[index];
      if (source[offset] == '.') {
        continue;
      }
      Core::View::Bytes parameter_name =
          name(parameter_entries[parameter_start + index].producer);
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
