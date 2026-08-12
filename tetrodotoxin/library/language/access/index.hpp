// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/library/language/expression.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/lexical/cursor.hpp"
#include "ttx/model/type.hpp"

namespace Tetrodotoxin::Library::Language::Access {

// Index evaluates one optional writable element address from Access storage.
// It remains an Expression because that address is real value flow: its Pack
// contains this one producer and reports the selected element Type. Assignment
// may write through an engaged result, while ordinary consumers may read the
// same indexed value without an Option Type or shadow Addressable identity.
class Index : public Expression {
 public:
  TTX_CONTRACT(Index, Expression, 0xaf085ce780ce4756, 0x8d2ac2399330c9a7);

  static auto parse(
      Perimortem::Memory::Allocator::Arena& domain,
      Monograph& source,
      Ttx::Lexical::Cursor& cursor,
      Expression& receiver) -> Perimortem::Core::Option<Expression&>;

  auto link(
      Tetrodotoxin::Language::Monograph& source,
      const Ttx::Concept::Abstract& lexical_context,
      Perimortem::Core::Option<const Ttx::Model::Type&> access_scope = {})
      -> Bool override;

  TTX_NAME("Index"_view);
  TTX_EMPTY_DOCUMENTATION();

  auto get_type() const -> const Ttx::Concept::Abstract& override;
  auto finalize() -> void override;

  constexpr auto get_receiver() const -> const Expression& { return receiver; }
  constexpr auto get_index() const -> const Expression& { return index; }
  auto get_element_type() const -> const Ttx::Concept::Abstract&;

 private:
  constexpr Index(
      Expression& receiver,
      Expression& index,
      Perimortem::Core::Option<Ttx::Lexical::Anchor> anchor)
      : Expression(anchor), receiver(receiver), index(index) {}

  Expression& receiver;
  Expression& index;
  Perimortem::Core::Option<Ttx::Concept::Reference<const Ttx::Model::Type>>
      element_type;
};

}  // namespace Tetrodotoxin::Library::Language::Access
