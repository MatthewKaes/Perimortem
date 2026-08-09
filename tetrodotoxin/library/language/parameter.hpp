// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/library/language/signature.hpp"
#include "ttx/model/addressable.hpp"

namespace Tetrodotoxin::Library::Language {

// Parameter is one named Function input Addressable. Signature owns its
// authored source slot while Parameter retains only that stable source edge
// and the exact resolved TTX Type required by the semantic graph.
class Parameter : public Ttx::Model::Addressable {
 public:
  using ClassCatagory = Parameter;
  static constexpr Perimortem::System::Uuid contract_id{
    0xd422d050fce343c0,
    0xbc6efef9f6271d15,
  };

  static auto create_authored(
      Perimortem::Memory::Allocator::Arena& domain,
      const Signature& signature,
      Count index,
      const Ttx::Model::Type& type) -> Perimortem::Core::Option<Parameter&>;

  Parameter(const Parameter&) = delete;
  Parameter(Parameter&&) = delete;
  auto operator=(const Parameter&) -> Parameter& = delete;
  auto operator=(Parameter&&) -> Parameter& = delete;

  constexpr auto implements(Perimortem::System::Uuid requested) const
      -> Bool override {
    return requested == contract_id ||
           Ttx::Model::Addressable::implements(requested);
  }

  constexpr auto get_name() const -> Perimortem::Core::View::Bytes override {
    return signature.get_parameter_name(index);
  }

  auto get_documentation() const -> const Ttx::Concept::Documentation& override;

  auto get_type() const -> const Ttx::Model::Type& override;

  auto get_name_token() const -> Ttx::Lexical::Token;

  auto get_span() const -> Ttx::Lexical::Span;

  auto get_type_span() const -> Ttx::Lexical::Span;

 private:
  constexpr Parameter(
      const Signature& signature,
      Count index,
      const Ttx::Model::Type& type)
      : signature(signature), index(index), type(type) {}

  const Signature& signature;
  Count index;
  const Ttx::Model::Type& type;
};

}  // namespace Tetrodotoxin::Library::Language
