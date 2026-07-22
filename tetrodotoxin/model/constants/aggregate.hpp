// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "ttx/concept/reference.hpp"
#include "ttx/model/constant.hpp"

namespace Tetrodotoxin::Model::Constants {

// Aggregate is one compile-time value of a real aggregate or vector Type. Its
// ordered child Constants are value facts. Field names and semantic structure
// remain on the Type's Layout rather than copied into this value.
class Aggregate final : public Ttx::Model::Constant {
 public:
  using ContractOwner = Aggregate;
  static constexpr Perimortem::System::Uuid contract_id{
    0x3c27ae962f26400c,
    0x9f4a3f59d68b6df1,
  };

  Aggregate(
      Perimortem::Memory::Allocator::Arena& arena,
      const Ttx::Model::Type& type,
      Perimortem::Core::View::Vector<
          Ttx::Concept::Reference<Ttx::Model::Constant>> values);

  constexpr auto implements(Perimortem::System::Uuid requested) const
      -> Bool override {
    return requested == contract_id || Constant::implements(requested);
  }
  constexpr auto get_name() const -> Perimortem::Core::View::Bytes override {
    return {};
  }
  constexpr auto resolve_context(Perimortem::Core::View::Bytes) const
      -> const Ttx::Concept::Abstract& override {
    return Ttx::Concept::Invalid::get_invalid();
  }
  constexpr auto get_type() const -> const Ttx::Concept::Abstract& override {
    return type.get();
  }
  auto equals(const Ttx::Model::Constant& rhs) const -> Bool override;

  constexpr auto get_size() const -> Count { return values.get_size(); }
  auto get_value(Count index) const -> const Ttx::Concept::Abstract&;

 private:
  Ttx::Concept::Reference<Ttx::Model::Type> type;
  Perimortem::Memory::Managed::Vector<
      Ttx::Concept::Reference<Ttx::Model::Constant>>
      values;
};

}  // namespace Tetrodotoxin::Model::Constants
