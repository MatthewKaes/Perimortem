// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/library/language/flow/block.hpp"
#include "tetrodotoxin/library/language/model/addressable.hpp"
#include "tetrodotoxin/library/language/model/pack.hpp"
#include "tetrodotoxin/library/language/type_reference.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/lexical/anchor.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Library::Language::Flow {

// RangeLoop owns one authored `for` statement and is itself the loop binding.
// The binding is a read only Addressable whose exact Type must match the
// element Type of either a Range or one contiguous input. Its body resolves
// that identity through ordinary lexical lookup.
class RangeLoop : public Model::Addressable {
 public:
  TTX_CONTRACT(RangeLoop, Model::Addressable);

  static auto interpret(
      Ttx::Lexical::Cursor& cursor,
      Block& lexical_context,
      Model::Callable& function,
      const Model::Type& access_scope) -> Perimortem::Core::Option<RangeLoop&>;

  RangeLoop(const RangeLoop&) = delete;
  RangeLoop(RangeLoop&&) = delete;
  auto operator=(const RangeLoop&) -> RangeLoop& = delete;
  auto operator=(RangeLoop&&) -> RangeLoop& = delete;

  auto link(Ttx::Lexical::Cursor& cursor, const Model::Type& access_scope)
      -> Bool;

  auto finalize(Ttx::Lexical::Cursor& cursor) -> void;

  auto lower(Llvm::Builder& body) const -> Bool;

  TTX_NAME(name);
  TTX_EMPTY_DOCUMENTATION();

  auto resolve() const -> const Ttx::Concept::Abstract& override;

  auto resolve_context(Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract& override;

  constexpr auto get_type() const -> const Model::Type& override {
    return type->get();
  }

  constexpr auto get_input() const -> const Model::Pack& { return input.get(); }

  constexpr auto get_body() const -> const Block& { return body->get(); }

  constexpr auto get_anchor() const -> Ttx::Lexical::Anchor { return anchor; }

 private:
  constexpr RangeLoop(
      Block& lexical_context,
      Ttx::Lexical::Token name_token,
      Perimortem::Core::View::Bytes name,
      TypeReference type_reference,
      Model::Pack& input,
      Ttx::Lexical::Anchor anchor)
      : lexical_context(lexical_context),
        name_token(name_token),
        name(name),
        type_reference(type_reference),
        input(input),
        anchor(anchor) {}

  Block& lexical_context;
  Ttx::Lexical::Token name_token;
  Perimortem::Core::View::Bytes name;
  TypeReference type_reference;
  Ttx::Concept::Reference<Model::Pack> input;
  Perimortem::Core::Option<Ttx::Concept::Reference<Block>> body;
  Perimortem::Core::Option<Ttx::Concept::Reference<const Model::Type>> type;
  Ttx::Lexical::Anchor anchor;
  Bool linked = False;
};

}  // namespace Tetrodotoxin::Library::Language::Flow
