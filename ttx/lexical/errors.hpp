// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/bytes.hpp"
#include "perimortem/memory/managed/map.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "perimortem/serialization/stream/textual.hpp"

#include "ttx/lexical/token.hpp"

namespace Ttx::Lexical {

// Collection of lexical errors surfaced across an evaluation boundary.
//
// Errors owns every diagnostic and exact source name to source text snapshot in
// its Arena. The first diagnostic for one source name copies that source once;
// every later diagnostic retains only the canonical name view.
class Errors {
 public:
  // Builds one diagnostic directly in the Errors Arena and publishes it when
  // the scope ends. The supplied source context is captured independently and
  // never replaces context owned by a Cursor or another parsing transaction.
  class Report {
   public:
    Report(
        Errors& errors,
        Perimortem::Core::View::Bytes source_name,
        Perimortem::Core::View::Bytes source_text =
            Perimortem::Core::View::Bytes(),
        Token start_token = Token(),
        Token end_token = Token());
    ~Report();
    Report(const Report&) = delete;
    Report(Report&&) = delete;
    auto operator=(const Report&) -> Report& = delete;
    auto operator=(Report&&) -> Report& = delete;

    template <typename value_type>
    auto operator<<(const value_type& value) -> Report& {
      message << value;
      return *this;
    }

    auto get_hint() -> Perimortem::Serialization::Stream::Textual<
        Perimortem::Memory::Managed::Bytes>& {
      return hint;
    }

   private:
    Errors& errors;
    Perimortem::Core::View::Bytes source_name;
    Perimortem::Core::View::Bytes source_text;
    Token start_token;
    Token end_token;
    Perimortem::Memory::Managed::Bytes message_storage;
    Perimortem::Memory::Managed::Bytes hint_storage;
    Perimortem::Serialization::Stream::Textual<
        Perimortem::Memory::Managed::Bytes>
        message;
    Perimortem::Serialization::Stream::Textual<
        Perimortem::Memory::Managed::Bytes>
        hint;
  };

  Errors() : source_map(arena), errors(arena) {}
  Errors(const Errors&) = delete;
  Errors(Errors&&) = delete;

  // Sets the context for errors
  constexpr auto set_source_context(
      Perimortem::Core::View::Bytes name,
      Perimortem::Core::View::Bytes text) -> void {
    current_context.name = name;
    current_context.text = text;
  }

  // Ends the active source borrow after a parsing transaction. Diagnostics
  // already copied their source snapshot and remain valid.
  constexpr auto clear_source_context() -> void {
    current_context.name = "<Unknown>"_view;
    current_context.text = Perimortem::Core::View::Bytes();
  }

  // Creates an error that should have a message logged at the source level.
  constexpr auto create_general_error(
      Perimortem::Core::View::Bytes message,
      Perimortem::Core::View::Bytes hint = Perimortem::Core::View::Bytes())
      -> void {
    create_token_error(Token(), message, hint);
  }

  // Creates an error associated with a single token.
  constexpr auto create_token_error(
      const Token token,
      Perimortem::Core::View::Bytes message,
      Perimortem::Core::View::Bytes hint = Perimortem::Core::View::Bytes())
      -> void {
    create_expression_error(token, token, message, hint);
  }

  // Creates an error associated with an expression which typically crosses
  // multiple tokens.
  //
  // `end` can be set to any arbitrary token, however if it's set before `start`
  // then it is clamped to start and a simple token error is created.
  constexpr auto create_expression_error(
      const Token start,
      const Token end,
      Perimortem::Core::View::Bytes message,
      Perimortem::Core::View::Bytes hint = Perimortem::Core::View::Bytes())
      -> void {
    Error error;
    error.message = arena.proxy(message);
    error.hint = arena.proxy(hint);
    error.source_name =
        retain_source(current_context.name, current_context.text);
    error.start_token = start;
    error.end_token =
        start.is_valid() &&
                (!end.is_valid() || end.get_offset() < start.get_offset())
            ? start
            : end;

    errors.insert(error);
  }

  // Centeralized render logic for rendering error messages.
  // Eventually can be moved out, but for now this keeps the logic local.
  auto render_message(Perimortem::Memory::Allocator::Arena& arena, Count index)
      const -> Perimortem::Core::View::Bytes;

  constexpr auto is_empty() const -> Bool { return get_size() == 0; }
  constexpr auto get_size() const -> Count { return errors.get_size(); }

 private:
  // Container for errors that let's us delay rendering of messages.
  struct Error {
    Perimortem::Core::View::Bytes message;
    Perimortem::Core::View::Bytes hint;
    Perimortem::Core::View::Bytes source_name;
    Token start_token;
    Token end_token;
  };

  struct SourceContext {
    Perimortem::Core::View::Bytes name;
    Perimortem::Core::View::Bytes text;
  };

  auto retain_source(
      Perimortem::Core::View::Bytes source_name,
      Perimortem::Core::View::Bytes source_text)
      -> Perimortem::Core::View::Bytes;

  auto publish_report(
      Perimortem::Core::View::Bytes source_name,
      Perimortem::Core::View::Bytes source_text,
      Perimortem::Core::View::Bytes message,
      Perimortem::Core::View::Bytes hint,
      Token start_token,
      Token end_token) -> void;

  // Error generation is inherently the slow path so Errors owns the lifetime of
  // any errors that it needs to own. While this does snag an entire Arena page
  // even if not used, arena pages are the most standardized Bibliotheca block
  // size so it's essentially free as long as a minimum number of Error context
  // are live at any one time.
  Perimortem::Memory::Allocator::Arena arena;
  Perimortem::Memory::Managed::
      Map<Perimortem::Core::View::Bytes, Perimortem::Core::View::Bytes>
          source_map;
  Perimortem::Memory::Managed::Vector<Error> errors;
  SourceContext current_context = {
    "<Unknown>"_view,
    Perimortem::Core::View::Bytes(),
  };
};

}  // namespace Ttx::Lexical
