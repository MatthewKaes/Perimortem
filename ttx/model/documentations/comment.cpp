// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/model/documentations/comment.hpp"

auto Ttx::Documentations::Comment::get_empty() -> const Comment& {
  static constexpr Comment comment{Perimortem::Core::View::Bytes()};
  return comment;
}
