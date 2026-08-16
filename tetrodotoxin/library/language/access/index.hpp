// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/library/language/expression.hpp"
#include "tetrodotoxin/library/language/model/type.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Library::Language::Access {

// Index evaluates one optional writable element address from Access storage.
// It remains an Expression because that address is real value flow: its Pack
// contains this one producer and reports the selected element Type. Assignment
// may write through an engaged result, while ordinary consumers may read the
// same indexed value without an Option Type or shadow Addressable identity.
class Index : public Expression {
 public:
  TTX_CONTRACT(Index, Expression);

  static auto parse(
      const Ttx::Concept::Abstract& context,
      Ttx::Lexical::Cursor& cursor,
      Expression& receiver) -> Perimortem::Core::Option<Expression&>;

  auto link(
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Abstract& lexical_context,
      Perimortem::Core::Option<const Ttx::Concept::Abstract&> access_scope = {})
      -> Bool override;

  TTX_NAME("Index"_view);
  TTX_EMPTY_DOCUMENTATION();

  auto get_type() const -> const Ttx::Concept::Abstract& override;
  auto get_write_type(const Model::Type& access_scope) const
      -> Perimortem::Core::Option<const Model::Type&> override;
  auto finalize(Ttx::Lexical::Cursor& cursor) -> void override;

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
  Perimortem::Core::Option<Ttx::Concept::Reference<const Model::Type>>
      element_type;
};

}  // namespace Tetrodotoxin::Library::Language::Access
