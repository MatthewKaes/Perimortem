// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/library/language/expression.hpp"
#include "tetrodotoxin/library/language/materializations.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/lexical/cursor.hpp"
#include "ttx/lexical/token.hpp"
#include "ttx/model/layouts/fluid.hpp"

namespace Tetrodotoxin::Library::Language::Access {

// Swizzle selects and reorders named Addressables from one receiver Type. Its
// positional result Layout retains those exact semantic identities without
// creating an aggregate Type or copying their member facts.
class Swizzle : public Expression {
 public:
  TTX_CONTRACT(Swizzle, Expression, 0xc43faea8e4984ac5, 0x81abdbb11a727513);

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

  TTX_NAME("Swizzle"_view);

  auto get_documentation() const -> const Ttx::Concept::Documentation& override;
  auto get_type() const -> const Ttx::Concept::Abstract& override;
  auto get_inputs() const -> const Ttx::Concept::Layout& override;
  auto fits(const Ttx::Model::Type& target) const -> Bool override;

  constexpr auto get_receiver() const -> const Expression& { return receiver; }
  constexpr auto get_results() const -> const Ttx::Concept::Layout& {
    return results;
  }

 private:
  Swizzle(
      Perimortem::Memory::Allocator::Arena& domain,
      Expression& receiver,
      Perimortem::Core::View::Vector<Ttx::Lexical::Token> name_tokens,
      Perimortem::Core::View::Vector<Perimortem::Core::View::Bytes> names,
      Perimortem::Core::Option<Ttx::Lexical::Anchor> anchor)
      : Expression(anchor),
        receiver(receiver),
        name_tokens(name_tokens),
        names(names),
        selected(domain),
        input(receiver),
        inputs({&this->input, 1}) {}

  Expression& receiver;
  Perimortem::Core::View::Vector<Ttx::Lexical::Token> name_tokens;
  Perimortem::Core::View::Vector<Perimortem::Core::View::Bytes> names;
  Perimortem::Memory::Managed::Vector<
      Ttx::Concept::Reference<const Ttx::Concept::Abstract>>
      selected;
  Ttx::Model::Layouts::Fluid results;
  Ttx::Concept::Reference<const Ttx::Concept::Abstract> input;
  Ttx::Model::Layouts::Fluid inputs;
  Bool linked = False;
};

}  // namespace Tetrodotoxin::Library::Language::Access
