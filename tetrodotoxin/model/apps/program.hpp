// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/static/union.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/model/app.hpp"
#include "tetrodotoxin/model/apps/lifecycle.hpp"
#include "tetrodotoxin/model/types/members.hpp"
#include "ttx/model/layouts/structured.hpp"

namespace Tetrodotoxin::Model::Apps {

// Program is the durable App composition owner. It is a managed state Type
// whose lifecycle, render roots, and Shader choices are direct semantic edges.
class Program final : public Model::App {
 public:
  enum class Role : Unsigned_8 {
    Start,
    Frame,
    Stop,
  };

  Program(
      Perimortem::Memory::Allocator::Arena& arena,
      Perimortem::Core::View::Bytes name,
      const Ttx::Concept::Documentation& documentation);

  constexpr auto get_name() const -> Perimortem::Core::View::Bytes override {
    return name;
  }
  constexpr auto get_documentation() const
      -> const Ttx::Concept::Documentation& override {
    return documentation;
  }
  constexpr auto get_layout() const -> const Ttx::Concept::Layout& override {
    return layout;
  }
  auto resolve() const -> const Ttx::Concept::Abstract& override;
  auto resolve_context(Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract& override;

  auto add_field(const Ttx::Model::Addressable& field) -> Bool;
  auto seal_state() -> Bool;
  auto add_lifecycle(const Lifecycle& lifecycle) -> Bool;
  auto set_role(Role role, const Lifecycle& lifecycle) -> Bool;
  auto add_render_root(const Ttx::Model::Addressable& root) -> Bool;
  auto add_binding(const Model::Render& render, const Model::Shader& shader)
      -> Bool;
  auto complete() -> Bool;

  constexpr auto get_member_count() const -> Count override {
    return members.get_root_count();
  }
  auto get_member(Count index) const -> const Ttx::Concept::Abstract& override {
    return members.get_root(index);
  }

  constexpr auto get_start() const -> const Ttx::Model::Callable& override {
    return start.get();
  }
  constexpr auto get_frame() const -> const Ttx::Model::Callable& override {
    return frame.get();
  }
  constexpr auto get_stop() const -> const Ttx::Model::Callable& override {
    return stop.get();
  }
  constexpr auto get_render_root_count() const -> Count override {
    return render_roots.get_size();
  }
  auto get_render_root(Count index) const
      -> const Ttx::Concept::Abstract& override;
  constexpr auto get_binding_count() const -> Count override {
    return bindings.get_size();
  }
  constexpr auto get_binding(Count index) const -> const Binding& override {
    return bindings.get_view()[index];
  }

 private:
  // LifecycleEdge keeps an incomplete Program's construction state distinct
  // from its semantic edges. Once assigned, the edge is always a non null
  // Reference to its ordinary Lifecycle owner.
  class LifecycleEdge {
   private:
    class Empty {};

   public:
    LifecycleEdge() : lifecycle(Empty()) {}

    auto set(const Lifecycle& value) -> Bool;
    auto is_empty() const -> Bool;
    auto get() const -> const Lifecycle&;

   private:
    Perimortem::Core::Static::Union<Empty, Ttx::Concept::Reference<Lifecycle>>
        lifecycle;
  };

  auto lifecycle_fits(Role role, const Lifecycle& lifecycle) const -> Bool;

  Perimortem::Core::View::Bytes name;
  const Ttx::Concept::Documentation& documentation;
  Types::Members members;
  Perimortem::Memory::Managed::Vector<
      Ttx::Concept::Reference<Ttx::Model::Addressable>>
      fields;
  Perimortem::Memory::Managed::Vector<
      Ttx::Concept::Reference<Ttx::Model::Addressable>>
      render_roots;
  Perimortem::Memory::Managed::Vector<Binding> bindings;
  Ttx::Model::Layouts::Structured layout;
  LifecycleEdge start;
  LifecycleEdge frame;
  LifecycleEdge stop;
  Bool state_sealed = False;
  Bool completed = False;
};

}  // namespace Tetrodotoxin::Model::Apps
