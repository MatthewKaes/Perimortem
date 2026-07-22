// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "ttx/concept/reference.hpp"
#include "ttx/model/addressables/writable.hpp"
#include "ttx/model/type.hpp"

namespace Tetrodotoxin::Model::Addressables {

// Field is the real named storage edge retained by a Library aggregate Type.
// Its read-only projection is a stable nested object with the same identity
// facts but without Writable capability. Neither object is a copied member
// record or a target offset.
class Field : public Ttx::Model::Addressables::Writable {
 public:
  using ContractOwner = Field;
  static constexpr Perimortem::System::Uuid contract_id{
    0x541fced112a7403a,
    0x8e00f21e37c2d788,
  };

  class ReadOnly final : public Ttx::Model::Addressable {
   public:
    constexpr ReadOnly(const Field& field) : field(field) {}

    constexpr auto get_name() const -> Perimortem::Core::View::Bytes override {
      return field.get_name();
    }
    constexpr auto get_documentation() const
        -> const Ttx::Concept::Documentation& override {
      return field.get_documentation();
    }
    constexpr auto get_type() const -> const Ttx::Concept::Abstract& override {
      return field.get_type();
    }

   private:
    const Field& field;
  };

  constexpr Field(
      Perimortem::Core::View::Bytes name,
      const Ttx::Model::Type& type,
      const Ttx::Concept::Documentation& documentation =
          Ttx::Concept::Documentation::get_empty())
      : name(name),
        type(type),
        documentation(documentation),
        read_only(*this) {}

  constexpr auto implements(Perimortem::System::Uuid requested) const
      -> Bool override {
    return requested == contract_id ||
           Ttx::Model::Addressables::Writable::implements(requested);
  }

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
  constexpr auto get_read_only() const
      -> const Ttx::Model::Addressable& override {
    return read_only;
  }

 private:
  Perimortem::Core::View::Bytes name;
  Ttx::Concept::Reference<Ttx::Model::Type> type;
  const Ttx::Concept::Documentation& documentation;
  ReadOnly read_only;
};

}  // namespace Tetrodotoxin::Model::Addressables
