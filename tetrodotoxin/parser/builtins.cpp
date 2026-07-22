// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/interpreter/builtins.hpp"

#include "ttx/concept/invalid.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Ttx::Concept;

Tetrodotoxin::Interpreter::Builtins::Builtins()
    : vec2d(arena.construct<Model::Types::NamedVector>(
          arena,
          "Vec2D"_view,
          real_32,
          2)),
      vec4d(arena.construct<Model::Types::NamedVector>(
          arena,
          "Vec4D"_view,
          real_32,
          4)),
      uvec2(arena.construct<Model::Types::NamedVector>(
          arena,
          "Uvec2"_view,
          unsigned_32,
          2)) {}

auto Tetrodotoxin::Interpreter::Builtins::get_documentation() const
    -> const Documentation& {
  return Documentation::get_empty();
}

auto Tetrodotoxin::Interpreter::Builtins::resolve_context(
    View::Bytes route) const -> const Abstract& {
  if (route == void_type.get_name()) {
    return void_type;
  }
  if (route == boolean.get_name()) {
    return boolean;
  }
  if (route == unsigned_8.get_name()) {
    return unsigned_8;
  }
  if (route == unsigned_32.get_name()) {
    return unsigned_32;
  }
  if (route == unsigned_64.get_name() || route == "Count"_view) {
    return unsigned_64;
  }
  if (route == real_32.get_name()) {
    return real_32;
  }
  if (route == real_64.get_name()) {
    return real_64;
  }
  if (route == vec2d.get_name()) {
    return vec2d;
  }
  if (route == vec4d.get_name()) {
    return vec4d;
  }
  if (route == uvec2.get_name()) {
    return uvec2;
  }
  return Invalid::get_invalid();
}
