// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/option.hpp"

#include "perimortem/memory/dynamic/map.hpp"
#include "perimortem/memory/dynamic/vector.hpp"

#include "llvm-c/Types.h"
#include "ttx/concept/layout.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/model/addressable.hpp"
#include "ttx/model/pack.hpp"
#include "ttx/model/type.hpp"

namespace Tetrodotoxin::Library::Llvm {

// Carriers retains the LLVM representation selected by each exact source Type.
// Library owners reserve recursive identities and complete only their own
// physical facts through the direct Builder.
class Carriers {
 public:
  // Kind describes the completed physical carrier selected by the source Type
  // owner. Consumers can inspect this target fact without rediscovering the
  // concrete declaration that contributed it.
  enum class Kind : Unsigned_8 {
    Value,
    Enumeration,
    Fixed,
    Option,
    Range,
    View,
    Access,
    Structure,
    Object,
    Context,
  };

  auto reserve(
      Ttx::Concept::Abstract& program,
      const Ttx::Model::Type& type,
      Kind kind) const -> Perimortem::Core::Option<Bool>;

  auto begin_completion(
      Ttx::Concept::Abstract& program,
      const Ttx::Model::Type& type) const -> Perimortem::Core::Option<Bool>;

  auto complete(
      Ttx::Concept::Abstract& program,
      const Ttx::Model::Type& type,
      Kind kind) const -> Bool;

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

  auto zero(Ttx::Concept::Abstract& program, const Ttx::Model::Type& type) const
      -> Perimortem::Core::Option<LLVMValueRef>;

  auto owns_resources(const Ttx::Model::Type& type) const -> Bool;

  auto retain(
      Ttx::Concept::Abstract& body,
      const Ttx::Model::Type& type,
      LLVMValueRef value) const -> Bool;

  auto release(
      Ttx::Concept::Abstract& body,
      const Ttx::Model::Type& type,
      LLVMValueRef value) const -> Bool;

  auto assemble(
      Ttx::Concept::Abstract& body,
      const Ttx::Model::Type& type,
      Perimortem::Core::View::Vector<LLVMValueRef> elements) const
      -> Perimortem::Core::Option<LLVMValueRef>;

  auto fit_and_assemble(
      Ttx::Concept::Abstract& body,
      const Ttx::Model::Type& type,
      const Ttx::Model::Pack& source,
      Perimortem::Core::View::Vector<LLVMValueRef> elements) const
      -> Perimortem::Core::Option<LLVMValueRef>;

  auto fit(
      Ttx::Concept::Abstract& body,
      const Ttx::Model::Pack& source,
      const Ttx::Concept::Layout& target,
      Perimortem::Core::View::Vector<LLVMValueRef> values) const
      -> Perimortem::Core::Option<
          Perimortem::Memory::Dynamic::Vector<LLVMValueRef>>;

  auto construct(
      Ttx::Concept::Abstract& body,
      const Ttx::Model::Type& type,
      Perimortem::Core::View::Vector<LLVMValueRef> values) const
      -> Perimortem::Core::Option<LLVMValueRef>;

  auto get_object_finalizer(
      Ttx::Concept::Abstract& program,
      const Ttx::Model::Type& type) const
      -> Perimortem::Core::Option<LLVMValueRef>;

 private:
  enum class Phase : Unsigned_8 {
    Reserved,
    Completing,
    Complete,
  };

  struct Carrier {
    Kind kind;
    Phase phase = Phase::Reserved;
    Perimortem::Core::Option<LLVMTypeRef> native;
    Perimortem::Core::Option<LLVMTypeRef> payload;
    Perimortem::Core::Option<LLVMValueRef> finalizer;
    Perimortem::Core::Option<const Ttx::Model::Type&> element;
    Perimortem::Core::Option<const Ttx::Model::Type&> flag;
    Perimortem::Core::Option<const Ttx::Concept::Layout&> fields;
    Count extent = 0;
    Bool real = False;
    Bool signed_value = False;
    Bool flag_value = False;
    Count width = 0;
  };

  auto publish(
      Ttx::Concept::Abstract& program,
      const Ttx::Model::Type& type,
      Carrier carrier) const -> Perimortem::Core::Option<Bool>;

  auto select_completion(
      Ttx::Concept::Abstract& program,
      const Ttx::Model::Type& type,
      Kind kind) const -> Perimortem::Core::Option<Carrier&>;

  auto complete_contiguous(
      Ttx::Concept::Abstract& program,
      const Ttx::Model::Type& type,
      const Ttx::Model::Type& element,
      Kind kind) const -> Bool;

  auto complete_aggregate(
      Ttx::Concept::Abstract& program,
      const Ttx::Model::Type& type,
      const Ttx::Concept::Layout& fields,
      Kind kind) const -> Bool;

  auto fit_values(
      Ttx::Concept::Abstract& body,
      const Ttx::Model::Pack& source,
      const Ttx::Concept::Layout& target,
      Perimortem::Core::View::Vector<LLVMValueRef> values) const
      -> Perimortem::Core::Option<
          Perimortem::Memory::Dynamic::Vector<LLVMValueRef>>;

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

}  // namespace Tetrodotoxin::Library::Llvm
