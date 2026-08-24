// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/graphics/frame/resource.hpp"

#include "perimortem/core/data.hpp"

using namespace Perimortem::Core;
using namespace Perimortem;

Graphics::Frame::Resource::Resource(Object<> object) : object(object) {
  object.retain();
}

auto Graphics::Frame::Resource::retain_image(const Graphics::Image& image)
    -> Resource {
  Core::View::Vector<Graphics::Pixel> pixels = image.get_pixels();
  BAIL_IF(pixels.is_empty());

  // Image proves that this View begins at one Object backed pixel allocation.
  // The frame retains that storage directly while dimensions and draw meaning
  // remain in the immutable Sprite input copied beside it.
  Object<> storage(
      Data::cast<U8>(const_cast<Graphics::Pixel*>(pixels.get_data())));
  return Resource(storage);
}

Graphics::Frame::Resource::Resource(const Resource& source)
    : object(source.object) {
  object.retain();
}

Graphics::Frame::Resource::Resource(Resource&& source) : object(source.object) {
  source.object = Object<>();
}

Graphics::Frame::Resource::~Resource() {
  object.release();
}

auto Graphics::Frame::Resource::operator=(const Resource& source) -> Resource& {
  if (object.get_payload() == source.object.get_payload()) {
    return *this;
  }

  source.object.retain();
  object.release();
  object = source.object;
  return *this;
}

auto Graphics::Frame::Resource::operator=(Resource&& source) -> Resource& {
  if (this == &source) {
    return *this;
  }

  object.release();
  object = source.object;
  source.object = Object<>();
  return *this;
}

auto Graphics::Frame::Resource::get_reservations() const -> Count {
  return object.get_reservations();
}

auto Graphics::Frame::Resource::get_capacity() const -> Count {
  return object.get_capacity();
}
