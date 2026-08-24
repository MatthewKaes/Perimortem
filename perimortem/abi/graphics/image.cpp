// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/abi/graphics/image.hpp"

#include "perimortem/core/data.hpp"

#include "perimortem/abi/core/object.hpp"
#include "perimortem/graphics/image.hpp"

using namespace Perimortem;

extern "C" auto perimortem_graphics_image_decode(
    Abi::Graphics::ImageBytes source)
    -> Abi::Core::Option<Abi::Graphics::Image> {
  auto image =
      Graphics::Image::decode(Core::View::Bytes(source.data, source.size));
  if (!image) {
    return Abi::Core::Option<Abi::Graphics::Image>::create();
  }

  Core::View::Vector<Graphics::Pixel> pixels = image->get_pixels();
  perimortem_core_object_retain(
      Core::Data::cast<U8>(const_cast<Graphics::Pixel*>(pixels.get_data())));
  Abi::Graphics::Image result = Abi::Graphics::Image::create(
      pixels.get_data(), pixels.get_size(),
      Abi::Graphics::Size2D::create(image->get_width(), image->get_height()),
      U8(image->get_addressing()));
  return Abi::Core::Option<Abi::Graphics::Image>::create(result);
}
