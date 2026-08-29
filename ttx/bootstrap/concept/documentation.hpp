// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "ttx/concept/documentation.h"

namespace Ttx::Concept {

// Documentation is the presentation contract shared by every Abstract without
// becoming another graph identity. Making the query total lets an editor,
// formatter, or Archive ask the real semantic object for its prose without an
// optional metadata table beside the graph.
//
// Implementations can borrow authored lines, generate stable prose, or compose
// several sources. Alias can therefore place local explanation before the
// target documentation while both semantic identities remain unchanged. Empty
// Documentation means that no prose is available rather than that a semantic
// query failed.
//
// Each returned line follows the lifetime of this object. Consumers preserve
// the ordered contract instead of assuming that every implementation is one
// flat source comment, which keeps layering and reconstruction available to the
// owners that need them.
class Documentation {
 public:
  constexpr Documentation() : abi{&abi_operations} {}
  constexpr virtual ~Documentation() = default;

  static auto get_empty() -> const Documentation&;

  virtual constexpr auto get_line(Count index) const
      -> Perimortem::Core::View::Bytes {
    return {};
  }

  virtual constexpr auto line_count() const -> Count { return 0; }

  virtual constexpr auto is_empty() const -> Bool { return line_count() == 0; }

  constexpr auto get_abi() const -> const ttx_documentation* { return &abi; }

 private:
  static auto from_abi(const ttx_documentation* documentation)
      -> const Documentation&;
  static auto visit_abi(
      const ttx_documentation* documentation,
      ttx_bytes_callable* visitor) -> void;
  static const ttx_documentation_operations abi_operations;

  ttx_documentation abi;
};

}  // namespace Ttx::Concept

#define TTX_EMPTY_DOCUMENTATION()                      \
  auto get_documentation() const                       \
      -> const Ttx::Concept::Documentation& override { \
    return Ttx::Concept::Documentation::get_empty();   \
  }

#define TTX_DOCUMENTATION(expression)                  \
  constexpr auto get_documentation() const             \
      -> const Ttx::Concept::Documentation& override { \
    return expression;                                 \
  }
