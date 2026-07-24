// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "ttx/concept/reference.hpp"
#include "ttx/model/addressables/writable.hpp"
#include "ttx/model/expression.hpp"

namespace Tetrodotoxin::Model::Renderables {

// Value is ordinary mutable per-render state with a complete initializer.
class Value : public Ttx::Model::Addressables::Writable {
 public:
  using ContractOwner = Value;
  static constexpr Perimortem::System::Uuid contract_id{
    0x196f60039fe74fa2,
    0x86f9576cd1cadc3f,
  };

  constexpr auto implements(Perimortem::System::Uuid requested) const
      -> Bool override {
    return requested == contract_id ||
           Ttx::Model::Addressables::Writable::implements(requested);
  }

  class ReadOnly : public Ttx::Model::Addressable {
   public:
    constexpr ReadOnly(const Value& value) : value(value) {}
    constexpr auto get_name() const -> Perimortem::Core::View::Bytes override {
      return value.get_name();
    }
    constexpr auto get_documentation() const
        -> const Ttx::Concept::Documentation& override {
      return value.get_documentation();
    }
    constexpr auto get_type() const -> const Ttx::Concept::Abstract& override {
      return value.get_type();
    }

   private:
    const Value& value;
  };

  constexpr Value(
      Perimortem::Core::View::Bytes name,
      const Ttx::Model::Type& type,
      const Ttx::Model::Expression& initializer,
      const Ttx::Concept::Documentation& documentation)
      : name(name),
        type(type),
        initializer(initializer),
        documentation(documentation),
        read_only(*this) {}

  constexpr auto get_name() const -> Perimortem::Core::View::Bytes override {
    return name;
  }
  constexpr auto get_documentation() const
      -> const Ttx::Concept::Documentation& override {
    return documentation;
  }
  constexpr auto get_type() const -> const Ttx::Concept::Abstract& override {
    return type.get();
  }
  constexpr auto get_initializer() const -> const Ttx::Model::Expression& {
    return initializer.get();
  }
  constexpr auto get_read_only() const
      -> const Ttx::Model::Addressable& override {
    return read_only;
  }

 private:
  Perimortem::Core::View::Bytes name;
  Ttx::Concept::Reference<Ttx::Model::Type> type;
  Ttx::Concept::Reference<Ttx::Model::Expression> initializer;
  const Ttx::Concept::Documentation& documentation;
  ReadOnly read_only;
};

}  // namespace Tetrodotoxin::Model::Renderables
