// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/model/addressables/field.hpp"
#include "tetrodotoxin/model/types/members.hpp"
#include "tetrodotoxin/model/types/represented.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/model/layouts/structured.hpp"
#include "ttx/model/type.hpp"

namespace Tetrodotoxin::Model::Types {

// Structure is a Library-owned inline aggregate Type. Construction reserves
// the final nonmoving Type identity, attaches real Field/function owners, and
// completes its semantic Layout before the Type is published by its Source.
class Structure : public Represented {
 public:
  using ContractOwner = Structure;
  static constexpr Perimortem::System::Uuid contract_id{
    0xe2fb00a38f7046dd,
    0xace5122ae0ed3bce,
  };

  Structure(
      Perimortem::Memory::Allocator::Arena& arena,
      Perimortem::Core::View::Bytes name,
      const Ttx::Concept::Documentation& documentation);

  constexpr auto implements(Perimortem::System::Uuid requested) const
      -> Bool override {
    return requested == contract_id ||
           (represented && Represented::implements(requested)) ||
           (!represented && Ttx::Model::Type::implements(requested));
  }
  constexpr auto get_name() const -> Perimortem::Core::View::Bytes override {
    return name;
  }
  constexpr auto get_documentation() const
      -> const Ttx::Concept::Documentation& override {
    return documentation;
  }
  constexpr auto get_layout() const -> const Ttx::Concept::Layout& override {
    return layout;
  }
  auto resolve() const -> const Ttx::Concept::Abstract& override;
  auto resolve_context(Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract& override;

  auto resolve_root(Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract&;
  auto resolve_static(Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract&;
  auto resolve_exported_static(Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract&;
  auto resolve_self(Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract&;
  auto resolve_exported_self(Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract&;

  auto add_field(
      const Addressables::Field& field,
      const Ttx::Concept::Abstract& outer_context =
          Ttx::Concept::Invalid::get_invalid()) -> Bool;
  auto add_exported_field(
      const Addressables::Field& field,
      const Ttx::Concept::Abstract& outer_context =
          Ttx::Concept::Invalid::get_invalid()) -> Bool;
  auto add_exposed_field(
      const Addressables::Field& field,
      const Ttx::Concept::Abstract& outer_context =
          Ttx::Concept::Invalid::get_invalid()) -> Bool;
  auto add_member(
      const Ttx::Concept::Abstract& member,
      const Ttx::Concept::Abstract& outer_context =
          Ttx::Concept::Invalid::get_invalid()) -> Bool;
  auto add_exported_member(
      const Ttx::Concept::Abstract& member,
      const Ttx::Concept::Abstract& outer_context =
          Ttx::Concept::Invalid::get_invalid()) -> Bool;
  auto add_static(
      const Ttx::Model::Callables::Static& callable,
      const Ttx::Concept::Abstract& outer_context =
          Ttx::Concept::Invalid::get_invalid()) -> Bool;
  auto add_exported_static(
      const Ttx::Model::Callables::Static& callable,
      const Ttx::Concept::Abstract& outer_context =
          Ttx::Concept::Invalid::get_invalid()) -> Bool;
  auto add_self(
      const Ttx::Model::Callables::Self& callable,
      const Ttx::Concept::Abstract& outer_context =
          Ttx::Concept::Invalid::get_invalid()) -> Bool;
  auto add_exported_self(
      const Ttx::Model::Callables::Self& callable,
      const Ttx::Concept::Abstract& outer_context =          Ttx::Concept::Invalid::get_invalid()) -> Bool;
  auto complete() -> Bool;
  auto set_shader_type(const Ttx::Model::Type& type) -> Bool;

  constexpr auto get_shader_type() const -> const Ttx::Model::Type& override {
    return shader_type.get();
  }

  constexpr auto get_member_count() const -> Count {
    return members.get_root_count();
  }
  auto get_member(Count index) const -> const Ttx::Concept::Abstract&;
  constexpr auto get_export_count() const -> Count {
    return members.get_export_count();
  }
  auto get_export(Count index) const -> const Ttx::Concept::Abstract&;

 private:
  Perimortem::Core::View::Bytes name;
  const Ttx::Concept::Documentation& documentation;
  Members members;
  Perimortem::Memory::Managed::Vector<
      Ttx::Concept::Reference<Ttx::Model::Addressable>>
      fields;
  Ttx::Model::Layouts::Structured layout;
  Bool completed = False;
  Bool represented = False;
  Ttx::Concept::Reference<Ttx::Model::Type> shader_type;
};

}  // namespace Tetrodotoxin::Model::Types
