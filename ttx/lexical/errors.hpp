// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "ttx/lexical/source.hpp"
#include "ttx/lexical/token.hpp"

namespace Ttx::Lexical {

// Collection of lexical errors surfaced across an evaluation boundary.
//
// Errors stores views and token references. The caller provides an arena for
// the collection storage, but the referenced source and token data still belong
// to the transaction that produced the errors. Consumers that want diagnostics
// after ending that transaction must explicitly migrate them into a longer
// lifetime.
class Errors {
 public:
  // An error in some source-related context.
  class Error {
   public:
    Error() = default;
    // Generates a generic error about the source with no logical position.
    Error(
        Perimortem::Core::View::Bytes source_path,
        Perimortem::Core::View::Bytes source,
        Perimortem::Core::View::Bytes message,
        Perimortem::Core::View::Bytes hint = Perimortem::Core::View::Bytes())
        : source_path(source_path),
          source(source),
          message(message),
          hint(hint),
          start_token(nullptr),
          end_token(nullptr) {}

    // Creates an error localized to a singular token.
    Error(
        const Lexical::Token& token,
        Perimortem::Core::View::Bytes source_path,
        Perimortem::Core::View::Bytes source,
        Perimortem::Core::View::Bytes message,
        Perimortem::Core::View::Bytes hint = Perimortem::Core::View::Bytes())
        : source_path(source_path),
          source(source),
          message(message),
          hint(hint),
          start_token(&token),
          end_token(&token) {}

    // Creates an error that highlights a range of tokens for issues that cross
    // multiple tokens such as an expression.
    Error(
        const Lexical::Token& start_token,
        const Lexical::Token& end_token,
        Perimortem::Core::View::Bytes source_path,
        Perimortem::Core::View::Bytes source,
        Perimortem::Core::View::Bytes message,
        Perimortem::Core::View::Bytes hint = Perimortem::Core::View::Bytes())
        : source_path(source_path),
          source(source),
          message(message),
          hint(hint),
          start_token(&start_token),
          end_token(&end_token) {}

    constexpr auto get_source_path() const -> Perimortem::Core::View::Bytes {
      return source_path;
    }

    constexpr auto get_source() const -> Perimortem::Core::View::Bytes {
      return source;
    }

    constexpr auto has_tokens() const -> Bool { return start_token != nullptr; }
    constexpr auto get_start_token() const -> const Lexical::Token& {
      return *start_token;
    }
    constexpr auto get_end_token() const -> const Lexical::Token& {
      return *end_token;
    }

    constexpr auto get_message() const -> Perimortem::Core::View::Bytes {
      return message;
    }

    constexpr auto get_hint() const -> Perimortem::Core::View::Bytes {
      return hint;
    }

    constexpr auto is_empty() const -> Bool { return message.is_empty(); }

   private:
    Perimortem::Core::View::Bytes source_path;
    Perimortem::Core::View::Bytes source;
    Perimortem::Core::View::Bytes message;
    Perimortem::Core::View::Bytes hint;
    const Lexical::Token* start_token = nullptr;
    const Lexical::Token* end_token = nullptr;
  };

  explicit Errors(Perimortem::Memory::Allocator::Arena& arena)
      : errors(arena) {}
  Errors(const Errors&) = delete;
  Errors(Errors&&) = delete;

  auto insert(const Error& error) -> void;
  auto insert(
      Source source,
      Perimortem::Core::View::Bytes message,
      Perimortem::Core::View::Bytes hint = Perimortem::Core::View::Bytes())
      -> void;
  auto insert(
      const Token& token,
      Source source,
      Perimortem::Core::View::Bytes message,
      Perimortem::Core::View::Bytes hint = Perimortem::Core::View::Bytes())
      -> void;
  auto insert_range(
      const Token& start,
      const Token& end,
      Source source,
      Perimortem::Core::View::Bytes message,
      Perimortem::Core::View::Bytes hint = Perimortem::Core::View::Bytes())
      -> void;

  constexpr auto has_errors() const -> Bool { return errors.get_size() != 0; }
  constexpr auto is_empty() const -> Bool { return errors.is_empty(); }
  constexpr auto get_size() const -> Count { return errors.get_size(); }
  constexpr auto get_view() const -> Perimortem::Core::View::Vector<Error> {
    return errors;
  }

  constexpr operator Perimortem::Core::View::Vector<Error>() const {
    return get_view();
  }

 private:
  Perimortem::Memory::Managed::Vector<Error> errors;
};

}  // namespace Ttx::Lexical
