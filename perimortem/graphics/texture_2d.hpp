// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/object.hpp"
#include "perimortem/core/option.hpp"

#include "perimortem/graphics/image.hpp"

namespace Perimortem::Graphics {

// Texture2D gives one Image stable rendering identity without acquiring a
// device handle. Backends key their physical resources by this Object and keep
// realization policy outside the portable graphics value.
class Texture2D {
 public:
  Texture2D();
  explicit Texture2D(const Image& image);
  Texture2D(const Texture2D& source);
  Texture2D(Texture2D&& source);
  ~Texture2D();

  auto operator=(const Texture2D& source) -> Texture2D&;
  auto operator=(Texture2D&& source) -> Texture2D&;

  auto get_image() const -> const Image&;
  auto get_size_pixels() const -> Size2D;
  auto is_drawable() const -> Bool;
  constexpr auto get_object() const -> Core::Object<> { return object; }

  static auto retain(Core::Object<> object) -> Core::Option<Texture2D>;

 private:
  class Payload {
   public:
    Image image;
  };

  explicit Texture2D(Core::Object<> object) : object(object) {}
  static auto finalize(U8* payload) -> void;
  auto get_payload() -> Payload&;
  auto get_payload() const -> const Payload&;

  static const Core::Object<>::Descriptor descriptor;
  Core::Object<> object;
};

static_assert(sizeof(Texture2D) == sizeof(U8*));
static_assert(alignof(Texture2D) == alignof(U8*));

}  // namespace Perimortem::Graphics
