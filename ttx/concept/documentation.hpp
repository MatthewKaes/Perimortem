// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include <cstdint>
#include <vector>

#include "perimortem/core/view/bytes.hpp"

#include "ttx/abi.h"

namespace Ttx {

// Documentation remains support data because prose describes a semantic owner
// without becoming another graph identity. The producer controls line
// boundaries and lifetime, while a consumer receives borrowed bytes through a
// synchronous visitor and can copy only the presentation it needs.
class Documentation {
 public:
  constexpr Documentation()
      : binding({
          .operations =
              {
                .header =
                    {
                      .size = sizeof(ttx_documentation_ops),
                      .abi_major = TTX_ABI_MAJOR,
                      .abi_minor = TTX_ABI_MINOR,
                    },
                .size = get_size,
                .visit_bytes = visit_bytes,
              },
        }) {}
  virtual ~Documentation() = default;

  Documentation(const Documentation&) = delete;
  Documentation(Documentation&&) = delete;
  auto operator=(const Documentation&) -> Documentation& = delete;
  auto operator=(Documentation&&) -> Documentation& = delete;

  static auto get_empty() -> const Documentation&;

  auto get_abi() const -> ttx_documentation {
    return {
      .operations = &binding.operations,
      .self = reinterpret_cast<ttx_documentation_self*>(
          const_cast<Documentation*>(this)),
    };
  }

  virtual constexpr auto get_line(Count) const
      -> Perimortem::Core::View::Bytes {
    return {};
  }
  virtual constexpr auto line_count() const -> Count { return 0; }
  constexpr auto is_empty() const -> Bool { return line_count() == 0; }

  virtual auto size() const -> uint64_t;
  virtual void visit(ttx_bytes_sink result) const;

 private:
  struct Binding {
    ttx_documentation_ops operations;
  };

  static auto select(ttx_documentation self) -> const Documentation&;
  static auto TTX_CALL get_size(ttx_documentation self) -> uint64_t;
  static void TTX_CALL
      visit_bytes(ttx_documentation self, ttx_bytes_sink result);

  const Binding binding;
};

// Lines is the owning reference implementation used when documentation does
// not already live in a source Arena. Each byte sequence is copied once, then
// lent unchanged to every visitor for this object's lifetime.
class DocumentationLines final : public Documentation {
 public:
  explicit DocumentationLines(std::vector<std::vector<uint8_t>> lines);

  auto size() const -> uint64_t override;
  void visit(ttx_bytes_sink result) const override;
  auto get_line(Count index) const -> Perimortem::Core::View::Bytes override;
  auto line_count() const -> Count override;

 private:
  const std::vector<std::vector<uint8_t>> lines;
};

}  // namespace Ttx

// Existing C++ graph owners use the Concept namespace for historical source
// compatibility. Both spellings name the one canonical support type above;
// no wrapper or second ABI identity is constructed.
namespace Ttx::Concept {
using Documentation = Ttx::Documentation;
}

#define TTX_EMPTY_DOCUMENTATION()                                        \
  auto get_documentation() const -> const Ttx::Documentation& override { \
    return Ttx::Documentation::get_empty();                              \
  }

#define TTX_DOCUMENTATION(expression)         \
  constexpr auto get_documentation() const    \
      -> const Ttx::Documentation& override { \
    return expression;                        \
  }
