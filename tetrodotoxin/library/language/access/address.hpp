// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/library/language/expression.hpp"
#include "tetrodotoxin/library/language/materializations.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/lexical/cursor.hpp"
#include "ttx/model/addressable.hpp"
#include "ttx/model/layouts/fluid.hpp"

namespace Tetrodotoxin::Library::Language::Access {

// Address evaluates one value receiver and selects one Addressable from its
// Type Layout. The selected address may use stack storage or a receiver base,
// while target generation owns its concrete representation.
class Address : public Expression {
 public:
  using ClassCatagory = Address;
  static constexpr Perimortem::System::Uuid contract_id{
    0xdf9d470a5b024335,
    0xb38fc893710f519e,
  };

  static auto parse(
      Perimortem::Memory::Allocator::Arena& domain,
      Materializations& materializations,
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Abstract& source_context,
      Expression& receiver) -> Perimortem::Core::Option<Expression&>;

  static auto create_authored(
      Perimortem::Memory::Allocator::Arena& domain,
      Perimortem::Core::View::Bytes route,
      Expression& receiver,
      Ttx::Lexical::Anchor anchor) -> Address&;

  static auto create_synthetic(
      Perimortem::Memory::Allocator::Arena& domain,
      Expression& receiver,
      const Ttx::Model::Addressable& addressable) -> Address&;

  auto link(
      Tetrodotoxin::Language::Monograph& source,
      const Ttx::Concept::Abstract& context,
      Materializations& materializations) -> Bool override;

  constexpr auto implements(Perimortem::System::Uuid requested) const
      -> Bool override {
    return requested == contract_id || Expression::implements(requested);
  }

  constexpr auto get_name() const -> Perimortem::Core::View::Bytes override {
    return route;
  }

  auto get_documentation() const -> const Ttx::Concept::Documentation& override;
  auto get_type() const -> const Ttx::Concept::Abstract& override;
  auto get_inputs() const -> const Ttx::Concept::Layout& override;

  constexpr auto get_receiver() const -> const Expression& { return receiver; }

  auto get_addressable() const
      -> Perimortem::Core::Option<const Ttx::Model::Addressable&>;

 private:
  constexpr Address(
      Perimortem::Core::View::Bytes route,
      Expression& receiver,
      Perimortem::Core::Option<
          Ttx::Concept::Reference<const Ttx::Model::Addressable>> addressable,
      Perimortem::Core::Option<Ttx::Lexical::Anchor> anchor)
      : Expression(anchor),
        route(route),
        receiver(receiver),
        addressable(addressable),
        input(receiver),
        inputs({&this->input, 1}) {}

  Perimortem::Core::View::Bytes route;
  Expression& receiver;
  Perimortem::Core::Option<
      Ttx::Concept::Reference<const Ttx::Model::Addressable>>
      addressable;
  Ttx::Concept::Reference<const Ttx::Concept::Abstract> input;
  Ttx::Model::Layouts::Fluid inputs;
};

}  // namespace Tetrodotoxin::Library::Language::Access
