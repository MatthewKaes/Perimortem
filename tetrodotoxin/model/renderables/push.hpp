// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "ttx/concept/reference.hpp"
#include "ttx/model/addressable.hpp"

namespace Tetrodotoxin::Model::Renderables {

// Push assigns the Render push constant role to one real Addressable.
class Push : public Ttx::Model::Addressable {
 public:
  using ContractOwner = Push;
  static constexpr Perimortem::System::Uuid contract_id{
    0xc8965f4102c74429,
    0xbaf45f94d8bed089,
  };

  constexpr Push(
      Perimortem::Core::View::Bytes name,
      const Ttx::Model::Addressable& source,
      const Ttx::Concept::Documentation& documentation)
      : name(name), source(source), documentation(documentation) {}
  constexpr auto implements(Perimortem::System::Uuid requested) const
      -> Bool override {
    return requested == contract_id ||
           Ttx::Model::Addressable::implements(requested);
  }
  constexpr auto get_name() const -> Perimortem::Core::View::Bytes override {
    return name;
  }
  constexpr auto get_documentation() const
      -> const Ttx::Concept::Documentation& override {
    return documentation;
  }
  constexpr auto get_type() const -> const Ttx::Concept::Abstract& override {
    return source.get().get_type();
  }
  constexpr auto get_source() const -> const Ttx::Model::Addressable& {
    return source.get();
  }

 private:
  Perimortem::Core::View::Bytes name;
  Ttx::Concept::Reference<Ttx::Model::Addressable> source;
  const Ttx::Concept::Documentation& documentation;
};

}  // namespace Tetrodotoxin::Model::Renderables
