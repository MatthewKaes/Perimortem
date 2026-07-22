// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "ttx/concept/reference.hpp"
#include "ttx/model/addressable.hpp"

namespace Tetrodotoxin::Model::Renderables {

// Resource assigns a descriptor set and binding to one Render Addressable.
class Resource final : public Ttx::Model::Addressable {
 public:
  using ContractOwner = Resource;
  static constexpr Perimortem::System::Uuid contract_id{
    0xd8dc846b73f74ef4,
    0x96ba7dd1bc3100b0,
  };

  constexpr Resource(
      Perimortem::Core::View::Bytes name,
      const Ttx::Model::Addressable& source,
      Unsigned_32 descriptor_set,
      Unsigned_32 binding,
      const Ttx::Concept::Documentation& documentation)
      : name(name),
        source(source),
        descriptor_set(descriptor_set),
        binding(binding),
        documentation(documentation) {}
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
  constexpr auto get_descriptor_set() const -> Unsigned_32 {
    return descriptor_set;
  }
  constexpr auto get_binding() const -> Unsigned_32 { return binding; }

 private:
  Perimortem::Core::View::Bytes name;
  Ttx::Concept::Reference<Ttx::Model::Addressable> source;
  Unsigned_32 descriptor_set;
  Unsigned_32 binding;
  const Ttx::Concept::Documentation& documentation;
};

}  // namespace Tetrodotoxin::Model::Renderables
