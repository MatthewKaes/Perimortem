// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/option.hpp"

#include "perimortem/memory/dynamic/map.hpp"
#include "perimortem/memory/dynamic/vector.hpp"

#include "llvm-c/Types.h"
#include "tetrodotoxin/terminal/abi/representation/type.hpp"
#include "tetrodotoxin/terminal/llvm/module/emission.hpp"
#include "ttx/concept/layout.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/model/addressable.hpp"
#include "ttx/model/pack.hpp"
#include "ttx/model/type.hpp"

namespace Tetrodotoxin::Terminal::Llvm::Module {

// Carriers retains the LLVM representation selected by each exact source Type.
// Library owners reserve recursive identities and complete only their own
// physical facts through the LLVM execution owners.
class Carriers {
 public:
  // Kind describes the completed physical carrier selected by the source Type
  // owner. Consumers can inspect this target fact without rediscovering the
  // concrete declaration that contributed it.
  using Kind = Tetrodotoxin::Terminal::Abi::Representation::Type::Kind;

  auto reserve(Emission& program, const Ttx::Model::Type& type, Kind kind) const
      -> Perimortem::Core::Option<Bool>;

  auto begin_completion(Emission& program, const Ttx::Model::Type& type) const
      -> Perimortem::Core::Option<Bool>;

  auto complete(Emission& program, const Ttx::Model::Type& type, Kind kind)
      const -> Bool;

  auto get_type(const Ttx::Model::Type& type) const
      -> Perimortem::Core::Option<LLVMTypeRef>;

  auto get_payload(const Ttx::Model::Type& type) const
      -> Perimortem::Core::Option<LLVMTypeRef>;

  auto get_kind(const Ttx::Model::Type& type) const
      -> Perimortem::Core::Option<Kind>;

  auto get_width(const Ttx::Model::Type& type) const
      -> Perimortem::Core::Option<Count>;

  auto get_element(const Ttx::Model::Type& type) const
      -> Perimortem::Core::Option<const Ttx::Model::Type&>;

  auto get_flag(const Ttx::Model::Type& type) const
      -> Perimortem::Core::Option<const Ttx::Model::Type&>;

  auto get_error(const Ttx::Model::Type& type) const
      -> Perimortem::Core::Option<const Ttx::Model::Type&>;

  auto get_extent(const Ttx::Model::Type& type) const
      -> Perimortem::Core::Option<Count>;

  auto get_fields(const Ttx::Model::Type& type) const
      -> Perimortem::Core::Option<const Ttx::Concept::Layout&>;

  auto get_field_index(const Ttx::Model::Addressable& field) const
      -> Perimortem::Core::Option<Count>;

  auto get_field_host(const Ttx::Model::Addressable& field) const
      -> Perimortem::Core::Option<const Ttx::Model::Type&>;

  auto is_real(const Ttx::Model::Type& type) const -> Bool;

  auto is_signed(const Ttx::Model::Type& type) const -> Bool;

  auto is_flag(const Ttx::Model::Type& type) const -> Bool;

  auto is_object(const Ttx::Model::Type& type) const -> Bool;

  auto zero(Emission& program, const Ttx::Model::Type& type) const
      -> Perimortem::Core::Option<LLVMValueRef>;

  auto owns_resources(const Ttx::Model::Type& type) const -> Bool;

  auto retain(Emission& body, const Ttx::Model::Type& type, LLVMValueRef value)
      const -> Bool;

  auto release(Emission& body, const Ttx::Model::Type& type, LLVMValueRef value)
      const -> Bool;

  auto select_result(
      Emission& body,
      const Ttx::Model::Type& type,
      LLVMValueRef value,
      Bool value_selected) const -> Perimortem::Core::Option<LLVMValueRef>;

  auto assemble(
      Emission& body,
      const Ttx::Model::Type& type,
      Perimortem::Core::View::Vector<LLVMValueRef> elements) const
      -> Perimortem::Core::Option<LLVMValueRef>;

  auto fit_and_assemble(
      Emission& body,
      const Ttx::Model::Type& type,
      const Ttx::Model::Pack& source,
      Perimortem::Core::View::Vector<LLVMValueRef> elements) const
      -> Perimortem::Core::Option<LLVMValueRef>;

  auto fit(
      Emission& body,
      const Ttx::Model::Pack& source,
      const Ttx::Concept::Layout& target,
      Perimortem::Core::View::Vector<LLVMValueRef> values) const
      -> Perimortem::Core::Option<
          Perimortem::Memory::Dynamic::Vector<LLVMValueRef>>;

  auto construct(
      Emission& body,
      const Ttx::Model::Type& type,
      Perimortem::Core::View::Vector<LLVMValueRef> values) const
      -> Perimortem::Core::Option<LLVMValueRef>;

  auto get_object_descriptor(Emission& program, const Ttx::Model::Type& type)
      const -> Perimortem::Core::Option<LLVMValueRef>;

 private:
  enum class Phase : U8 {
    Reserved,
    Completing,
    Complete,
  };

  struct Carrier {
    enum class Property : U8 {
      Real = 1,
      Signed = 2,
      Flag = 4,
    };

    constexpr auto has(Property property) const -> Bool {
      return Bool(properties & U8(property));
    }

    constexpr auto set(Property property, Bool value) -> void {
      if (value) {
        properties |= U8(property);
      }
    }

    Kind kind;
    Phase phase = Phase::Reserved;
    Perimortem::Core::Option<LLVMTypeRef> native;
    Perimortem::Core::Option<LLVMTypeRef> payload;
    Perimortem::Core::Option<LLVMValueRef> finalizer;
    Perimortem::Core::Option<LLVMValueRef> descriptor;
    Perimortem::Core::Option<const Ttx::Model::Type&> element;
    Perimortem::Core::Option<const Ttx::Model::Type&> error;
    Perimortem::Core::Option<const Ttx::Model::Type&> flag;
    Perimortem::Core::Option<const Ttx::Concept::Layout&> fields;
    Count extent = 0;
    U8 properties = 0;
    Count width = 0;
  };

  auto publish(Emission& program, const Ttx::Model::Type& type, Carrier carrier)
      const -> Perimortem::Core::Option<Bool>;

  auto select_completion(
      Emission& program,
      const Ttx::Model::Type& type,
      Kind kind) const -> Perimortem::Core::Option<Carrier&>;

  auto complete_contiguous(
      Emission& program,
      const Ttx::Model::Type& type,
      const Ttx::Model::Type& element,
      Kind kind) const -> Bool;

  auto complete_aggregate(
      Emission& program,
      const Ttx::Model::Type& type,
      const Ttx::Concept::Layout& fields,
      Kind kind) const -> Bool;

  auto fit_values(
      Emission& body,
      const Ttx::Model::Pack& source,
      const Ttx::Concept::Layout& target,
      Perimortem::Core::View::Vector<LLVMValueRef> values) const
      -> Perimortem::Core::Option<
          Perimortem::Memory::Dynamic::Vector<LLVMValueRef>>;

  auto assemble_result(
      Emission& body,
      const Ttx::Model::Type& type,
      const Ttx::Model::Type& alternative,
      Bool value_selected,
      Perimortem::Core::View::Vector<LLVMValueRef> elements) const
      -> Perimortem::Core::Option<LLVMValueRef>;

  auto owns_resources(
      const Ttx::Model::Type& type,
      Perimortem::Memory::Dynamic::Vector<
          Ttx::Concept::Reference<const Ttx::Model::Type>>& active) const
      -> Bool;

  mutable Perimortem::Memory::Dynamic::Map<const Ttx::Model::Type*, Carrier>
      carriers;
  mutable Perimortem::Memory::Dynamic::
      Map<const Ttx::Model::Addressable*, Count>
          field_indices;
  mutable Perimortem::Memory::Dynamic::
      Map<const Ttx::Model::Addressable*, const Ttx::Model::Type*>
          field_hosts;
};

}  // namespace Tetrodotoxin::Terminal::Llvm::Module
