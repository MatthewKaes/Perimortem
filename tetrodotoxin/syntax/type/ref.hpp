// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"

#include "tetrodotoxin/lexical/class.hpp"
#include "tetrodotoxin/syntax/context.hpp"

namespace Tetrodotoxin::Syntax::Type {

// A type annotation such as `Bits_32`, `Vec[Bits_8, 8]`, or `Graphics.Image`.
//
// name_path holds the dot-separated segments (e.g. ["Graphics", "Image"]).
// params holds generic arguments; a numeric size literal (e.g. the 8 in
// Vec[Bits_8, 8]) is stored as a Ref with is_size_literal == True.
class Ref {
 public:
  struct Segment {
    Lexical::Class klass = Lexical::Class::Type::EndOfStream;
    Perimortem::Core::View::Bytes text;
  };

  static auto parse(Context& ctx) -> Ref;

  // First segment of the path. Empty means this Ref failed to parse.
  auto get_name() const -> Perimortem::Core::View::Bytes {
    return !name_path.is_empty() ? name_path[0]
                                 : Perimortem::Core::View::Bytes();
  }

  auto get_second_name() const -> Perimortem::Core::View::Bytes {
    return name_path.get_size() < 2 ? Perimortem::Core::View::Bytes()
                                    : name_path[1];
  }

  auto get_last_name() const -> Perimortem::Core::View::Bytes {
    return name_path.is_empty() ? Perimortem::Core::View::Bytes()
                                : name_path[name_path.get_size() - 1];
  }

  auto get_name_path() const
      -> Perimortem::Core::View::Vector<Perimortem::Core::View::Bytes> {
    return name_path;
  }

  auto get_segments() const -> Perimortem::Core::View::Vector<Segment> {
    return segments;
  }

  auto get_params() const -> Perimortem::Core::View::Vector<Ref> {
    return params;
  }

  auto is_generic() const -> Bool { return !params.is_empty(); }
  auto is_empty() const -> Bool { return name_path.get_size() == 0; }
  auto is_size_literal() const -> Bool { return size_literal; }

 private:
  Perimortem::Core::View::Vector<Perimortem::Core::View::Bytes> name_path;
  Perimortem::Core::View::Vector<Segment> segments;
  Perimortem::Core::View::Vector<Ref> params;
  Bool size_literal = False;
};

}  // namespace Tetrodotoxin::Syntax::Type
