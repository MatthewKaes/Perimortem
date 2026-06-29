// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/dynamic/bytes.hpp"

#include "tetrodotoxin/lexical/class.hpp"
#include "tetrodotoxin/lexical/token.hpp"
#include "tetrodotoxin/lexical/tokenizer.hpp"

namespace Tetrodotoxin::Syntax {

namespace Dialect {
class Dialect;
}

// Non-owning cursor over a Tokenizer's output that also carries the unified
// parsing state for all AST nodes.
//
// Arena allocations performed during parsing draw from the same arena that
// produced the token stream, keeping all AST data in one lifetime region.
//
// Multiple parses on the same token stream can cause fragmentation and should
// be avoided.
class Context {
 public:
  Context(
      const Lexical::Tokenizer& tokenizer,
      Perimortem::Core::View::Bytes source_path)
      : tokens(tokenizer.get_tokens()),
        source(tokenizer.get_source()),
        path(source_path),
        arena(tokenizer.get_arena()) {}

  auto get_current() const -> const Lexical::Token& { return tokens[cursor]; }

  auto get_peek(Count ahead = 1) const -> const Lexical::Token& {
    Count target = cursor + ahead;
    if (target >= tokens.get_size()) {
      return tokens[tokens.get_size() - 1];
    }

    return tokens[target];
  }

  auto advance() -> const Lexical::Token& {
    if (cursor + 1 < tokens.get_size()) {
      cursor++;
    }

    return get_current();
  }

  auto matches(Lexical::Class::Type type) const -> Bool {
    return get_current().get_class() == type;
  }

  // Advances past the current token and returns it. Does not check the class.
  auto consume() -> const Lexical::Token& {
    const Lexical::Token& token = get_current();
    advance();
    return token;
  }

  // Reports an error if the current token is not of the expected class, then
  // leaves the stream cursor at the current token.
  auto expect(Lexical::Class::Type type) -> Bool;

  // Requires a delimiter or marker token as parser precondition. Reports an
  // error if the current token is not of the expected class, then advances
  // regardless so the caller can continue recovering.
  auto require(Lexical::Class::Type type) -> Bool;

  // Skips one balanced delimited group beginning at the current token.
  auto recover_to_matching(
      Lexical::Class::Type open,
      Lexical::Class::Type close) -> void;

  // Skips the current malformed statement without consuming an enclosing
  // scope end.
  auto recover_to_statement() -> void;

  // Emits an error at the current token position.
  auto create_error(
      Perimortem::Core::View::Bytes message,
      Perimortem::Core::View::Bytes hint = Perimortem::Core::View::Bytes())
      -> void;

  auto create_error(
      const Lexical::Token& start,
      const Lexical::Token& end,
      Perimortem::Core::View::Bytes message,
      Perimortem::Core::View::Bytes hint = Perimortem::Core::View::Bytes())
      -> void;

  auto error(
      Perimortem::Core::View::Bytes message,
      Perimortem::Core::View::Bytes hint = Perimortem::Core::View::Bytes())
      -> void {
    create_error(message, hint);
  }

  auto error(
      const Lexical::Token& start,
      const Lexical::Token& end,
      Perimortem::Core::View::Bytes message,
      Perimortem::Core::View::Bytes hint = Perimortem::Core::View::Bytes())
      -> void {
    create_error(start, end, message, hint);
  }

  auto set_color_enabled(Bool value) -> void { color_enabled = value; }

  auto get_errors() const -> Perimortem::Core::View::Bytes {
    return diagnostics.get_view();
  }

  auto is_done() const -> Bool {
    return get_current().get_class() == Lexical::Class::Type::EndOfStream;
  }

  auto get_arena() const -> Perimortem::Memory::Allocator::Arena& {
    return arena;
  }

  auto get_source() const -> Perimortem::Core::View::Bytes { return source; }

  auto get_path() const -> Perimortem::Core::View::Bytes { return path; }

  auto set_dialect(const Dialect::Dialect& value) -> void {
    current_dialect = &value;
  }

  auto get_dialect() const -> const Dialect::Dialect* {
    return current_dialect;
  }

  auto get_error_count() const -> Count { return error_count; }
  auto get_cursor() const -> Count { return cursor; }

 private:
  Perimortem::Core::View::Vector<Lexical::Token> tokens;
  Perimortem::Core::View::Bytes source;
  Perimortem::Core::View::Bytes path;
  Perimortem::Memory::Allocator::Arena& arena;
  const Dialect::Dialect* current_dialect = nullptr;
  Count cursor = 0;
  Count error_count = 0;
  Bool color_enabled = True;
  Perimortem::Memory::Dynamic::Bytes diagnostics;
};

}  // namespace Tetrodotoxin::Syntax
