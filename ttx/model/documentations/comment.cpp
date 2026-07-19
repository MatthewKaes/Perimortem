// Perimortem Engine
// Copyright © Matt Kaes

#include "ttx/model/documentations/comment.hpp"

using namespace Ttx::Model;

auto Documentations::Comment::get_empty() -> const Comment& {
  static constexpr Comment comment{Perimortem::Core::View::Bytes()};
  return comment;
}
