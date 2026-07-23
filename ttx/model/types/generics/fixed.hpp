// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/static/vector.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "ttx/concept/invalid.hpp"
#include "ttx/model/documentations/comment.hpp"
#include "ttx/model/layouts/ranged.hpp"
#include "ttx/model/types/generics.hpp"

namespace Ttx::Model::Types::Generics {

// Fixed is the homogeneous static-range formula. Its materialized Types retain
// the element identity and non-negative extent, then expose that shape through
// the common Ranged Layout instead of allocating one edge per position.
class Fixed final : public Generic {
 public:
  static constexpr Perimortem::Core::View::Bytes name = "Fixed"_view;

  class Type final : public Ttx::Model::Type {
   public:
    using ContractOwner = Type;
    static constexpr Perimortem::System::Uuid contract_id{
      0xf1d212690ed5471f,
      0x8f47246bd80d2cbe,
    };

    Type(
        Perimortem::Core::View::Bytes name,
        const Ttx::Model::Type& element,
        ::Signed_64 extent)
        : name(name), layout(element, Count(extent)) {
      arguments[0] = Generic::Argument(element);
      arguments[1] = Generic::Argument(extent);
    }

    constexpr auto implements(Perimortem::System::Uuid requested) const
        -> Bool override {
      return requested == contract_id ||
             Ttx::Model::Type::implements(requested);
    }

    constexpr auto get_name() const -> Perimortem::Core::View::Bytes override {
      return name;
    }

    constexpr auto get_documentation() const
        -> const Concept::Documentation& override {
      return documentation;
    }

    constexpr auto resolve_context(Perimortem::Core::View::Bytes) const
        -> const Concept::Abstract& override {
      return Concept::Invalid::get_invalid();
    }

    constexpr auto get_layout() const -> const Layouts::Ranged& override {
      return layout;
    }

    constexpr auto get_arguments() const
        -> Perimortem::Core::View::Vector<Generic::Argument> {
      return arguments;
    }

    constexpr auto get_element_type() const -> const Ttx::Model::Type& {
      return *arguments[0].find<const Ttx::Model::Type&>();
    }

    constexpr auto get_extent() const -> ::Signed_64 {
      return *arguments[1].find<::Signed_64>();
    }

   private:
    Perimortem::Core::View::Bytes name;
    Perimortem::Core::Static::Vector<Generic::Argument, 2> arguments;
    Layouts::Ranged layout;
  };

  constexpr auto get_name() const -> Perimortem::Core::View::Bytes override {
    return name;
  }

  constexpr auto get_documentation() const
      -> const Concept::Documentation& override {
    return documentation;
  }

  constexpr auto resolve_context(Perimortem::Core::View::Bytes) const
      -> const Concept::Abstract& override {
    return Concept::Invalid::get_invalid();
  }

  constexpr auto get_parameterization() const
      -> Perimortem::Core::View::Vector<Parameters> override {
    return parameterization;
  }

 private:
  auto create(
      Perimortem::Core::View::Vector<Argument> arguments,
      Perimortem::Memory::Allocator::Arena& arena) const
      -> Perimortem::Utility::Option<const Ttx::Model::Type&> override;

  static constexpr Perimortem::Core::Static::Vector<Parameters, 2>
      parameterization = {{Parameters::Type, Parameters::Signed_64}};
  static constexpr Documentations::Comment documentation{
    "Creates a fixed homogeneous range Type."_view,
  };
};

}  // namespace Ttx::Model::Types::Generics
