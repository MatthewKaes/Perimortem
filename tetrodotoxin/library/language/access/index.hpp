// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/library/language/expression.hpp"
#include "tetrodotoxin/library/language/materializations.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/lexical/cursor.hpp"
#include "ttx/model/layouts/fluid.hpp"
#include "ttx/model/type.hpp"

namespace Tetrodotoxin::Library::Language::Access {

// Index selects one optional writable reference from Access storage. It keeps
// the exact receiver and integer Expression edges required by a later
// address consuming statement, but it never presents the selected element as
// an ordinary value Expression.
class Index : public Expression {
 public:
  TTX_CONTRACT(Index, Expression, 0xaf085ce780ce4756, 0x8d2ac2399330c9a7);

  static auto parse(
      Perimortem::Memory::Allocator::Arena& domain,
      Materializations& materializations,
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Abstract& source_context,
      Expression& receiver) -> Perimortem::Core::Option<Expression&>;

  auto link(
      Tetrodotoxin::Language::Monograph& source,
      const Ttx::Concept::Abstract& lexical_context,
      Materializations& materializations,
      Perimortem::Core::Option<const Ttx::Model::Type&> access_scope = {})
      -> Bool override;

  TTX_NAME("Index"_view);

  auto get_documentation() const -> const Ttx::Concept::Documentation& override;
  auto get_type() const -> const Ttx::Concept::Abstract& override;
  auto get_inputs() const -> const Ttx::Concept::Layout& override;

  constexpr auto get_receiver() const -> const Expression& { return receiver; }
  constexpr auto get_index() const -> const Expression& { return index; }
  auto get_element_type() const -> const Ttx::Concept::Abstract&;

 private:
  constexpr Index(
      Expression& receiver,
      Expression& index,
      Perimortem::Core::Option<Ttx::Lexical::Anchor> anchor)
      : Expression(anchor),
        receiver(receiver),
        index(index),
        input{receiver, index},
        inputs({input, 2}) {}

  Expression& receiver;
  Expression& index;
  Perimortem::Core::Option<Ttx::Concept::Reference<const Ttx::Model::Type>>
      element_type;
  Ttx::Concept::Reference<const Ttx::Concept::Abstract> input[2];
  Ttx::Model::Layouts::Fluid inputs;
};

}  // namespace Tetrodotoxin::Library::Language::Access
