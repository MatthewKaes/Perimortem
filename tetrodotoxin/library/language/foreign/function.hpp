// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/language/visibility.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "tetrodotoxin/library/language/signature.hpp"
#include "ttx/concept/authorship.hpp"
#include "ttx/lexical/anchor.hpp"
#include "ttx/lexical/cursor.hpp"
#include "ttx/model/callable.hpp"
#include "ttx/model/type.hpp"

namespace Tetrodotoxin::Library::Language::Foreign {

class Surface;

// Function is one bodyless external Callable. It keeps the exact Library
// Signature, authored symbol spelling, and owning Surface while later target
// owners decide how the Surface ABI request is represented.
class Function final : public Ttx::Model::Callable,
                       public Ttx::Concept::Authorship {
 public:
  TTX_CONTRACT(
      Function,
      Ttx::Model::Callable,
      0x7bf36544d5f14f68,
      0xb4bd06e4f2d1af8a);

  static auto interpret(
      Perimortem::Memory::Allocator::Arena& domain,
      Monograph& source,
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Model::Type& resolution_scope,
      const Surface& surface) -> Perimortem::Core::Option<Function&>;

  Function(const Function&) = delete;
  Function(Function&&) = delete;
  auto operator=(const Function&) -> Function& = delete;
  auto operator=(Function&&) -> Function& = delete;

  auto link(Monograph& source) -> Bool;

  TTX_NAME(name);

  constexpr auto get_documentation() const
      -> const Ttx::Concept::Documentation& override {
    return documentation;
  }

  constexpr auto get_authorship() const
      -> Perimortem::Core::Option<const Ttx::Concept::Authorship&> override {
    return static_cast<const Ttx::Concept::Authorship&>(*this);
  }

  constexpr auto get_anchor() const -> Ttx::Lexical::Anchor override {
    return anchor;
  }

  constexpr auto is_published() const -> Bool override { return True; }

  auto resolve() const -> const Ttx::Concept::Abstract& override;

  auto resolve_context(Perimortem::Core::View::Bytes) const
      -> const Ttx::Concept::Abstract& override;

  constexpr auto get_parameters() const
      -> const Ttx::Concept::Layout& override {
    return signature.get_parameters();
  }

  constexpr auto get_results() const -> const Ttx::Concept::Layout& override {
    return signature.get_results();
  }

  auto get_abi() const -> Perimortem::Core::View::Bytes;

  constexpr auto get_symbol() const -> Perimortem::Core::View::Bytes {
    return symbol;
  }

 private:
  constexpr Function(
      const Ttx::Concept::Documentation& documentation,
      Perimortem::Core::View::Bytes name,
      Perimortem::Core::View::Bytes symbol,
      Signature& signature,
      const Ttx::Model::Type& resolution_scope,
      const Surface& surface,
      Ttx::Lexical::Anchor anchor)
      : documentation(documentation),
        name(name),
        symbol(symbol),
        signature(signature),
        resolution_scope(resolution_scope),
        surface(surface),
        anchor(anchor) {}

  const Ttx::Concept::Documentation& documentation;
  Perimortem::Core::View::Bytes name;
  Perimortem::Core::View::Bytes symbol;
  Signature& signature;
  const Ttx::Model::Type& resolution_scope;
  const Surface& surface;
  Ttx::Lexical::Anchor anchor;
  Bool linked = False;
};

}  // namespace Tetrodotoxin::Library::Language::Foreign
