// Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/library/language/flow/block.hpp"
#include "tetrodotoxin/library/language/model/pack.hpp"
#include "tetrodotoxin/library/language/parameter.hpp"
#include "tetrodotoxin/library/language/type_reference.hpp"
#include "ttx/concept/abstract.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/lexical/anchor.hpp"
#include "ttx/lexical/cursor.hpp"
#include "ttx/model/layouts/named.hpp"

namespace Tetrodotoxin::Library::Language::Flow {

// RangeLoop owns one authored `for` statement and its complete binding Layout.
// Each binding is one real Parameter identity retained by the loop, while the
// selected input Type owns which Layouts it can produce during iteration.
class RangeLoop : public Ttx::Concept::Abstract {
 public:
  // AuthoredBinding is the retained source description for one loop entry.
  // Linking replaces each delayed Type route with a real Parameter while this
  // evidence remains available for diagnostics and reflection.
  struct AuthoredBinding {
    Ttx::Lexical::Token name_token;
    Perimortem::Core::View::Bytes name;
    TypeReference type_reference;
  };

  TTX_CONTRACT(RangeLoop, Ttx::Concept::Abstract);

  static auto create_authored(
      Perimortem::Memory::Allocator::Arena& domain,
      Block& lexical_context,
      Perimortem::Core::View::Vector<AuthoredBinding> bindings,
      Model::Pack& input,
      Ttx::Lexical::Anchor anchor) -> RangeLoop&;

  auto complete_body(Block& selected, Ttx::Lexical::Anchor selected_anchor)
      -> Bool;

  RangeLoop(const RangeLoop&) = delete;
  RangeLoop(RangeLoop&&) = delete;
  auto operator=(const RangeLoop&) -> RangeLoop& = delete;
  auto operator=(RangeLoop&&) -> RangeLoop& = delete;

  auto link(Ttx::Lexical::Cursor& cursor, const Model::Type& access_scope)
      -> Bool;

  auto finalize(Ttx::Lexical::Cursor& cursor) -> void;

  TTX_NAME("For"_view);
  TTX_EMPTY_DOCUMENTATION();

  auto resolve_context(Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract& override;

  constexpr auto get_input() const -> const Model::Pack& { return input.get(); }

  constexpr auto get_input_type() const
      -> Perimortem::Core::Option<const Model::Type&> {
    return input_type.visit(
        []() -> Perimortem::Core::Option<const Model::Type&> { return {}; },
        [](const Ttx::Concept::Reference<const Model::Type>& selected)
            -> Perimortem::Core::Option<const Model::Type&> {
          return selected.get();
        });
  }

  constexpr auto get_bindings() const -> const Ttx::Concept::Layout& {
    return *binding_layout;
  }

  constexpr auto get_body() const -> const Block& { return body->get(); }

  constexpr auto get_anchor() const -> Ttx::Lexical::Anchor { return anchor; }

 private:
  RangeLoop(
      Perimortem::Memory::Allocator::Arena& domain,
      Block& lexical_context,
      Perimortem::Core::View::Vector<AuthoredBinding> bindings,
      Model::Pack& input,
      Ttx::Lexical::Anchor anchor);

  Perimortem::Memory::Allocator::Arena& domain;
  Block& lexical_context;
  Perimortem::Memory::Managed::Vector<AuthoredBinding> authored_bindings;
  Perimortem::Memory::Managed::Vector<Ttx::Concept::Reference<Parameter>>
      bindings;
  Perimortem::Memory::Managed::Vector<
      Ttx::Concept::Reference<const Ttx::Concept::Abstract>>
      binding_entries;
  Perimortem::Core::Option<Ttx::Model::Layouts::Named> binding_layout;
  Ttx::Concept::Reference<Model::Pack> input;
  Perimortem::Core::Option<Ttx::Concept::Reference<Block>> body;
  Perimortem::Core::Option<Ttx::Concept::Reference<const Model::Type>>
      input_type;
  Ttx::Lexical::Anchor anchor;
  Bool linked = False;
};

}  // namespace Tetrodotoxin::Library::Language::Flow
