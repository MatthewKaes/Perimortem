// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/library/language/expression.hpp"
#include "tetrodotoxin/library/language/field.hpp"
#include "tetrodotoxin/library/language/model/pack.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "tetrodotoxin/library/language/types/object.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Library::Language {

// Initializer is the authored `new` signal for one declaration that already
// supplies an Object Type. It retains supplied values in source order while
// the Object and its Fields remain the only initialization shape.
class Initializer : public Expression {
 public:
  TTX_CONTRACT(Initializer, Expression, 0x921ccce2e4934c8d, 0x9d19d3434e18f49a);

  static auto is_next(const Ttx::Lexical::Cursor& cursor) -> Bool;

  static auto parse(
      Perimortem::Memory::Allocator::Arena& domain,
      Monograph& source,
      Ttx::Lexical::Cursor& cursor) -> Perimortem::Core::Option<Initializer&>;

  Initializer(const Initializer&) = delete;
  Initializer(Initializer&&) = delete;
  auto operator=(const Initializer&) -> Initializer& = delete;
  auto operator=(Initializer&&) -> Initializer& = delete;

  TTX_NAME("Initializer"_view);

  auto get_documentation() const
      -> const Ttx::Concept::Documentation& override {
    return Ttx::Concept::Documentation::get_empty();
  }

  auto get_type() const -> const Ttx::Concept::Abstract& override;

  auto link(
      Tetrodotoxin::Language::Monograph& source,
      const Ttx::Concept::Abstract& lexical_context,
      Perimortem::Core::Option<const Ttx::Model::Type&> access_scope = {})
      -> Bool override;

  auto finalize() -> void override;

 private:
  Initializer(
      Perimortem::Memory::Allocator::Arena& domain,
      Model::Pack& arguments,
      Perimortem::Core::Option<Ttx::Lexical::Anchor> anchor);

  auto supplies(
      Perimortem::Core::Option<const Ttx::Model::Type&> access_scope,
      const Types::Object& target,
      const Field& field) const -> Bool;

  auto has_mandatory_cycle(
      Perimortem::Core::Option<const Ttx::Model::Type&> access_scope,
      const Types::Object& target,
      Perimortem::Core::View::Vector<
          Ttx::Concept::Reference<const Types::Object>> path) const -> Bool;

  Perimortem::Memory::Allocator::Arena& domain;
  Model::Pack& arguments;
  Perimortem::Core::Option<Ttx::Concept::Reference<const Types::Object>>
      expected_type;
};

}  // namespace Tetrodotoxin::Library::Language
