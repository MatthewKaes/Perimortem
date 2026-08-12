// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/library/language/expression.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/lexical/cursor.hpp"
#include "ttx/model/layouts/fluid.hpp"
#include "ttx/model/type.hpp"

namespace Tetrodotoxin::Library::Language::Access {

// Type is one postfix `:: Name` Expression. It retains the receiver and exact
// authored Token without binding during parsing. Linking evaluates the
// receiver result, requires a semantic Type, and selects the next Type through
// that owner's context. Its value Type is Descriptor while get_result()
// preserves the selected semantic Type for another access operation.
class Type : public Expression {
 public:
  TTX_CONTRACT(Type, Expression, 0x39c8cead0d1e4e63, 0xa25250161e7a57cc);

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

  TTX_NAME(name);

  auto get_documentation() const -> const Ttx::Concept::Documentation& override;
  auto get_type() const -> const Ttx::Concept::Abstract& override;
  auto get_result() const -> const Ttx::Concept::Abstract& override;
  auto get_inputs() const -> const Ttx::Concept::Layout& override;

  constexpr auto get_receiver() const -> const Expression& { return receiver; }
  constexpr auto get_token() const -> Ttx::Lexical::Token { return token; }

 private:
  constexpr Type(
      Expression& receiver,
      Ttx::Lexical::Token token,
      Perimortem::Core::View::Bytes name,
      Perimortem::Core::Option<Ttx::Lexical::Anchor> anchor)
      : Expression(anchor),
        receiver(receiver),
        token(token),
        name(name),
        input(receiver),
        inputs({&this->input, 1}) {}

  Expression& receiver;
  Ttx::Lexical::Token token;
  Perimortem::Core::View::Bytes name;
  Perimortem::Core::Option<Ttx::Concept::Reference<const Ttx::Model::Type>>
      selected;
  Ttx::Concept::Reference<const Ttx::Concept::Abstract> input;
  Ttx::Model::Layouts::Fluid inputs;
};

}  // namespace Tetrodotoxin::Library::Language::Access
