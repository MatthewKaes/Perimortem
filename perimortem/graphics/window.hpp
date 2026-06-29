// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include <pthread.h>

#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/thread/worker.hpp"

#include "perimortem/graphics/image.hpp"
#include "perimortem/graphics/ttx_shader.hpp"

namespace Perimortem::Graphics {

class Scene;
class Window;

class Sprite2D {
 public:
  static auto create(const Image& image) -> Sprite2D&;

  Sprite2D() = default;
  Sprite2D(const Sprite2D&) = delete;
  auto operator=(const Sprite2D&) -> Sprite2D& = delete;

  auto set_shader(const Ttx::Shader& shader) -> void;
  auto set_position(Real_32 x, Real_32 y) -> void;
  auto set_size_pixels(Real_32 width, Real_32 height) -> void;
  auto set_alpha(Real_32 value) -> void;

 private:
  friend class Scene;
  friend class Window;

  auto initialize(Window* owner, Count image_index) -> void;

  Window* window = nullptr;
  Count index = 0;
};

class Window {
 public:
  static constexpr Count max_sprites_2d = 64;

  Window(Bits_32 width, Bits_32 height, const char* title);
  ~Window();
  Window(const Window&) = delete;
  auto operator=(const Window&) -> Window& = delete;

  auto poll_events() -> Bool;
  auto render() -> void;
  auto wait_idle() -> void;

  auto get_width() -> Bits_32;
  auto get_height() -> Bits_32;

 private:
  friend class Scene;
  friend class Sprite2D;

  enum class CommandKind : Bits_8 {
    Stop,
    CreateSprite2D,
    SetSprite2DShader,
    SetSprite2DPosition,
    SetSprite2DSize,
    SetSprite2DAlpha,
    PollEvents,
    Render,
    WaitIdle,
    GetWidth,
    GetHeight,
  };

  class Command {
   public:
    CommandKind kind = CommandKind::WaitIdle;
    Count sprite_index = 0;
    const Image* image = nullptr;
    const Ttx::Shader* shader = nullptr;
    Real_32 x = 0.0f;
    Real_32 y = 0.0f;
    Bool bool_result = False;
    Bits_32 bits_32_result = 0;
  };

  class Runtime {
   public:
    Runtime(Bits_32 width, Bits_32 height, const char* title);
    ~Runtime();
    Runtime(const Runtime&) = delete;
    auto operator=(const Runtime&) -> Runtime& = delete;

    auto operator()() -> void;
    auto wait_until_ready() -> void;
    auto execute(Command& command) -> void;

   private:
    auto wait_for_command() -> Command;
    auto finish_command(const Command& command) -> void;

    Bits_32 width = 0;
    Bits_32 height = 0;
    const char* title = nullptr;
    pthread_mutex_t mutex;
    pthread_cond_t command_ready;
    pthread_cond_t command_finished;
    pthread_cond_t runtime_ready;
    Command pending_command;
    Bool has_command = False;
    Bool command_complete = False;
    Bool ready = False;
  };

  auto create_sprite_2d(const Image& image) -> Sprite2D&;
  auto set_sprite_2d_shader(Count sprite_index, const Ttx::Shader& shader)
      -> void;
  auto set_sprite_2d_position(Count sprite_index, Real_32 x, Real_32 y) -> void;
  auto set_sprite_2d_size(Count sprite_index, Real_32 width, Real_32 height)
      -> void;
  auto set_sprite_2d_alpha(Count sprite_index, Real_32 value) -> void;

  Runtime runtime;
  Core::Thread::Worker worker;
  Core::Static::Vector<Sprite2D, max_sprites_2d> sprites_2d;
  Count sprite_2d_count = 0;
};

class Scene {
 public:
  Scene(Window& window);
  ~Scene();
  Scene(const Scene&) = delete;
  auto operator=(const Scene&) -> Scene& = delete;

  auto poll_events() -> Bool;
  auto render() -> void;
  auto wait_idle() -> void;

  auto get_width() -> Bits_32;
  auto get_height() -> Bits_32;

 private:
  friend class Sprite2D;

  static auto current() -> Scene&;
  auto create_sprite_2d(const Image& image) -> Sprite2D&;

  Window* window = nullptr;
  Scene* previous = nullptr;
};

}  // namespace Perimortem::Graphics
