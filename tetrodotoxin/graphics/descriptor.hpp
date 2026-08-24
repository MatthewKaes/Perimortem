// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/object.hpp"
#include "perimortem/core/option.hpp"

#include "perimortem/graphics/frame/program.hpp"
#include "perimortem/graphics/frame/transform.hpp"

namespace Tetrodotoxin::Graphics {

// Descriptor is target runtime behavior derived from a Type that already
// satisfied the semantic Host Interface. Its callbacks read the real Object
// payload during collection and retain no parallel Object or Field inventory.
class Descriptor {
 public:
  class Placement {
   public:
    constexpr Placement() = default;
    constexpr Placement(
        Perimortem::Graphics::Frame::Transform transform,
        Bool visible,
        S64 z_index)
        : transform(transform), visible(visible), z_index(z_index) {}

    constexpr auto get_transform() const
        -> const Perimortem::Graphics::Frame::Transform& {
      return transform;
    }
    constexpr auto is_visible() const -> Bool { return visible; }
    constexpr auto get_z_index() const -> S64 { return z_index; }

   private:
    Perimortem::Graphics::Frame::Transform transform;
    Bool visible = True;
    S64 z_index = 0;
  };

  class Child {
   public:
    constexpr Child() = default;
    constexpr Child(
        Perimortem::Core::Object<> object,
        const Descriptor& descriptor)
        : object(object), descriptor(&descriptor) {}

    constexpr auto get_object() const -> Perimortem::Core::Object<> {
      return object;
    }
    constexpr auto get_descriptor() const -> const Descriptor* {
      return descriptor;
    }

   private:
    Perimortem::Core::Object<> object;
    const Descriptor* descriptor = nullptr;
  };

  class Draw {
   public:
    constexpr Draw() = default;
    constexpr Draw(
        Perimortem::Graphics::Frame::Program program,
        Perimortem::Core::View::Vector<Perimortem::Core::Object<>> resources,
        Perimortem::Core::View::Bytes inputs,
        Count vertex_count,
        S64 z_offset)
        : program(program),
          resources(resources),
          inputs(inputs),
          vertex_count(vertex_count),
          z_offset(z_offset) {}

    constexpr auto get_program() const -> Perimortem::Graphics::Frame::Program {
      return program;
    }
    constexpr auto get_resources() const
        -> Perimortem::Core::View::Vector<Perimortem::Core::Object<>> {
      return resources;
    }
    constexpr auto get_inputs() const -> Perimortem::Core::View::Bytes {
      return inputs;
    }
    constexpr auto get_vertex_count() const -> Count { return vertex_count; }
    constexpr auto get_z_offset() const -> S64 { return z_offset; }

   private:
    Perimortem::Graphics::Frame::Program program;
    Perimortem::Core::View::Vector<Perimortem::Core::Object<>> resources;
    Perimortem::Core::View::Bytes inputs;
    Count vertex_count = 0;
    S64 z_offset = 0;
  };

  using ReadPlacement = Placement (*)(const U8*);
  using ReadChildCount = Count (*)(const U8*);
  using ReadChild = Child (*)(const U8*, Count);
  using ReadDrawCount = Count (*)(const U8*);
  using ReadDraw = Draw (*)(const U8*, Count);

  constexpr Descriptor(
      ReadPlacement read_placement,
      ReadChildCount read_child_count,
      ReadChild read_child,
      ReadDrawCount read_draw_count,
      ReadDraw read_draw)
      : read_placement(read_placement),
        read_child_count(read_child_count),
        read_child(read_child),
        read_draw_count(read_draw_count),
        read_draw(read_draw) {}

  auto placement(const U8* payload) const
      -> Perimortem::Core::Option<Placement>;
  auto child_count(const U8* payload) const -> Count;
  auto child(const U8* payload, Count index) const
      -> Perimortem::Core::Option<Child>;
  auto draw_count(const U8* payload) const -> Count;
  auto draw(const U8* payload, Count index) const
      -> Perimortem::Core::Option<Draw>;

 private:
  ReadPlacement read_placement;
  ReadChildCount read_child_count;
  ReadChild read_child;
  ReadDrawCount read_draw_count;
  ReadDraw read_draw;
};

}  // namespace Tetrodotoxin::Graphics
