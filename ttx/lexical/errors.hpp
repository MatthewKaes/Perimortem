// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/bytes.hpp"
#include "perimortem/memory/managed/map.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "perimortem/serialization/stream/textual.hpp"

#include "ttx/lexical/anchor.hpp"

namespace Ttx::Lexical {

// One Errors value is the publication boundary for diagnostics tied to authored
// text. Rendering happens after parsing, so the first report for an exact
// source name retains both the name and body needed to interpret every later
// Anchor. Reusing that snapshot prevents a repeated name from silently
// changing the text beneath an earlier diagnostic.
class Errors {
 public:
  // Report accumulates a complete message before publishing it at scope exit.
  // This keeps partially streamed messages out of Errors and gives every early
  // return the same publication behavior. Every context argument is explicit
  // because an absent body or range must be a deliberate choice by an owner
  // that actually has textual context.
  class Report {
   public:
    Report(
        Errors& errors,
        Perimortem::Core::View::Bytes source_name,
        Perimortem::Core::View::Bytes source_text,
        Anchor anchor);
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
    Anchor anchor;
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

  // Rendering remains delayed so callers can collect failures across one
  // transaction before choosing how to present them. The caller Arena owns only
  // the rendered view while the retained source snapshot stays canonical here.
  // A presentation owner may supply a physical display path without changing
  // the source key used by Workspace and editor tooling.
  auto render_message(
      Perimortem::Memory::Allocator::Arena& arena,
      Count index,
      Perimortem::Core::View::Bytes display_source_name = {}) const
      -> Perimortem::Core::View::Bytes;

  constexpr auto get_message(Count index) const
      -> Perimortem::Core::View::Bytes {
    return index < errors.get_size() ? errors.at(index).message
                                     : Perimortem::Core::View::Bytes();
  }

  constexpr auto get_anchor(Count index) const -> Anchor {
    return index < errors.get_size() ? errors.at(index).anchor
                                     : Anchor::create(Span());
  }

  constexpr auto get_source_name(Count index) const
      -> Perimortem::Core::View::Bytes {
    return index < errors.get_size() ? errors.at(index).source_name
                                     : Perimortem::Core::View::Bytes();
  }

  constexpr auto is_empty() const -> Bool { return get_size() == 0; }
  constexpr auto get_size() const -> Count { return errors.get_size(); }

 private:
  // Error stores Anchors against the canonical source snapshot rather than
  // retaining rendered lines that would duplicate the source for every report.
  struct Error {
    Perimortem::Core::View::Bytes message;
    Perimortem::Core::View::Bytes hint;
    Perimortem::Core::View::Bytes source_name;
    Anchor anchor;
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
      Anchor anchor) -> void;

  // Diagnostics are already the failure path, so one Arena favors stable views
  // and bulk release over reclaiming each message independently. The source map
  // also prevents repeated reports from paying for repeated source bodies.
  Perimortem::Memory::Allocator::Arena arena;
  Perimortem::Memory::Managed::
      Map<Perimortem::Core::View::Bytes, Perimortem::Core::View::Bytes>
          source_map;
  Perimortem::Memory::Managed::Vector<Error> errors;
};

}  // namespace Ttx::Lexical
