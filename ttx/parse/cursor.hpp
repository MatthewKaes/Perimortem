// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"
#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "ttx/lexical/tokenizer.hpp"
#include "ttx/parse/error.hpp"

namespace Ttx::Parse {

class Cursor {
 public:
  explicit Cursor(const Lexical::Tokenizer& tokenizer);

  auto current() const -> const Lexical::Token&;
  auto consume() -> const Lexical::Token&;
  auto require(
      Lexical::Class::Type type,
      Perimortem::Core::View::Bytes message) -> const Lexical::Token&;
  auto error(Perimortem::Core::View::Bytes message) -> void;
  auto error(
      Perimortem::Core::View::Bytes message,
      Perimortem::Core::View::Bytes hint) -> void;
  auto recover_to_statement() -> void;

  auto matches(Lexical::Class::Type type) const -> Bool;
  auto has_error() const -> Bool;
  auto get_errors() const -> Perimortem::Core::View::Vector<Error>;
  auto remaining_tokens() const -> Perimortem::Core::View::Vector<Lexical::Token>;

  auto get_arena() const -> Perimortem::Memory::Allocator::Arena&;
  auto get_token_index() const -> Count;
  auto get_error_count() const -> Count;

 private:
  const Lexical::Tokenizer& tokenizer;
  Perimortem::Core::View::Vector<Lexical::Token> tokens;
  Perimortem::Memory::Managed::Vector<Error> errors;
  Count token_index = 0;
};

}  // namespace Ttx::Parse
