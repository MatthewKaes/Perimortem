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
// several sources. A declaration can therefore place local explanation before
// its target's documentation without changing that target's own prose. Empty
// Documentation means that no prose is available rather than that a semantic
// query failed.
//
// Each returned line follows the lifetime of this object. Consumers preserve
// the ordered contract instead of assuming that every implementation is one
// flat source comment, which keeps layering and reconstruction available to the
// owners that need them.
class Documentation {
 public:
  // Foreign documentation already knows how to supply its lines. Wrapping it
  // in a native Documentation subclass would add an object solely to forward
  // those calls. This handle borrows the table directly, while native owners
  // expose their existing methods through the same table.
  class Handle {
   public:
    using Operations = ttx_documentation_ops;

    explicit constexpr Handle(ttx_documentation value) : value(value) {}

    constexpr auto get_abi() const -> ttx_documentation { return value; }

    auto get_line(Count index) const -> Perimortem::Core::View::Bytes {
      const auto line = value.operations->get_line(value.source, index);
      return {line.data, line.size};
    }

    auto line_count() const -> Count {
      return value.operations->line_count(value.source);
    }

    auto is_empty() const -> Bool { return line_count() == 0; }

   private:
    ttx_documentation value;
  };

  auto get_interface() const -> Handle;

  constexpr virtual ~Documentation() = default;

  static auto get_empty() -> const Documentation&;

  virtual constexpr auto get_line(Count index) const
      -> Perimortem::Core::View::Bytes {
    return {};
  }

  virtual constexpr auto line_count() const -> Count { return 0; }

  virtual constexpr auto is_empty() const -> Bool { return line_count() == 0; }
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
