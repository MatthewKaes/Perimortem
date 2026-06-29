// Perimortem Engine
// Copyright © Matt Kaes
//
// Fades two Perimortem icons in and out. The icon shader is
// compiled from apps/shaders/icon.ttx by the Tetrodotoxin toolchain.

#include "perimortem/core/null_terminated.hpp"
#include "perimortem/core/time.hpp"

#include "perimortem/system/file.hpp"
#include "perimortem/serialization/format/png.hpp"

#include "perimortem/graphics/image.hpp"
#include "perimortem/graphics/window.hpp"

#include "ttx/icon_shader.hpp"

using namespace Perimortem;
using namespace Perimortem::Core;
using namespace Perimortem::Graphics;

constexpr Bits_32 window_logical_size = 700;
constexpr Bits_32 icon_size_pixels = 250;
constexpr Real_64 fade_in_seconds = 1.0;
constexpr Real_64 hold_seconds = 1.5;
constexpr Real_64 fade_out_seconds = 1.0;
constexpr Real_64 total_seconds =
    fade_in_seconds + hold_seconds + fade_out_seconds;

class SplashScene {
 public:
  auto start() -> void {
    System::File icon_file;
    icon_file.read(".data/icon.png"_view);
    const Image icon_image =
        Serialization::Format::Png::decode(icon_file.get_view());

    top_icon = &Sprite2D::create(icon_image);
    top_icon->set_shader(Ttx::icon_shader);
    top_icon->set_size_pixels(icon_size_pixels, icon_size_pixels);
    top_icon->set_position(0.0f, 0.45f);

    bottom_icon = &Sprite2D::create(icon_image);
    bottom_icon->set_shader(Ttx::icon_shader);
    bottom_icon->set_size_pixels(icon_size_pixels, icon_size_pixels);
    bottom_icon->set_position(0.0f, -0.45f);

    start_time = Time::now();
  }

  auto update() -> Bool {
    const Real_64 elapsed = start_time.measure().convert_to_seconds();
    if (elapsed >= total_seconds) {
      return False;
    }

    Real_32 alpha = 0.0f;
    if (elapsed < fade_in_seconds) {
      alpha = Real_32(elapsed / fade_in_seconds);
    } else if (elapsed < fade_in_seconds + hold_seconds) {
      alpha = 1.0f;
    } else {
      alpha = Real_32(
          1.0 - (elapsed - fade_in_seconds - hold_seconds) / fade_out_seconds);
    }

    top_icon->set_alpha(alpha);
    bottom_icon->set_alpha(alpha);

    return True;
  }

 private:
  Time start_time;
  Sprite2D* top_icon = nullptr;
  Sprite2D* bottom_icon = nullptr;
};

int main() {
  Window window(window_logical_size, window_logical_size, "Perimortem");
  Scene scene(window);

  SplashScene splash;
  splash.start();

  while (scene.poll_events()) {
    if (!splash.update()) {
      break;
    }
    scene.render();
  }

  scene.wait_idle();
  return 0;
}
