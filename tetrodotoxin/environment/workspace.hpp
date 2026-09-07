// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include <memory>
#include <string>
#include <vector>

#include "tetrodotoxin/environment/toolchain.hpp"
#include "tetrodotoxin/language/query.hpp"
#include "tetrodotoxin/language/source_graph.hpp"

namespace Tetrodotoxin::Environment {

// Workspace owns stable source routes and publishes provider generations at
// those routes. This native host serializes observations, replacement, and
// final releases on its owning worker because its providers may use worker
// Arenas. A caller keeping producer borrows across replacement retains the
// supplying source graphs, or retain_sources() for the whole observation's
// closure. This keeps old Packs usable without accumulating every edit in
// Workspace.
class Workspace : public Ttx::Abstract {
 public:
  explicit Workspace(Toolchain& toolchain) : toolchain(toolchain) {}
  ~Workspace();
  Workspace(const Workspace&) = delete;
  auto operator=(const Workspace&) -> Workspace& = delete;

  auto interpret_source(
      tetrodotoxin_dialect_provider provider,
      Perimortem::Core::View::Bytes name,
      Perimortem::Core::View::Bytes path,
      Perimortem::Core::View::Bytes contents) -> Language::InterpretationState;
  auto observe_source(Perimortem::Core::View::Bytes name) const
      -> Language::SourceGraph;
  auto retain_sources() const -> std::vector<Language::SourceGraph>;
  auto source_authority(Perimortem::Core::View::Bytes name) -> ttx_abstract;
  auto source_revision(Perimortem::Core::View::Bytes name) const -> uint64_t;
  auto resolve_concept(ttx_borrowed_bytes name) const -> ttx_abstract override;
  void visit_concepts(ttx_concept_sink sink) const override;

 private:
  // Sources keep an address for routing while their graph generation changes.
  // That address denotes this authority, never the provider's private root.
  class Source : public Ttx::Abstract {
   public:
    explicit Source(Perimortem::Core::View::Bytes name);
    auto get_name() const -> Perimortem::Core::View::Bytes override;
    auto resolve(ttx_abstract self) const -> ttx_abstract override;
    auto resolve_concept(ttx_borrowed_bytes route) const
        -> ttx_abstract override;
    void visit_concepts(ttx_concept_sink sink) const override;
    std::string name;
    Language::SourceGraph graph;
    uint64_t revision = 0;
  };
  auto find(Perimortem::Core::View::Bytes name) const -> Source*;
  Toolchain& toolchain;
  std::vector<std::unique_ptr<Source>> sources;
};

}  // namespace Tetrodotoxin::Environment
