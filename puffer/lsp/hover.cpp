// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "puffer/lsp/hover.hpp"

#include <algorithm>
#include <cstddef>
#include <vector>

#include "perimortem/core/null_terminated.hpp"

#include "perimortem/memory/managed/bytes.hpp"

#include "perimortem/serialization/json/blueprint.hpp"
#include "perimortem/serialization/stream/textual.hpp"

#include "ttx/query.hpp"

using namespace Perimortem;
using namespace Perimortem::Core;

template <typename Operations>
static auto supports(const Operations* operations, uint32_t size) -> bool {
  return operations != nullptr &&
         operations->header.abi_major == TTX_ABI_MAJOR &&
         operations->header.size >= size;
}

template <typename Owner, typename Self>
static auto select_owner(Self* self) -> Owner& {
  return *reinterpret_cast<Owner*>(self);
}

static auto name(ttx_abstract semantic) -> Core::View::Bytes {
  if (!supports(semantic.operations, TTX_ABSTRACT_INTERFACE_PREFIX_SIZE)) {
    return {};
  }
  const ttx_borrowed_bytes value = semantic.operations->name(semantic);
  return Core::View::Bytes(value.data, value.size);
}

static auto append_name(
    Serialization::Stream::Textual<Memory::Managed::Bytes>& output,
    Core::View::Bytes value) -> void {
  constexpr Core::View::Bytes hexadecimal = "0123456789ABCDEF"_view;
  Bool textual = !value.is_empty();
  for (Count index = 0; index < value.get_size(); ++index) {
    textual &=
        value[index] >= 0x20 && value[index] <= 0x7e && value[index] != '`';
  }
  if (textual) {
    output << value;
    return;
  }

  output << "$["_view;
  for (Count index = 0; index < value.get_size(); ++index) {
    if (index != 0) {
      output << " "_view;
    }
    output << hexadecimal.slice(value[index] >> 4, 1)
           << hexadecimal.slice(value[index] & 0x0f, 1);
  }
  output << "]"_view;
}

static auto append_domain(
    Serialization::Stream::Textual<Memory::Managed::Bytes>& output,
    ttx_abstract semantic) -> void {
  const Ttx::DomainObservation domain = Ttx::resolve_domain(semantic);
  if (domain.state == Ttx::Observation::Resolved) {
    append_name(output, name(domain.domain));
  } else if (domain.state == Ttx::Observation::Unknown) {
    output << "Unknown"_view;
  } else {
    output << "None"_view;
  }
}

struct LayoutEntry {
  std::vector<uint8_t> path;
  ttx_abstract producer;
};

struct EntryCapture {
  ttx_layout_entry_sink_ops operations;
  std::vector<LayoutEntry> entries;
  bool completed;
};

static void TTX_CALL retain_entry(
    ttx_layout_entry_sink self,
    ttx_borrowed_bytes path,
    ttx_abstract producer) {
  auto& capture = select_owner<EntryCapture>(self.self);
  capture.entries.push_back({
    .path = path.size == 0
                ? std::vector<uint8_t>()
                : std::vector<uint8_t>(path.data, path.data + path.size),
    .producer = producer,
  });
}

static void TTX_CALL entries_completed(ttx_layout_entry_sink self) {
  select_owner<EntryCapture>(self.self).completed = true;
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

static auto entries(ttx_layout layout) -> std::vector<LayoutEntry> {
  if (!supports(layout.operations, sizeof(ttx_layout_ops))) {
    return {};
  }
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
  if (!enumerable.answered ||
      !supports(enumerable.enumerable.operations, sizeof(ttx_enumerable_ops))) {
    return {};
  }

  EntryCapture capture = {
    .operations =
        {
          .header =
              {
                .size = sizeof(ttx_layout_entry_sink_ops),
                .abi_major = TTX_ABI_MAJOR,
                .abi_minor = TTX_ABI_MINOR,
              },
          .entry = retain_entry,
          .completed = entries_completed,
        },
    .entries = {},
    .completed = false,
  };
  const ttx_layout_entry_sink sink = {
    .operations = &capture.operations,
    .self = reinterpret_cast<ttx_layout_entry_sink_self*>(&capture),
  };
  enumerable.enumerable.operations->visit(enumerable.enumerable, sink);
  if (!capture.completed ||
      capture.entries.size() != enumerable.enumerable.operations->cardinality(
                                    enumerable.enumerable)) {
    return {};
  }
  std::sort(
      capture.entries.begin(), capture.entries.end(),
      [](const LayoutEntry& left, const LayoutEntry& right) {
        return left.path < right.path;
      });
  return std::move(capture.entries);
}

struct NamedRoute {
  std::vector<uint8_t> path;
  Ttx::Observation state;
  std::vector<uint8_t> route;
};

struct RouteCapture {
  ttx_named_route_sink_ops operations;
  std::vector<NamedRoute> routes;
  bool completed;
};

static auto copy(ttx_borrowed_bytes bytes) -> std::vector<uint8_t> {
  return bytes.size == 0
             ? std::vector<uint8_t>()
             : std::vector<uint8_t>(bytes.data, bytes.data + bytes.size);
}

static void TTX_CALL
    route_unknown(ttx_named_route_sink self, ttx_borrowed_bytes path) {
  select_owner<RouteCapture>(self.self)
      .routes.push_back(
          {.path = copy(path),
           .state = Ttx::Observation::Unknown,
           .route = {}});
}

static void TTX_CALL
    route_none(ttx_named_route_sink self, ttx_borrowed_bytes path) {
  select_owner<RouteCapture>(self.self)
      .routes.push_back(
          {.path = copy(path), .state = Ttx::Observation::None, .route = {}});
}

static void TTX_CALL route_resolved(
    ttx_named_route_sink self,
    ttx_borrowed_bytes path,
    ttx_borrowed_bytes route) {
  select_owner<RouteCapture>(self.self)
      .routes.push_back({
        .path = copy(path),
        .state = Ttx::Observation::Resolved,
        .route = copy(route),
      });
}

static void TTX_CALL routes_completed(ttx_named_route_sink self) {
  select_owner<RouteCapture>(self.self).completed = true;
}

struct NamedCapture {
  ttx_named_result_ops operations;
  bool answered;
  ttx_named named;
};

static void TTX_CALL named_rejected(ttx_named_result self) {
  auto& capture = select_owner<NamedCapture>(self.self);
  capture.answered = true;
  capture.named = {};
}

static void TTX_CALL named_satisfied(ttx_named_result self, ttx_named named) {
  auto& capture = select_owner<NamedCapture>(self.self);
  capture.answered = true;
  capture.named = named;
}

static auto routes(ttx_layout layout) -> std::vector<NamedRoute> {
  if (!supports(layout.operations, sizeof(ttx_layout_ops))) {
    return {};
  }
  NamedCapture named = {
    .operations =
        {
          .header =
              {
                .size = sizeof(ttx_named_result_ops),
                .abi_major = TTX_ABI_MAJOR,
                .abi_minor = TTX_ABI_MINOR,
              },
          .rejected = named_rejected,
          .satisfied = named_satisfied,
        },
    .answered = false,
    .named = {},
  };
  const ttx_named_result result = {
    .operations = &named.operations,
    .self = reinterpret_cast<ttx_named_result_self*>(&named),
  };
  layout.operations->named(layout, result);
  if (!named.answered ||
      !supports(named.named.operations, sizeof(ttx_named_ops))) {
    return {};
  }

  RouteCapture capture = {
    .operations =
        {
          .header =
              {
                .size = sizeof(ttx_named_route_sink_ops),
                .abi_major = TTX_ABI_MAJOR,
                .abi_minor = TTX_ABI_MINOR,
              },
          .unknown = route_unknown,
          .none = route_none,
          .route = route_resolved,
          .completed = routes_completed,
        },
    .routes = {},
    .completed = false,
  };
  const ttx_named_route_sink sink = {
    .operations = &capture.operations,
    .self = reinterpret_cast<ttx_named_route_sink_self*>(&capture),
  };
  named.named.operations->visit_routes(named.named, sink);
  return capture.completed ? std::move(capture.routes)
                           : std::vector<NamedRoute>();
}

static auto find_route(
    const std::vector<NamedRoute>& routes,
    const std::vector<uint8_t>& path) -> const NamedRoute* {
  for (const NamedRoute& route : routes) {
    if (route.path == path) {
      return &route;
    }
  }
  return nullptr;
}

static auto append_layout(
    Serialization::Stream::Textual<Memory::Managed::Bytes>& output,
    ttx_layout layout) -> void {
  const std::vector<LayoutEntry> values = entries(layout);
  const std::vector<NamedRoute> names = routes(layout);
  output << "["_view;
  for (size_t index = 0; index < values.size(); ++index) {
    if (index != 0) {
      output << ", "_view;
    }
    const NamedRoute* route = find_route(names, values[index].path);
    if (route != nullptr) {
      output << "."_view;
      if (route->state == Ttx::Observation::Resolved) {
        append_name(
            output,
            Core::View::Bytes(route->route.data(), route->route.size()));
      } else {
        output
            << (route->state == Ttx::Observation::Unknown ? "Unknown"_view
                                                          : "None"_view);
      }
      output << " : "_view;
    }
    append_domain(output, values[index].producer);
  }
  output << "]"_view;
}

static auto append_identity(
    Serialization::Stream::Textual<Memory::Managed::Bytes>& output,
    ttx_abstract semantic) -> Bool {
  const ttx_abstract resolved = Ttx::resolve(semantic);
  if (!ttx_abstract_same(resolved, semantic)) {
    append_name(output, name(semantic));
    output << " = "_view;
    if (ttx_abstract_same(resolved, ttx_unknown())) {
      output << "Unknown"_view;
    } else if (ttx_abstract_same(resolved, ttx_none())) {
      output << "None"_view;
    } else {
      append_name(output, name(resolved));
    }
    return True;
  }

  const Ttx::CallableObservation callable = Ttx::resolve_callable(semantic);
  if (callable.state == Ttx::Observation::Resolved) {
    output << "func "_view;
    append_name(output, name(semantic));
    append_layout(
        output, callable.callable.operations->parameters(callable.callable));
    output << " -> "_view;
    append_layout(
        output, callable.callable.operations->results(callable.callable));
    return True;
  }

  const ttx_interface_relation addressable =
      Ttx::relation(semantic, ttx_addressable_requirement());
  if (addressable == TTX_INTERFACE_SATISFIED ||
      addressable == TTX_INTERFACE_EQUIVALENT) {
    append_name(output, name(semantic));
    output << " : "_view;
    append_domain(output, semantic);
    return True;
  }

  const ttx_interface_relation constant =
      Ttx::relation(semantic, ttx_constant_requirement());
  if (constant == TTX_INTERFACE_SATISFIED ||
      constant == TTX_INTERFACE_EQUIVALENT) {
    output << "const "_view;
    append_name(output, name(semantic));
    return True;
  }

  const Ttx::DomainObservation domain = Ttx::resolve_domain(semantic);
  if (domain.state == Ttx::Observation::Resolved &&
      ttx_abstract_same(domain.domain, semantic)) {
    output << "domain "_view;
    append_name(output, name(semantic));
    return True;
  }
  if (name(semantic).is_empty()) {
    return False;
  }
  append_name(output, name(semantic));
  return True;
}

struct DocumentationWriter {
  ttx_bytes_sink_ops operations;
  Serialization::Stream::Textual<Memory::Managed::Bytes>* output;
  bool first;
};

static void TTX_CALL
    write_documentation(ttx_bytes_sink self, ttx_borrowed_bytes line) {
  auto& writer = select_owner<DocumentationWriter>(self.self);
  *writer.output << (writer.first ? "\n\n"_view : "\n"_view)
                 << Core::View::Bytes(line.data, line.size);
  writer.first = false;
}

static void TTX_CALL documentation_completed(ttx_bytes_sink) {}

auto Puffer::Lsp::semantic_hover(
    Memory::Allocator::Arena& arena,
    ttx_abstract semantic) -> Serialization::Json::Node {
  if (!supports(semantic.operations, sizeof(ttx_abstract_ops))) {
    return {};
  }
  Memory::Managed::Bytes buffer(arena);
  Serialization::Stream::Textual<Memory::Managed::Bytes> output(buffer);
  output << "```tetrodotoxin\n"_view;
  if (!append_identity(output, semantic)) {
    return {};
  }
  output << "\n```"_view;

  const ttx_documentation documentation =
      semantic.operations->documentation(semantic);
  if (supports(documentation.operations, sizeof(ttx_documentation_ops))) {
    DocumentationWriter writer = {
      .operations =
          {
            .header =
                {
                  .size = sizeof(ttx_bytes_sink_ops),
                  .abi_major = TTX_ABI_MAJOR,
                  .abi_minor = TTX_ABI_MINOR,
                },
            .bytes = write_documentation,
            .completed = documentation_completed,
          },
      .output = &output,
      .first = true,
    };
    const ttx_bytes_sink sink = {
      .operations = &writer.operations,
      .self = reinterpret_cast<ttx_bytes_sink_self*>(&writer),
    };
    documentation.operations->visit_bytes(documentation, sink);
  }

  return Serialization::Json::Blueprint{
    {
      {"contents"_view,
       {
         {"kind"_view, "markdown"_view},
         {"value"_view, buffer.get_view()},
       }},
    }}.construct(arena);
}
