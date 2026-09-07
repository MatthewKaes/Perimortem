// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include <utility>

#include "tetrodotoxin/language/provider.h"

namespace Tetrodotoxin::Language {

// Keep this ownership value beside any Pack that borrows a source's producers
// across replacement. It retains the provider generation, not a copy of its
// graph. Dropping the last copy lets that provider reclaim its own storage.
// The installed provider and borrowed Workspace context outlive these values.
// Retaining a generation preserves existing borrows. A new query after an edit
// captures a new source closure before following live cross source references.
class SourceGraph {
 public:
  SourceGraph() = default;
  static auto adopt(tetrodotoxin_source_graph graph) -> SourceGraph {
    SourceGraph owner;
    owner.graph = graph;
    return owner;
  }
  SourceGraph(const SourceGraph& other) : graph(other.graph) {
    if (graph.operations) {
      graph.operations->retain(graph.self);
    }
  }
  SourceGraph(SourceGraph&& other) noexcept
      : graph(std::exchange(other.graph, {})) {}
  auto operator=(SourceGraph other) noexcept -> SourceGraph& {
    std::swap(graph, other.graph);
    return *this;
  }
  ~SourceGraph() {
    if (graph.operations) {
      graph.operations->release(graph.self);
    }
  }
  explicit operator bool() const { return graph.operations != nullptr; }
  auto get() const -> tetrodotoxin_source_graph { return graph; }
  auto root() const -> ttx_abstract {
    return graph.operations ? graph.operations->root(graph.self)
                            : ttx_unknown();
  }

 private:
  tetrodotoxin_source_graph graph = {};
};

}  // namespace Tetrodotoxin::Language
