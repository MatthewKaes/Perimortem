// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/language/diagnostics.hpp"
#include "ttx/concept/abstract.hpp"
#include "ttx/concept/documentation.hpp"

namespace Tetrodotoxin::Language {

// Monograph is the retained semantic root produced by one source or restored
// payload. Concrete facts and completion diagnostics share its Arena lifetime
// without retaining a parser or an installed Dialect.
class Monograph : public Ttx::Concept::Abstract {
 public:
  virtual ~Monograph() = 0;

  Monograph(
      Perimortem::Memory::Allocator::Arena& domain,
      const Ttx::Concept::Documentation& documentation);

  Monograph(
      Perimortem::Memory::Allocator::Arena& domain,
      const Ttx::Concept::Documentation& documentation,
      Diagnostics& diagnostics);

  Monograph(const Monograph&) = delete;
  Monograph(Monograph&&) = delete;
  auto operator=(const Monograph&) -> Monograph& = delete;
  auto operator=(Monograph&&) -> Monograph& = delete;

  TTX_DOCUMENTATION(documentation);

  // Linking may connect declarations only after every source has established
  // its stable graph identities. Finalization then validates those completed
  // edges without combining the two ordered Workspace barriers.
  virtual auto link() -> Bool;
  virtual auto finalize() -> Bool;

  // The shared transaction copies diagnostic text into its Arena before
  // publishing the ordered fact. An authored failure supplies its exact Anchor
  // while a synthetic or restored failure leaves that location absent. An
  // Anchor with no valid Span is the same source free state, while an empty
  // focus Token preserves a valid Span for presentation without carets.
  // Environment remains responsible for attaching source bytes.
  auto report(
      Perimortem::Core::Option<Ttx::Lexical::Anchor> anchor,
      Perimortem::Core::View::Bytes message,
      Perimortem::Core::View::Bytes hint = {}) -> void;

  auto get_diagnostics() const -> Perimortem::Core::View::Vector<Diagnostic>;

 protected:
  // Concrete facts remain in the same lifetime domain as their Monograph so
  // graph edges never outlive their storage.
  Perimortem::Memory::Allocator::Arena& domain;

  // The opening Documentation remains attached to the semantic root because
  // later owners may need it after the parser transaction has ended.
  const Ttx::Concept::Documentation& documentation;

 private:
  Diagnostics& diagnostics;
};

}  // namespace Tetrodotoxin::Language
