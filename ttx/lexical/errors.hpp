// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/vector.hpp"

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
  Errors() : source_map(arena), errors(arena) {}
  Errors(const Errors&) = delete;
  Errors(Errors&&) = delete;

  // Sets the context for errors
  constexpr auto set_source_context(
      Perimortem::Core::View::Bytes name,
      Perimortem::Core::View::Bytes text) -> void {
    proxied_context = false;
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
    error.start_token = start;
    error.end_token =
        start.is_valid() &&
                (!end.is_valid() || end.get_offset() < start.get_offset())
            ? start
            : end;

    // Lazily persist the source data and only copy it if an error was
    // generated.
    if (!proxied_context) {
      proxied_context = true;

      Info new_mapping;
      new_mapping.name = arena.proxy(current_context.name);
      new_mapping.text = arena.proxy(current_context.text);
      source_map.insert(new_mapping);
    }

    // Store the source map index that the error belongs too.
    error.source_id = source_map.get_size() - 1;
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
    Count source_id;
    Token start_token;
    Token end_token;
  };

  // Source info is copied into the error context lazily.
  struct Info {
    Perimortem::Core::View::Bytes name;
    Perimortem::Core::View::Bytes text;
  };

  // Error generation is inherently the slow path so Errors owns the lifetime of
  // any errors that it needs to own. While this does snag an entire Arena page
  // even if not used, arena pages are the most standardized Bibliotheca block
  // size so it's essentially free as long as a minimum number of Error context
  // are live at any one time.
  Perimortem::Memory::Allocator::Arena arena;
  Perimortem::Memory::Managed::Vector<Info> source_map;
  Perimortem::Memory::Managed::Vector<Error> errors;
  Info current_context = {"<Unknown>"_view, Perimortem::Core::View::Bytes()};
  Bool proxied_context = false;
};

}  // namespace Ttx::Lexical
