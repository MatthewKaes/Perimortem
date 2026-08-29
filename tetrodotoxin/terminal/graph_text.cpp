// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/terminal/graph_text.hpp"

#include "perimortem/core/null_terminated.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"
#include "perimortem/memory/dynamic/vector.hpp"

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

class GraphConceptEdge {
 public:
  constexpr GraphConceptEdge(Core::View::Bytes name, const ttx_abstract* target)
      : name(name), target(target) {}

  Core::View::Bytes name;
  const ttx_abstract* target;
};

class GraphLayoutEntry {
 public:
  constexpr GraphLayoutEntry(Core::View::Bytes name, const ttx_abstract* target)
      : name(name), target(target) {}

  Core::View::Bytes name;
  const ttx_abstract* target;
};

class GraphNode {
 public:
  constexpr explicit GraphNode(const ttx_abstract* abstract)
      : abstract(abstract) {}

  const ttx_abstract* abstract;
  Memory::Dynamic::Vector<GraphConceptEdge> concepts;
};

class GraphConceptVisitor {
 public:
  explicit GraphConceptVisitor(Memory::Dynamic::Vector<GraphConceptEdge>& edges)
      : callable{&operations}, operations{.call = call}, edges(edges) {}

  ttx_named_abstract_callable callable;

 private:
  static auto call(
      ttx_named_abstract_callable* callable,
      perimortem_bytes name,
      const ttx_abstract* target) -> void {
    auto& self = *reinterpret_cast<GraphConceptVisitor*>(callable);
    self.edges.emplace(
        GraphConceptEdge(Core::View::Bytes(name.data, name.size), target));
  }

  ttx_named_abstract_callable_operations operations;
  Memory::Dynamic::Vector<GraphConceptEdge>& edges;
};

class GraphLayoutVisitor {
 public:
  explicit GraphLayoutVisitor(
      Memory::Dynamic::Vector<GraphLayoutEntry>& entries)
      : callable{&operations}, operations{.call = call}, entries(entries) {}

  ttx_abstract_callable callable;

 private:
  static auto call(ttx_abstract_callable* callable, const ttx_abstract* entry)
      -> void {
    auto& self = *reinterpret_cast<GraphLayoutVisitor*>(callable);
    self.entries.emplace(GraphLayoutEntry({}, entry));
  }

  ttx_abstract_callable_operations operations;
  Memory::Dynamic::Vector<GraphLayoutEntry>& entries;
};

class GraphNamedLayoutVisitor {
 public:
  explicit GraphNamedLayoutVisitor(
      Memory::Dynamic::Vector<GraphLayoutEntry>& entries)
      : callable{&operations}, operations{.call = call}, entries(entries) {}

  ttx_named_abstract_callable callable;

 private:
  static auto call(
      ttx_named_abstract_callable* callable,
      perimortem_bytes name,
      const ttx_abstract* entry) -> void {
    auto& self = *reinterpret_cast<GraphNamedLayoutVisitor*>(callable);
    self.entries.emplace(
        GraphLayoutEntry(Core::View::Bytes(name.data, name.size), entry));
  }

  ttx_named_abstract_callable_operations operations;
  Memory::Dynamic::Vector<GraphLayoutEntry>& entries;
};

class GraphDocumentationVisitor {
 public:
  explicit GraphDocumentationVisitor(
      Memory::Dynamic::Vector<Core::View::Bytes>& lines)
      : callable{&operations}, operations{.call = call}, lines(lines) {}

  ttx_bytes_callable callable;

 private:
  static auto call(ttx_bytes_callable* callable, perimortem_bytes line)
      -> void {
    auto& self = *reinterpret_cast<GraphDocumentationVisitor*>(callable);
    self.lines.emplace(Core::View::Bytes(line.data, line.size));
  }

  ttx_bytes_callable_operations operations;
  Memory::Dynamic::Vector<Core::View::Bytes>& lines;
};

static auto bytes(const ttx_abstract* abstract) -> Core::View::Bytes {
  perimortem_bytes name = ttx_abstract_name(abstract);
  return Core::View::Bytes(name.data, name.size);
}

static auto compare_bytes(Core::View::Bytes left, Core::View::Bytes right)
    -> S32 {
  Count size = Core::Math::min(left.get_size(), right.get_size());
  for (Count index = 0; index < size; index++) {
    if (left[index] != right[index]) {
      return left[index] < right[index] ? -1 : 1;
    }
  }
  if (left.get_size() == right.get_size()) {
    return 0;
  }
  return left.get_size() < right.get_size() ? -1 : 1;
}

static auto find_node(
    Core::View::Vector<GraphNode> nodes,
    const ttx_abstract* abstract) -> Core::Option<Count> {
  for (Count index = 0; index < nodes.get_size(); index++) {
    if (nodes.get_data()[index].abstract == abstract) {
      return index;
    }
  }
  return {};
}

static auto retain_node(
    Memory::Dynamic::Vector<GraphNode>& nodes,
    const ttx_abstract* abstract) -> Count {
  auto existing = find_node(nodes.get_view(), abstract);
  if (existing) {
    return *existing;
  }
  Count id = nodes.get_size();
  nodes.emplace(GraphNode(abstract));
  return id;
}

static auto sort_concepts(Memory::Dynamic::Vector<GraphConceptEdge>& edges)
    -> void {
  for (Count index = 1; index < edges.get_size(); index++) {
    Count selected = index;
    while (selected != 0) {
      const GraphConceptEdge& left = edges[selected - 1];
      const GraphConceptEdge& right = edges[selected];
      S32 order = compare_bytes(left.name, right.name);
      if (order == 0) {
        order = compare_bytes(bytes(left.target), bytes(right.target));
      }
      if (order <= 0) {
        break;
      }
      Core::Data::swap(edges[selected - 1], edges[selected]);
      selected--;
    }
  }
}

static auto collect_layout(
    const ttx_layout* layout,
    Memory::Dynamic::Vector<GraphLayoutEntry>& entries) -> void {
  ttx_named_layout_view named;
  if (ttx_named_layout_prove(layout, &named)) {
    GraphNamedLayoutVisitor visitor(entries);
    ttx_named_layout_visit(&named, &visitor.callable);
    return;
  }
  GraphLayoutVisitor visitor(entries);
  ttx_layout_visit(layout, &visitor.callable);
}

static auto retain_layout(
    Memory::Dynamic::Vector<GraphNode>& nodes,
    const ttx_layout* layout) -> void {
  Memory::Dynamic::Vector<GraphLayoutEntry> entries;
  collect_layout(layout, entries);
  for (const GraphLayoutEntry& entry : entries.get_view()) {
    retain_node(nodes, entry.target);
  }
}

static auto explore(Memory::Dynamic::Vector<GraphNode>& nodes) -> void {
  for (Count index = 0; index < nodes.get_size(); index++) {
    const ttx_abstract* abstract = nodes[index].abstract;
    const ttx_abstract* resolved = ttx_abstract_resolve(abstract);
    ttx_unknown_view unknown;
    retain_node(nodes, resolved);
    retain_node(
        nodes, ttx_unknown_prove(resolved, &unknown)
                   ? ttx_unknown()
                   : ttx_abstract_type(abstract));

    Memory::Dynamic::Vector<GraphConceptEdge> discovered;
    GraphConceptVisitor visitor(discovered);
    ttx_abstract_visit_concepts(abstract, &visitor.callable);
    sort_concepts(discovered);
    for (const GraphConceptEdge& edge : discovered.get_view()) {
      retain_node(nodes, edge.target);
    }

    ttx_type_view type;
    if (ttx_type_prove(abstract, &type)) {
      retain_layout(nodes, ttx_type_layout(&type));
    }
    ttx_callable_view callable;
    if (ttx_callable_prove(abstract, &callable)) {
      retain_layout(nodes, ttx_callable_parameters(&callable));
      retain_layout(nodes, ttx_callable_results(&callable));
    }
    nodes[index].concepts =
        static_cast<Memory::Dynamic::Vector<GraphConceptEdge>&&>(discovered);
  }
}

static auto write_bytes(Memory::Dynamic::Bytes& output, Core::View::Bytes value)
    -> void {
  static constexpr U8 digits[] = "0123456789abcdef";
  output.append('x');
  for (Count index = 0; index < value.get_size(); index++) {
    U8 byte = value[index];
    output.append(digits[byte >> 4]);
    output.append(digits[byte & 0x0f]);
  }
}

static auto write_id(
    Serialization::Stream::Textual<Memory::Dynamic::Bytes>& output,
    Core::View::Vector<GraphNode> nodes,
    const ttx_abstract* abstract) -> void {
  output << U64(*find_node(nodes, abstract));
}

static auto write_layout(
    Memory::Dynamic::Bytes& bytes_output,
    Serialization::Stream::Textual<Memory::Dynamic::Bytes>& output,
    Core::View::Vector<GraphNode> nodes,
    Core::View::Bytes role,
    const ttx_layout* layout) -> void {
  Memory::Dynamic::Vector<GraphLayoutEntry> entries;
  collect_layout(layout, entries);
  output << "  layout "_view << role << " "_view << U64(entries.get_size())
         << "\n"_view;
  for (const GraphLayoutEntry& entry : entries.get_view()) {
    output << "    entry "_view;
    write_bytes(bytes_output, entry.name);
    output << " "_view;
    write_id(output, nodes, entry.target);
    output << "\n"_view;
  }
}

static auto write_contracts(
    Serialization::Stream::Textual<Memory::Dynamic::Bytes>& output,
    const ttx_abstract* abstract) -> void {
  output << "  contracts abstract"_view;
  ttx_constant_view constant;
  if (ttx_constant_prove(abstract, &constant)) {
    output << " constant"_view;
  }
  ttx_alias_view alias;
  if (ttx_alias_prove(abstract, &alias)) {
    output << " alias"_view;
  }
  ttx_type_view type;
  if (ttx_type_prove(abstract, &type)) {
    output << " type"_view;
  }
  ttx_addressable_view addressable;
  if (ttx_addressable_prove(abstract, &addressable)) {
    output << " addressable"_view;
  }
  ttx_callable_view callable;
  if (ttx_callable_prove(abstract, &callable)) {
    output << " callable"_view;
  }
  ttx_unknown_view unknown;
  if (ttx_unknown_prove(abstract, &unknown)) {
    output << " unknown"_view;
  }
  ttx_none_view none;
  if (ttx_none_prove(abstract, &none)) {
    output << " none"_view;
  }
  output << "\n"_view;
}

static auto serialize(
    Core::View::Bytes source,
    const ttx_abstract* dialect,
    const ttx_abstract* root,
    Core::View::Vector<GraphNode> nodes) -> Memory::Dynamic::Bytes {
  Memory::Dynamic::Bytes bytes_output;
  Serialization::Stream::Textual<Memory::Dynamic::Bytes> output(bytes_output);
  output << "ttx.graph 1\nsource "_view;
  write_bytes(bytes_output, source);
  output << "\ndialect "_view;
  write_id(output, nodes, dialect);
  output << "\nroot "_view;
  write_id(output, nodes, root);
  output << "\nnodes "_view << U64(nodes.get_size()) << "\n"_view;

  for (Count id = 0; id < nodes.get_size(); id++) {
    const GraphNode& node = nodes.get_data()[id];
    const ttx_abstract* abstract = node.abstract;
    output << "node "_view << U64(id) << "\n  name "_view;
    write_bytes(bytes_output, bytes(abstract));
    output << "\n"_view;
    write_contracts(output, abstract);
    output << "  resolve "_view;
    write_id(output, nodes, ttx_abstract_resolve(abstract));
    output << "\n  type "_view;
    const ttx_abstract* resolved = ttx_abstract_resolve(abstract);
    ttx_unknown_view unknown;
    write_id(
        output, nodes,
        ttx_unknown_prove(resolved, &unknown) ? ttx_unknown()
                                              : ttx_abstract_type(abstract));

    Memory::Dynamic::Vector<Core::View::Bytes> documentation;
    GraphDocumentationVisitor documentation_visitor(documentation);
    ttx_documentation_visit(
        ttx_abstract_documentation(abstract), &documentation_visitor.callable);
    output << "\n  documentation "_view << U64(documentation.get_size())
           << "\n"_view;
    for (Core::View::Bytes line : documentation.get_view()) {
      output << "    line "_view;
      write_bytes(bytes_output, line);
      output << "\n"_view;
    }

    output << "  concepts "_view << U64(node.concepts.get_size()) << "\n"_view;
    for (const GraphConceptEdge& edge : node.concepts.get_view()) {
      output << "    concept "_view;
      write_bytes(bytes_output, edge.name);
      output << " "_view;
      write_id(output, nodes, edge.target);
      output << "\n"_view;
    }
    ttx_type_view type;
    if (ttx_type_prove(abstract, &type)) {
      write_layout(
          bytes_output, output, nodes, "type"_view, ttx_type_layout(&type));
    }
    ttx_callable_view callable;
    if (ttx_callable_prove(abstract, &callable)) {
      write_layout(
          bytes_output, output, nodes, "parameters"_view,
          ttx_callable_parameters(&callable));
      write_layout(
          bytes_output, output, nodes, "results"_view,
          ttx_callable_results(&callable));
    }
    output << "end\n"_view;
  }
  return bytes_output;
}

auto Tetrodotoxin::Terminal::GraphText::write(
    Memory::Allocator::Arena& arena,
    Core::View::Bytes source,
    const ttx_abstract* dialect,
    const ttx_abstract* root,
    const ttx_abstract* graph) -> const Tetrodotoxin::Language::Product& {
  Memory::Dynamic::Vector<GraphNode> nodes;
  retain_node(nodes, graph);
  retain_node(nodes, dialect);
  retain_node(nodes, root);
  explore(nodes);
  Memory::Dynamic::Bytes text =
      serialize(source, dialect, root, nodes.get_view());
  return Tetrodotoxin::Language::Product::create(
      arena, "graph.ttxg"_view, text.get_view());
}
