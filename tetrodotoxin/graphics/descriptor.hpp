// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/object.hpp"
#include "perimortem/core/option.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"
#include "perimortem/memory/dynamic/vector.hpp"

#include "perimortem/graphics/frame/program.hpp"
#include "perimortem/graphics/frame/resource.hpp"
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
    Draw() = default;
    Draw(
        Perimortem::Graphics::Frame::Program program,
        Perimortem::Memory::Dynamic::Vector<
            Perimortem::Graphics::Frame::Resource>&& resources,
        Perimortem::Core::View::Bytes inputs,
        Count vertex_count,
        S64 z_offset)
        : program(program),
          resources(
              static_cast<Perimortem::Memory::Dynamic::Vector<
                  Perimortem::Graphics::Frame::Resource>&&>(resources)),
          inputs(inputs),
          vertex_count(vertex_count),
          z_offset(z_offset) {}

    constexpr auto get_program() const -> Perimortem::Graphics::Frame::Program {
      return program;
    }
    constexpr auto get_resources() const -> Perimortem::Core::View::Vector<
        Perimortem::Graphics::Frame::Resource> {
      return resources.get_view();
    }
    auto get_inputs() const -> Perimortem::Core::View::Bytes {
      return inputs.get_view();
    }
    constexpr auto get_vertex_count() const -> Count { return vertex_count; }
    constexpr auto get_z_offset() const -> S64 { return z_offset; }
    auto take_resources() -> Perimortem::Memory::Dynamic::Vector<
        Perimortem::Graphics::Frame::Resource>&& {
      return static_cast<Perimortem::Memory::Dynamic::Vector<
          Perimortem::Graphics::Frame::Resource>&&>(resources);
    }
    auto take_inputs() -> Perimortem::Memory::Dynamic::Bytes&& {
      return static_cast<Perimortem::Memory::Dynamic::Bytes&&>(inputs);
    }

   private:
    Perimortem::Graphics::Frame::Program program;
    Perimortem::Memory::Dynamic::Vector<Perimortem::Graphics::Frame::Resource>
        resources;
    Perimortem::Memory::Dynamic::Bytes inputs;
    Count vertex_count = 0;
    S64 z_offset = 0;
  };

  using ReadPlacement = Placement (*)(const U8*, Perimortem::Core::Object<>);
  using ReadChildCount = Count (*)(const U8*, Perimortem::Core::Object<>);
  using ReadChild = Child (*)(const U8*, Perimortem::Core::Object<>, Count);
  using ReadDrawCount = Count (*)(const U8*, Perimortem::Core::Object<>);
  using ReadDraw = Draw (*)(const U8*, Perimortem::Core::Object<>, Count);

  constexpr Descriptor(
      const U8* product,
      ReadPlacement read_placement,
      ReadChildCount read_child_count,
      ReadChild read_child,
      ReadDrawCount read_draw_count,
      ReadDraw read_draw)
      : product(product),
        read_placement(read_placement),
        read_child_count(read_child_count),
        read_child(read_child),
        read_draw_count(read_draw_count),
        read_draw(read_draw) {}

  auto placement(Perimortem::Core::Object<> object) const
      -> Perimortem::Core::Option<Placement>;
  auto child_count(Perimortem::Core::Object<> object) const -> Count;
  auto child(Perimortem::Core::Object<> object, Count index) const
      -> Perimortem::Core::Option<Child>;
  auto draw_count(Perimortem::Core::Object<> object) const -> Count;
  auto draw(Perimortem::Core::Object<> object, Count index) const
      -> Perimortem::Core::Option<Draw>;

 private:
  const U8* product;
  ReadPlacement read_placement;
  ReadChildCount read_child_count;
  ReadChild read_child;
  ReadDrawCount read_draw_count;
  ReadDraw read_draw;
};

}  // namespace Tetrodotoxin::Graphics
