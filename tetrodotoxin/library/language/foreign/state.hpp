// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/language/visibility.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "tetrodotoxin/library/language/type_reference.hpp"
#include "ttx/concept/authorship.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/lexical/anchor.hpp"
#include "ttx/lexical/cursor.hpp"
#include "ttx/model/addressable.hpp"

namespace Tetrodotoxin::Library::Language::Foreign {

class Surface;

// State is one external data declaration contributed by a Foreign block. It
// retains its authored Type route, source facts, and owning Surface while the
// parent Library source supplies the exact Type resolution scope. ABI remains
// the one Surface fact borrowed by every attached declaration.
class State final : public Ttx::Model::Addressable,
                    public Ttx::Concept::Authorship {
 public:
  TTX_CONTRACT(
      State,
      Ttx::Model::Addressable,
      0x76f7bf1d074e49cc,
      0x9ddcc62c6196af4b);

  static auto interpret(
      Perimortem::Memory::Allocator::Arena& domain,
      Monograph& source,
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Model::Type& resolution_scope,
      const Surface& surface) -> Perimortem::Core::Option<State&>;

  State(const State&) = delete;
  State(State&&) = delete;
  auto operator=(const State&) -> State& = delete;
  auto operator=(State&&) -> State& = delete;

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

  constexpr auto get_type() const -> const Ttx::Model::Type& override {
    return type->get();
  }

  constexpr auto get_type_reference() const -> const TypeReference& {
    return type_reference;
  }

  constexpr auto get_visibility() const -> Tetrodotoxin::Language::Visibility {
    return visibility;
  }

  auto get_abi() const -> Perimortem::Core::View::Bytes;

 private:
  constexpr State(
      const Ttx::Concept::Documentation& documentation,
      Tetrodotoxin::Language::Visibility visibility,
      Perimortem::Core::View::Bytes name,
      TypeReference type_reference,
      const Ttx::Model::Type& resolution_scope,
      const Surface& surface,
      Ttx::Lexical::Anchor anchor)
      : documentation(documentation),
        visibility(visibility),
        name(name),
        type_reference(type_reference),
        resolution_scope(resolution_scope),
        surface(surface),
        anchor(anchor) {}

  const Ttx::Concept::Documentation& documentation;
  Tetrodotoxin::Language::Visibility visibility;
  Perimortem::Core::View::Bytes name;
  TypeReference type_reference;
  const Ttx::Model::Type& resolution_scope;
  const Surface& surface;
  Ttx::Lexical::Anchor anchor;
  Perimortem::Core::Option<Ttx::Concept::Reference<const Ttx::Model::Type>>
      type;
};

}  // namespace Tetrodotoxin::Library::Language::Foreign
