// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/static/vector.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "ttx/concept/invalid.hpp"
#include "ttx/model/documentations/comment.hpp"
#include "ttx/model/types/generics.hpp"

namespace Ttx::Model::Types::Generics {

// Access is the writable contiguous-storage formula. Its materialized Types
// retain the element identity and prove the nested Type contract. Access
// establishes write capability but does not claim noalias.
class Access final : public Generic {
 public:
  class Type final : public Ttx::Model::Type {
   public:
    using ContractOwner = Type;
    static constexpr Perimortem::System::Uuid contract_id{
      0x9297f2706d2e464e,
      0x82d4b4fece93716d,
    };

    constexpr Type(
        Perimortem::Core::View::Bytes name,
        const Ttx::Model::Type& element)
        : name(name), argument(element) {}

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

    constexpr auto get_arguments() const
        -> Perimortem::Core::View::Vector<Generic::Argument> {
      return {&argument, 1};
    }

    constexpr auto get_element_type() const -> const Ttx::Model::Type& {
      return *argument.find<const Ttx::Model::Type&>();
    }

   private:
    Perimortem::Core::View::Bytes name;
    Generic::Argument argument;
  };

  Access(Perimortem::Memory::Allocator::Arena& arena) : cache(arena) {}

  constexpr auto get_name() const -> Perimortem::Core::View::Bytes override {
    return "Access"_view;
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

  auto find(Perimortem::Core::View::Vector<Argument> arguments) const
      -> Perimortem::Utility::Option<Ttx::Model::Type&> override;

 private:
  // The owning semantic environment supplies the arena and bounds this cache
  // to the interpretation transaction shared by its Sources.
  mutable Perimortem::Memory::Managed::Vector<Type*> cache;
  inline static constexpr Perimortem::Core::Static::Vector<Parameters, 1>
      parameterization = {{Parameters::Type}};
  inline static constexpr Documentations::Comment documentation{
    "Provides writable access to contiguous values."_view,
  };
};

}  // namespace Ttx::Model::Types::Generics
