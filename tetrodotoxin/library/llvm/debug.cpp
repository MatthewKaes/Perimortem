// Perimortem Engine
// Copyright © Matt Kaes

// LLVM must enter before Perimortem so the standard placement declaration is
// visible before the freestanding fallback used by Perimortem headers.
#if __has_include("llvm/IR/IRBuilder.h")
#include "llvm/IR/IRBuilder.h"
#else
#error LLVM IRBuilder is required by the Library native compiler
#endif

#include "llvm-c/Core.h"
#include "llvm-c/DebugInfo.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/IR/DIBuilder.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/CBindingWrapping.h"
#include "llvm/Support/SHA256.h"
#include "tetrodotoxin/library/llvm/body.hpp"
#include "tetrodotoxin/library/llvm/debug.hpp"
#include "tetrodotoxin/library/llvm/program.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin::Library;

auto Llvm::Debug::release() -> void {
  if (released) {
    return;
  }

  builder.visit(
      []() {},
      [](LLVMOpaqueDIBuilder& selected) { LLVMDisposeDIBuilder(&selected); });
  released = True;
}

auto Llvm::Debug::find_type(const Ttx::Model::Type& type) const
    -> Core::Option<LLVMMetadataRef> {
  auto found = types.find(&type);
  return found ? Core::Option<LLVMMetadataRef>(found->value)
               : Core::Option<LLVMMetadataRef>();
}

auto Llvm::Debug::publish_type(
    const Ttx::Model::Type& type,
    LLVMMetadataRef metadata) -> Bool {
  if (types.contains(&type) || !metadata) {
    return False;
  }

  types.insert(&type, metadata);
  return True;
}

auto Llvm::Debug::replace_type(
    const Ttx::Model::Type& type,
    LLVMMetadataRef metadata) -> Bool {
  auto found = types.find(&type);
  if (!found || !metadata) {
    return False;
  }

  found->value = metadata;
  return True;
}

auto Llvm::Debug::find_payload(const Ttx::Model::Type& type) const
    -> Core::Option<LLVMMetadataRef> {
  auto found = payloads.find(&type);
  return found ? Core::Option<LLVMMetadataRef>(found->value)
               : Core::Option<LLVMMetadataRef>();
}

auto Llvm::Debug::publish_payload(
    const Ttx::Model::Type& type,
    LLVMMetadataRef metadata) -> Bool {
  if (payloads.contains(&type) || !metadata) {
    return False;
  }

  payloads.insert(&type, metadata);
  return True;
}

auto Llvm::Debug::replace_payload(
    const Ttx::Model::Type& type,
    LLVMMetadataRef metadata) -> Bool {
  auto found = payloads.find(&type);
  if (!found || !metadata) {
    return False;
  }

  found->value = metadata;
  return True;
}

auto Llvm::Debug::publish_enumerator(
    const Ttx::Model::Type& type,
    LLVMMetadataRef metadata) -> Bool {
  if (!metadata) {
    return False;
  }

  auto found = enumerators.find(&type);
  if (!found) {
    Memory::Dynamic::Vector<LLVMMetadataRef> entries;
    enumerators.insert(&type, entries);
    found = enumerators.find(&type);
  }

  if (!found) {
    return False;
  }

  found->value.insert(metadata);
  return True;
}

auto Llvm::Debug::get_enumerators(const Ttx::Model::Type& type) const
    -> Core::View::Vector<LLVMMetadataRef> {
  auto found = enumerators.find(&type);
  return found ? found->value.get_view()
               : Core::View::Vector<LLVMMetadataRef>();
}

auto Llvm::Debug::find_scope(const Ttx::Model::Type& type) const
    -> Core::Option<LLVMMetadataRef> {
  auto found = scopes.find(&type);
  return found ? Core::Option<LLVMMetadataRef>(found->value)
               : Core::Option<LLVMMetadataRef>();
}

auto Llvm::Debug::publish_scope(
    const Ttx::Model::Type& type,
    LLVMMetadataRef metadata) -> Bool {
  if (scopes.contains(&type) || !metadata) {
    return False;
  }

  scopes.insert(&type, metadata);
  scope_types.insert(type);
  return True;
}

auto Llvm::Debug::replace_scope(
    const Ttx::Model::Type& type,
    LLVMMetadataRef metadata) -> Bool {
  auto found = scopes.find(&type);
  if (!found || !metadata) {
    return False;
  }

  found->value = metadata;
  return True;
}

auto Llvm::Debug::get_scope_types() const
    -> Core::View::Vector<Ttx::Concept::Reference<const Ttx::Model::Type>> {
  return scope_types.get_view();
}

auto Llvm::Debug::publish_member(
    const Ttx::Model::Type& type,
    LLVMMetadataRef metadata) -> Bool {
  if (!metadata) {
    return False;
  }

  auto found = members.find(&type);
  if (!found) {
    Memory::Dynamic::Vector<LLVMMetadataRef> entries;
    members.insert(&type, entries);
    found = members.find(&type);
  }

  if (!found) {
    return False;
  }

  found->value.insert(metadata);
  return True;
}

auto Llvm::Debug::get_members(const Ttx::Model::Type& type) const
    -> Core::View::Vector<LLVMMetadataRef> {
  auto found = members.find(&type);
  return found ? found->value.get_view()
               : Core::View::Vector<LLVMMetadataRef>();
}

static auto select_program(Ttx::Concept::Abstract& program)
    -> Core::Option<Llvm::Program&> {
  return program.select<Llvm::Program>();
}

static auto select_body(Llvm::Body& body) -> Core::Option<Llvm::Body&> {
  return body.select<Llvm::Body>();
}

static auto native_builder(Llvm::Program& program)
    -> Core::Option<llvm::DIBuilder&> {
  auto selected = program.get_debug().get_builder();
  return selected ? Core::Option<llvm::DIBuilder&>(*llvm::unwrap(&*selected))
                  : Core::Option<llvm::DIBuilder&>();
}

static auto native_file(Llvm::Program& program) -> Core::Option<llvm::DIFile&> {
  auto selected = program.get_debug().get_file();
  return selected ? Core::Option<llvm::DIFile&>(
                        *llvm::cast<llvm::DIFile>(llvm::unwrap(&*selected)))
                  : Core::Option<llvm::DIFile&>();
}

static auto native_scope(Llvm::Body& body) -> Core::Option<llvm::DIScope&> {
  auto selected = body.get_debug_scope();
  return selected ? Core::Option<llvm::DIScope&>(
                        *llvm::cast<llvm::DIScope>(llvm::unwrap(*selected)))
                  : Core::Option<llvm::DIScope&>();
}

static auto native_text(Core::View::Bytes value) -> llvm::StringRef {
  return llvm::StringRef(
      reinterpret_cast<const char*>(value.get_data()), value.get_size());
}

auto Llvm::Debug::initialize(
    Ttx::Concept::Abstract& program,
    Core::View::Bytes source_path,
    Core::View::Bytes source_text) -> Bool {
  auto selected = select_program(program);
  if (!selected) {
    return False;
  }

  if (level == Level::None) {
    return True;
  }

  llvm::Module& module = *llvm::unwrap(&selected->get_module());
  module.addModuleFlag(llvm::Module::Warning, "Dwarf Version", 5);
  LLVMDIBuilderRef builder_handle =
      LLVMCreateDIBuilder(&selected->get_module());
  llvm::DIBuilder& native_builder = *llvm::unwrap(builder_handle);
  builder = *builder_handle;

  llvm::StringRef path = native_text(source_path);
  llvm::StringRef source = native_text(source_text);
  llvm::SHA256 checksum;
  checksum.update(source);
  std::string digest = llvm::toHex(checksum.final(), true);
  llvm::DIFile& native_file = *native_builder.createFile(
      path, "",
      llvm::DIFile::ChecksumInfo<llvm::StringRef>(
          llvm::DIFile::ChecksumKind::CSK_SHA256, digest));
  file = *llvm::wrap(&native_file);

  llvm::DICompileUnit::DebugEmissionKind emission =
      level == Level::Line ? llvm::DICompileUnit::LineTablesOnly
                           : llvm::DICompileUnit::FullDebug;
  native_builder.createCompileUnit(
      llvm::dwarf::DW_LANG_C11, &native_file, "Tetrodotoxin Library LLVM",
      false, "", 0, "", emission);
  return True;
}

static auto source_token(Ttx::Lexical::Anchor anchor) -> Ttx::Lexical::Token {
  Ttx::Lexical::Token token = anchor.get_token();
  return token ? token : anchor.get_span().get_start();
}

static auto source_line(Ttx::Lexical::Anchor anchor) -> Count {
  return source_token(anchor).get_line();
}

static auto source_column(Ttx::Lexical::Anchor anchor) -> Count {
  return source_token(anchor).get_column();
}

static auto set_location(Llvm::Body& body, Ttx::Lexical::Anchor anchor)
    -> Bool {
  auto scope = native_scope(body);
  if (!scope) {
    return True;
  }

  auto& builder = *reinterpret_cast<llvm::IRBuilder<>*>(body.get_builder());
  builder.SetCurrentDebugLocation(
      llvm::DILocation::get(
          llvm::unwrap<llvm::Function>(body.get_function())->getContext(),
          Unsigned_32(source_line(anchor)), Unsigned_32(source_column(anchor)),
          &*scope));
  return True;
}

static auto select_type(const Ttx::Concept::Layout& layout, Count index)
    -> Core::Option<const Ttx::Model::Type&> {
  auto entry = layout.get_abstract(index);
  if (!entry) {
    return {};
  }

  auto type = entry->select<Ttx::Model::Type>();
  if (type) {
    return *type;
  }

  auto addressable = entry->select<Ttx::Model::Addressable>();
  return addressable
             ? Core::Option<const Ttx::Model::Type&>(addressable->get_type())
             : Core::Option<const Ttx::Model::Type&>();
}

static auto native_module(Llvm::Program& program) -> llvm::Module& {
  return *llvm::unwrap(&program.get_module());
}

static auto size_in_bits(Llvm::Program& program, llvm::Type& type) -> Count {
  return Count(native_module(program)
                   .getDataLayout()
                   .getTypeAllocSizeInBits(&type)
                   .getFixedValue());
}

static auto alignment_in_bits(Llvm::Program& program, llvm::Type& type)
    -> Count {
  return Count(native_module(program)
                   .getDataLayout()
                   .getABITypeAlign(&type)
                   .value()) *
         8;
}

static auto create_member(
    Llvm::Program& program,
    llvm::DIBuilder& builder,
    llvm::DIFile& file,
    llvm::DICompositeType& scope,
    llvm::StructType& native,
    Count index,
    Core::View::Bytes name,
    llvm::DIType& type,
    Count line = 0) -> llvm::DIDerivedType& {
  llvm::Type& native_member = *native.getElementType(Unsigned_32(index));
  const llvm::StructLayout& layout =
      *native_module(program).getDataLayout().getStructLayout(&native);
  return *builder.createMemberType(
      &scope, native_text(name), &file, Unsigned_32(line),
      size_in_bits(program, native_member),
      Unsigned_32(alignment_in_bits(program, native_member)),
      layout.getElementOffsetInBits(Unsigned_32(index)), llvm::DINode::FlagZero,
      &type);
}

static auto create_debug_type(
    Llvm::Program& program,
    const Ttx::Model::Type& type,
    Count line = 0) -> Core::Option<llvm::DIType&> {
  auto create = [&](auto& create_type, const Ttx::Model::Type& selected,
                    Count selected_line) -> Core::Option<llvm::DIType&> {
    auto existing = program.get_debug().find_type(selected);
    if (existing) {
      return *llvm::cast<llvm::DIType>(llvm::unwrap(*existing));
    }

    auto builder = native_builder(program);
    auto file = native_file(program);
    auto kind = program.get_carriers().get_kind(selected);
    auto physical = program.get_carriers().get_type(selected);
    if (!builder || !file || !kind || !physical) {
      return {};
    }

    llvm::Type& native = *llvm::unwrap(*physical);
    Count size = size_in_bits(program, native);
    Count alignment = alignment_in_bits(program, native);
    if (*kind == Llvm::Carriers::Kind::Value) {
      Unsigned_32 encoding = llvm::dwarf::DW_ATE_unsigned;
      if (program.get_carriers().is_flag(selected)) {
        encoding = llvm::dwarf::DW_ATE_boolean;
      } else if (program.get_carriers().is_real(selected)) {
        encoding = llvm::dwarf::DW_ATE_float;
      } else if (program.get_carriers().is_signed(selected)) {
        encoding = llvm::dwarf::DW_ATE_signed;
      }

      llvm::DIType& created = *builder->createBasicType(
          native_text(selected.get_name()), size, encoding);
      if (!program.get_debug().publish_type(selected, llvm::wrap(&created))) {
        return {};
      }

      return created;
    }

    if (*kind == Llvm::Carriers::Kind::Enumeration) {
      Unsigned_32 encoding = program.get_carriers().is_signed(selected)
                                 ? llvm::dwarf::DW_ATE_signed
                                 : llvm::dwarf::DW_ATE_unsigned;
      llvm::DIType& storage =
          *builder->createBasicType("storage", size, encoding);
      llvm::SmallVector<llvm::Metadata*, 16> elements;
      for (LLVMMetadataRef entry :
           program.get_debug().get_enumerators(selected)) {
        elements.push_back(llvm::unwrap(entry));
      }

      llvm::DIType& created = *builder->createEnumerationType(
          &*file, native_text(selected.get_name()), &*file,
          Unsigned_32(selected_line), size, Unsigned_32(alignment),
          builder->getOrCreateArray(elements), &storage);
      if (!program.get_debug().publish_type(selected, llvm::wrap(&created))) {
        return {};
      }

      return created;
    }

    if (*kind == Llvm::Carriers::Kind::Fixed) {
      auto element = program.get_carriers().get_element(selected);
      auto extent = program.get_carriers().get_extent(selected);
      if (!element || !extent) {
        return {};
      }

      auto debug_element = create_type(create_type, *element, 0);
      if (!debug_element) {
        return {};
      }

      llvm::Metadata* subrange =
          builder->getOrCreateSubrange(0, Signed_64(*extent));
      llvm::DIType& created = *builder->createArrayType(
          &*file, native_text(selected.get_name()), &*file,
          Unsigned_32(selected_line), size, Unsigned_32(alignment),
          &*debug_element, builder->getOrCreateArray({subrange}));
      if (!program.get_debug().publish_type(selected, llvm::wrap(&created))) {
        return {};
      }

      return created;
    }

    if (*kind == Llvm::Carriers::Kind::Object) {
      auto payload_handle = program.get_carriers().get_payload(selected);
      if (!payload_handle) {
        return {};
      }

      auto* payload_native =
          llvm::dyn_cast<llvm::StructType>(llvm::unwrap(*payload_handle));
      auto fields = program.get_carriers().get_fields(selected);
      if (!payload_native || !fields) {
        return {};
      }

      Count payload_size = size_in_bits(program, *payload_native);
      Count payload_alignment = alignment_in_bits(program, *payload_native);
      auto retained_scope = program.get_debug().find_scope(selected);
      llvm::DICompositeType* temporary =
          retained_scope ? llvm::dyn_cast<llvm::DICompositeType>(
                               llvm::unwrap(*retained_scope))
                         : builder->createReplaceableCompositeType(
                               llvm::dwarf::DW_TAG_structure_type,
                               native_text(selected.get_name()), &*file, &*file,
                               Unsigned_32(selected_line), 0, payload_size,
                               Unsigned_32(payload_alignment));
      if (!temporary) {
        return {};
      }

      if (!retained_scope &&
          !program.get_debug().publish_scope(selected, llvm::wrap(temporary))) {
        return {};
      }

      if (!program.get_debug().publish_payload(
              selected, llvm::wrap(temporary))) {
        return {};
      }

      llvm::DIType& pointer =
          *builder->createPointerType(temporary, size, Unsigned_32(alignment));
      if (!program.get_debug().publish_type(selected, llvm::wrap(&pointer))) {
        return {};
      }

      llvm::SmallVector<llvm::Metadata*, 16> members;
      for (Count index = 0; index < fields->get_size(); index++) {
        auto entry = fields->get_abstract(index);
        auto field = entry ? entry->select<Ttx::Model::Addressable>()
                           : Core::Option<const Ttx::Model::Addressable&>();
        if (!field) {
          return {};
        }

        auto debug_field = create_type(create_type, field->get_type(), 0);
        if (!debug_field) {
          return {};
        }

        members.push_back(&create_member(
            program, *builder, *file, *temporary, *payload_native, index,
            field->get_name(), *debug_field));
      }

      for (LLVMMetadataRef member : program.get_debug().get_members(selected)) {
        members.push_back(llvm::unwrap(member));
      }

      llvm::DICompositeType* completed = builder->createStructType(
          &*file, native_text(selected.get_name()), &*file,
          Unsigned_32(selected_line), payload_size,
          Unsigned_32(payload_alignment), llvm::DINode::FlagZero, nullptr,
          builder->getOrCreateArray(members));
      completed = builder->replaceTemporary(
          llvm::TempDICompositeType(temporary), completed);
      if (!program.get_debug().replace_payload(
              selected, llvm::wrap(completed))) {
        return {};
      }

      if (!program.get_debug().replace_scope(selected, llvm::wrap(completed))) {
        return {};
      }

      return pointer;
    }

    auto* native_struct = llvm::dyn_cast<llvm::StructType>(&native);
    if (!native_struct) {
      return {};
    }

    Bool structured = *kind == Llvm::Carriers::Kind::Structure;
    auto retained_scope = structured ? program.get_debug().find_scope(selected)
                                     : Core::Option<LLVMMetadataRef>();
    llvm::DICompositeType* temporary =
        retained_scope
            ? llvm::dyn_cast<llvm::DICompositeType>(
                  llvm::unwrap(*retained_scope))
            : builder->createReplaceableCompositeType(
                  llvm::dwarf::DW_TAG_structure_type,
                  native_text(selected.get_name()), &*file, &*file,
                  Unsigned_32(selected_line), 0, size, Unsigned_32(alignment));
    if (!temporary) {
      return {};
    }

    if (structured && !retained_scope &&
        !program.get_debug().publish_scope(selected, llvm::wrap(temporary))) {
      return {};
    }

    if (!program.get_debug().publish_type(selected, llvm::wrap(temporary))) {
      return {};
    }

    llvm::SmallVector<llvm::Metadata*, 16> members;
    if (*kind == Llvm::Carriers::Kind::Option) {
      auto element = program.get_carriers().get_element(selected);
      auto flag = program.get_carriers().get_flag(selected);
      if (!element || !flag) {
        return {};
      }

      auto debug_element = create_type(create_type, *element, 0);
      auto debug_flag = create_type(create_type, *flag, 0);
      if (!debug_element || !debug_flag) {
        return {};
      }

      members.push_back(&create_member(
          program, *builder, *file, *temporary, *native_struct, 0, "value"_view,
          *debug_element));
      members.push_back(&create_member(
          program, *builder, *file, *temporary, *native_struct, 1, "set"_view,
          *debug_flag));
    } else if (*kind == Llvm::Carriers::Kind::Result) {
      auto value = program.get_carriers().get_element(selected);
      auto error = program.get_carriers().get_error(selected);
      auto flag = program.get_carriers().get_flag(selected);
      auto storage = program.get_carriers().get_payload(selected);
      auto native_value = value ? program.get_carriers().get_type(*value)
                                : Core::Option<LLVMTypeRef>();
      auto native_error = error ? program.get_carriers().get_type(*error)
                                : Core::Option<LLVMTypeRef>();
      if (!value || !error || !flag || !storage || !native_value ||
          !native_error) {
        return {};
      }

      auto debug_value = create_type(create_type, *value, 0);
      auto debug_error = create_type(create_type, *error, 0);
      auto debug_flag = create_type(create_type, *flag, 0);
      if (!debug_value || !debug_error || !debug_flag) {
        return {};
      }

      llvm::SmallVector<llvm::Metadata*, 2> alternatives;
      alternatives.push_back(builder->createMemberType(
          temporary, "value", &*file, 0,
          size_in_bits(program, *llvm::unwrap(*native_value)),
          Unsigned_32(alignment_in_bits(program, *llvm::unwrap(*native_value))),
          0, llvm::DINode::FlagZero, &*debug_value));
      alternatives.push_back(builder->createMemberType(
          temporary, "error", &*file, 0,
          size_in_bits(program, *llvm::unwrap(*native_error)),
          Unsigned_32(alignment_in_bits(program, *llvm::unwrap(*native_error))),
          0, llvm::DINode::FlagZero, &*debug_error));
      llvm::DICompositeType* debug_storage = builder->createUnionType(
          temporary, "storage", &*file, 0,
          size_in_bits(program, *llvm::unwrap(*storage)),
          Unsigned_32(alignment_in_bits(program, *llvm::unwrap(*storage))),
          llvm::DINode::FlagZero, builder->getOrCreateArray(alternatives));
      members.push_back(&create_member(
          program, *builder, *file, *temporary, *native_struct, 0,
          "storage"_view, *debug_storage));
      members.push_back(&create_member(
          program, *builder, *file, *temporary, *native_struct, 1,
          "value_selected"_view, *debug_flag));
    } else if (
        *kind == Llvm::Carriers::Kind::View ||
        *kind == Llvm::Carriers::Kind::Access) {
      auto element = program.get_carriers().get_element(selected);
      if (!element) {
        return {};
      }

      auto debug_element = create_type(create_type, *element, 0);
      if (!debug_element) {
        return {};
      }

      llvm::DIType* pointed = &*debug_element;
      if (*kind == Llvm::Carriers::Kind::View) {
        pointed = builder->createQualifiedType(
            llvm::dwarf::DW_TAG_const_type, pointed);
      }

      llvm::Type& pointer_native = *native_struct->getElementType(0);
      llvm::DIType& pointer = *builder->createPointerType(
          pointed, size_in_bits(program, pointer_native),
          Unsigned_32(alignment_in_bits(program, pointer_native)));
      llvm::DIType& count =
          *builder->createBasicType("Count", 64, llvm::dwarf::DW_ATE_unsigned);
      members.push_back(&create_member(
          program, *builder, *file, *temporary, *native_struct, 0, "data"_view,
          pointer));
      members.push_back(&create_member(
          program, *builder, *file, *temporary, *native_struct, 1, "size"_view,
          count));
    } else if (*kind == Llvm::Carriers::Kind::Range) {
      auto element = program.get_carriers().get_element(selected);
      if (!element) {
        return {};
      }

      auto debug_element = create_type(create_type, *element, 0);
      if (!debug_element) {
        return {};
      }

      members.push_back(&create_member(
          program, *builder, *file, *temporary, *native_struct, 0, "start"_view,
          *debug_element));
      members.push_back(&create_member(
          program, *builder, *file, *temporary, *native_struct, 1, "end"_view,
          *debug_element));
    } else {
      auto fields = program.get_carriers().get_fields(selected);
      if (*kind != Llvm::Carriers::Kind::Context && !fields) {
        return {};
      }

      if (fields) {
        for (Count index = 0; index < fields->get_size(); index++) {
          auto entry = fields->get_abstract(index);
          auto field = entry ? entry->select<Ttx::Model::Addressable>()
                             : Core::Option<const Ttx::Model::Addressable&>();
          if (!field) {
            return {};
          }

          auto debug_field = create_type(create_type, field->get_type(), 0);
          if (!debug_field) {
            return {};
          }

          members.push_back(&create_member(
              program, *builder, *file, *temporary, *native_struct, index,
              field->get_name(), *debug_field));
        }
      }
    }

    if (structured) {
      for (LLVMMetadataRef member : program.get_debug().get_members(selected)) {
        members.push_back(llvm::unwrap(member));
      }
    }

    llvm::DICompositeType* completed = builder->createStructType(
        &*file, native_text(selected.get_name()), &*file,
        Unsigned_32(selected_line), size, Unsigned_32(alignment),
        llvm::DINode::FlagZero, nullptr, builder->getOrCreateArray(members));
    completed = builder->replaceTemporary(
        llvm::TempDICompositeType(temporary), completed);
    if (!program.get_debug().replace_type(selected, llvm::wrap(completed))) {
      return {};
    }

    if (structured &&
        !program.get_debug().replace_scope(selected, llvm::wrap(completed))) {
      return {};
    }

    return *completed;
  };

  return create(create, type, line);
}

static auto reserve_debug_scope(
    Llvm::Program& program,
    const Ttx::Model::Type& type,
    Count line,
    Bool complete) -> Core::Option<llvm::DIScope&> {
  auto builder = native_builder(program);
  auto file = native_file(program);
  if (!builder || !file) {
    program.fail_backend(
        "LLVM cannot create a Type debug scope before its debug file is "
        "available."_view);
    return {};
  }

  auto retained = program.get_debug().find_scope(type);
  Bool source = type.get_name() == "<source>"_view;
  if (source && !retained) {
    llvm::DICompositeType& created = *builder->createReplaceableCompositeType(
        llvm::dwarf::DW_TAG_structure_type, "source", &*file, &*file, 0);
    if (!program.get_debug().publish_scope(type, llvm::wrap(&created))) {
      program.fail_backend(
          "LLVM cannot publish the Source debug scope twice."_view);
      return {};
    }

    retained = program.get_debug().find_scope(type);
  } else if (complete) {
    auto kind = program.get_carriers().get_kind(type);
    if (!kind) {
      program.fail_backend(
          "LLVM cannot complete this authored Type debug scope."_view);
      return {};
    }

    if (*kind != Llvm::Carriers::Kind::Context) {
      auto created = create_debug_type(program, type, line);
      if (!created) {
        program.fail_backend(
            "LLVM cannot complete one authored Type debug scope."_view);
        return {};
      }

      retained = program.get_debug().find_scope(type);
    }
  }

  if (!retained) {
    llvm::DICompositeType& created = *builder->createReplaceableCompositeType(
        llvm::dwarf::DW_TAG_structure_type, native_text(type.get_name()),
        &*file, &*file, Unsigned_32(line));
    if (!program.get_debug().publish_scope(type, llvm::wrap(&created))) {
      program.fail_backend(
          "LLVM cannot publish one authored Type debug scope twice."_view);
      return {};
    }

    retained = program.get_debug().find_scope(type);
  }

  if (!retained) {
    program.fail_backend(
        "LLVM did not retain the authored Type debug scope it created."_view);
    return {};
  }

  auto* scope = llvm::dyn_cast<llvm::DIScope>(llvm::unwrap(*retained));
  if (!scope) {
    program.fail_backend(
        "LLVM retained one authored Type debug identity without a scope."_view);
    return {};
  }

  return *scope;
}

static auto debug_visibility(Tetrodotoxin::Language::Visibility visibility)
    -> llvm::DINode::DIFlags {
  switch (visibility) {
  case Tetrodotoxin::Language::Visibility::Private:
    return llvm::DINode::FlagPrivate;

  case Tetrodotoxin::Language::Visibility::Public:
  case Tetrodotoxin::Language::Visibility::Exposed:
    return llvm::DINode::FlagPublic;
  }

  return llvm::DINode::FlagZero;
}

static auto declare_local(
    Llvm::Body& body,
    const Ttx::Model::Addressable& addressable,
    Ttx::Lexical::Anchor anchor,
    Core::Option<Count> parameter) -> Bool {
  auto selected_body = select_body(body);
  auto selected_program = select_program(body.get_program());
  if (!selected_body || !selected_program) {
    return False;
  }

  if (selected_program->get_debug().get_level() != Llvm::Debug::Level::Full) {
    return True;
  }

  auto builder = native_builder(*selected_program);
  auto file = native_file(*selected_program);
  auto scope = native_scope(*selected_body);
  auto address = selected_body->find_address(addressable);
  if (!builder || !file || !scope || !address) {
    return False;
  }

  auto type = create_debug_type(*selected_program, addressable.get_type());
  if (!type) {
    return False;
  }

  Core::Option<llvm::DILocalVariable&> variable;
  if (parameter) {
    auto* created = builder->createParameterVariable(
        &*scope, native_text(addressable.get_name()),
        Unsigned_32(*parameter + 1), &*file, Unsigned_32(source_line(anchor)),
        &*type, true);
    if (created) {
      variable = *created;
    }
  } else {
    auto* created = builder->createAutoVariable(
        &*scope, native_text(addressable.get_name()), &*file,
        Unsigned_32(source_line(anchor)), &*type, true);
    if (created) {
      variable = *created;
    }
  }

  if (!variable) {
    return selected_program->fail_backend(
        "LLVM could not create one local debug declaration."_view);
  }

  llvm::Function& function =
      *llvm::unwrap<llvm::Function>(selected_body->get_function());
  llvm::DILocation* location = llvm::DILocation::get(
      function.getContext(), Unsigned_32(source_line(anchor)),
      Unsigned_32(source_column(anchor)), &*scope);
  builder->insertDeclare(
      llvm::unwrap(*address), &*variable, builder->createExpression(), location,
      &function.getEntryBlock());
  return True;
}

auto Llvm::Debug::type(
    const Ttx::Model::Type&,
    Core::Option<Ttx::Lexical::Anchor>) -> Bool {
  return True;
}

auto Llvm::Debug::field(const Ttx::Model::Addressable&, Ttx::Lexical::Anchor)
    -> Bool {
  return True;
}

auto Llvm::Debug::signed_enumerator(
    Ttx::Concept::Abstract& program,
    const Ttx::Model::Type& type,
    const Ttx::Concept::Abstract& enumerator,
    Signed_64 value) -> Bool {
  auto selected = select_program(program);
  if (!selected ||
      selected->get_debug().get_level() != Llvm::Debug::Level::Full) {
    return Bool(selected);
  }

  auto builder = native_builder(*selected);
  return builder && publish_enumerator(
                        type, llvm::wrap(builder->createEnumerator(
                                  native_text(enumerator.get_name()),
                                  Unsigned_64(value), false)));
}

auto Llvm::Debug::unsigned_enumerator(
    Ttx::Concept::Abstract& program,
    const Ttx::Model::Type& type,
    const Ttx::Concept::Abstract& enumerator,
    Unsigned_64 value) -> Bool {
  auto selected = select_program(program);
  if (!selected ||
      selected->get_debug().get_level() != Llvm::Debug::Level::Full) {
    return Bool(selected);
  }

  auto builder = native_builder(*selected);
  return builder &&
         publish_enumerator(
             type, llvm::wrap(builder->createEnumerator(
                       native_text(enumerator.get_name()), value, true)));
}

auto Llvm::Debug::global(
    Ttx::Concept::Abstract& program,
    const Ttx::Model::Addressable& addressable,
    const Tetrodotoxin::Language::Definition& definition,
    Bool local,
    Bool defined) -> Bool {
  auto selected_program = select_program(program);
  if (!selected_program) {
    return False;
  }

  if (selected_program->get_debug().get_level() != Llvm::Debug::Level::Full) {
    return True;
  }

  auto builder = native_builder(*selected_program);
  auto file = native_file(*selected_program);
  auto address = selected_program->get_globals().find_address(addressable);
  if (!builder || !file || !address) {
    return selected_program->fail_backend(
        "LLVM cannot describe a Static before its debug builder, file, and "
        "global storage are complete."_view);
  }

  auto* global = llvm::dyn_cast<llvm::GlobalVariable>(llvm::unwrap(*address));
  if (!global) {
    return selected_program->fail_backend(
        "LLVM debug information requires Static storage to be one global."_view);
  }

  Ttx::Lexical::Anchor anchor = definition.get_anchor();
  auto type = create_debug_type(*selected_program, addressable.get_type());
  if (!type) {
    return selected_program->fail_backend(
        "LLVM cannot describe the completed Static carrier."_view);
  }

  auto host = definition.get_host().select<Ttx::Model::Type>();
  auto scope = host ? reserve_debug_scope(
                          *selected_program, *host, source_line(anchor), False)
                    : Core::Option<llvm::DIScope&>(*file);
  if (!scope) {
    return selected_program->fail_backend(
        "LLVM cannot create the authored Type debug scope for one Static."_view);
  }

  Core::Option<llvm::DIDerivedType&> declaration;
  if (auto* composite = llvm::dyn_cast<llvm::DICompositeType>(&*scope)) {
    auto physical =
        selected_program->get_carriers().get_type(addressable.get_type());
    if (!physical) {
      return selected_program->fail_backend(
          "LLVM cannot find the completed Static carrier alignment."_view);
    }

    declaration = *builder->createStaticMemberType(
        composite, native_text(addressable.get_name()), &*file,
        Unsigned_32(source_line(anchor)), &*type,
        debug_visibility(definition.get_visibility()), nullptr,
        llvm::dwarf::DW_TAG_variable,
        Unsigned_32(
            alignment_in_bits(*selected_program, *llvm::unwrap(*physical))));
    if (!publish_member(*host, llvm::wrap(&*declaration))) {
      return selected_program->fail_backend(
          "LLVM cannot retain one Static member declaration twice."_view);
    }
  }

  llvm::DIGlobalVariableExpression& expression =
      *builder->createGlobalVariableExpression(
          declaration ? static_cast<llvm::DIScope*>(&*file) : &*scope,
          native_text(addressable.get_name()), global->getName(), &*file,
          Unsigned_32(source_line(anchor)), &*type, bool(local), bool(defined),
          nullptr, declaration ? &*declaration : nullptr);
  global->addDebugInfo(&expression);
  return True;
}

auto Llvm::Debug::begin_function(
    Ttx::Concept::Abstract& body,
    const Ttx::Model::Callable& callable,
    const Tetrodotoxin::Language::Definition& definition) -> Bool {
  auto selected_body = body.select<Llvm::Body>();
  auto selected_program = selected_body
                              ? select_program(selected_body->get_program())
                              : Core::Option<Llvm::Program&>();
  if (!selected_body || !selected_program) {
    return False;
  }

  auto builder = native_builder(*selected_program);
  auto file = native_file(*selected_program);
  if (!builder || !file) {
    return True;
  }

  llvm::Function& function =
      *llvm::unwrap<llvm::Function>(selected_body->get_function());
  Ttx::Lexical::Anchor anchor = definition.get_anchor();
  auto host = definition.get_host().select<Ttx::Model::Type>();
  auto owner_scope =
      host ? reserve_debug_scope(
                 *selected_program, *host, source_line(anchor), True)
           : Core::Option<llvm::DIScope&>(*file);
  if (!owner_scope) {
    return False;
  }

  llvm::SmallVector<llvm::Metadata*, 16> signature_types;
  const Ttx::Concept::Layout& results = callable.get_results();
  if (results.is_empty()) {
    signature_types.push_back(nullptr);
  } else if (results.get_size() == 1) {
    auto result = select_type(results, 0);
    auto debug_result = result ? create_debug_type(*selected_program, *result)
                               : Core::Option<llvm::DIType&>();
    if (!debug_result) {
      return False;
    }

    signature_types.push_back(&*debug_result);
  } else {
    auto native_result =
        selected_program->get_functions().find_sret_type(callable);
    llvm::Type* returned =
        native_result ? llvm::unwrap(*native_result) : function.getReturnType();
    auto* returned_struct = llvm::dyn_cast<llvm::StructType>(returned);
    if (!returned_struct ||
        returned_struct->getNumElements() != results.get_size()) {
      return False;
    }

    Count size = size_in_bits(*selected_program, *returned_struct);
    Count alignment = alignment_in_bits(*selected_program, *returned_struct);
    llvm::DICompositeType* temporary = builder->createReplaceableCompositeType(
        llvm::dwarf::DW_TAG_structure_type, "TTX results", &*file, &*file,
        Unsigned_32(source_line(anchor)), 0, size, Unsigned_32(alignment));
    llvm::SmallVector<llvm::Metadata*, 8> members;
    for (Count index = 0; index < results.get_size(); index++) {
      auto result = select_type(results, index);
      auto debug_result = result ? create_debug_type(*selected_program, *result)
                                 : Core::Option<llvm::DIType&>();
      auto entry = results.get_abstract(index);
      if (!debug_result || !entry) {
        return False;
      }

      auto name = results.get_name(index);

      members.push_back(&create_member(
          *selected_program, *builder, *file, *temporary, *returned_struct,
          index, name ? *name : entry->get_name(), *debug_result,
          source_line(anchor)));
    }

    llvm::DICompositeType* completed = builder->createStructType(
        &*file, "TTX results", &*file, Unsigned_32(source_line(anchor)), size,
        Unsigned_32(alignment), llvm::DINode::FlagZero, nullptr,
        builder->getOrCreateArray(members));
    completed = builder->replaceTemporary(
        llvm::TempDICompositeType(temporary), completed);
    signature_types.push_back(completed);
  }

  const Ttx::Concept::Layout& parameters = callable.get_parameters();
  for (Count index = 0; index < parameters.get_size(); index++) {
    auto parameter = select_type(parameters, index);
    auto debug_parameter =
        parameter ? create_debug_type(*selected_program, *parameter)
                  : Core::Option<llvm::DIType&>();
    if (!debug_parameter) {
      return False;
    }

    signature_types.push_back(&*debug_parameter);
  }

  llvm::DISubroutineType& signature = *builder->createSubroutineType(
      builder->getOrCreateTypeArray(signature_types));
  llvm::DISubprogram& scope = *builder->createFunction(
      &*owner_scope, native_text(callable.get_name()), function.getName(),
      &*file, Unsigned_32(source_line(anchor)), &signature,
      Unsigned_32(source_line(anchor)), llvm::DINode::FlagPrototyped,
      llvm::DISubprogram::SPFlagDefinition);
  function.setSubprogram(&scope);
  selected_body->set_debug_scope(llvm::wrap(&scope));
  return set_location(*selected_body, anchor);
}

auto Llvm::Debug::parameter(
    Ttx::Concept::Abstract& body,
    const Ttx::Model::Addressable& parameter,
    Ttx::Lexical::Anchor anchor,
    Count index) -> Bool {
  auto selected = body.select<Llvm::Body>();
  return selected && declare_local(*selected, parameter, anchor, index);
}

auto Llvm::Debug::end_function(Ttx::Concept::Abstract& body) -> Bool {
  auto selected = body.select<Llvm::Body>();
  if (!selected) {
    return False;
  }

  reinterpret_cast<llvm::IRBuilder<>*>(selected->get_builder())
      ->SetCurrentDebugLocation(llvm::DebugLoc());
  selected->set_debug_scope({});
  return True;
}

auto Llvm::Debug::begin_block(
    Ttx::Concept::Abstract& body,
    const Ttx::Concept::Abstract& block,
    Ttx::Lexical::Anchor anchor) -> Bool {
  auto selected_body = body.select<Llvm::Body>();
  auto selected_program = selected_body
                              ? select_program(selected_body->get_program())
                              : Core::Option<Llvm::Program&>();
  if (!selected_body || !selected_program) {
    return False;
  }

  if (!selected_body->push_block_scope(block)) {
    return False;
  }

  auto builder = native_builder(*selected_program);
  auto file = native_file(*selected_program);
  auto scope = native_scope(*selected_body);
  if (!builder || !file || !scope) {
    return True;
  }

  llvm::DILexicalBlock& debug_scope = *builder->createLexicalBlock(
      &*scope, &*file, Unsigned_32(source_line(anchor)),
      Unsigned_32(source_column(anchor)));
  if (!selected_body->push_debug_scope(llvm::wrap(&debug_scope))) {
    return False;
  }

  return set_location(*selected_body, anchor);
}

auto Llvm::Debug::statement(
    Ttx::Concept::Abstract& body,
    Ttx::Lexical::Anchor anchor) -> Bool {
  auto selected = body.select<Llvm::Body>();
  return selected && set_location(*selected, anchor);
}

auto Llvm::Debug::local(
    Ttx::Concept::Abstract& body,
    const Ttx::Model::Addressable& local,
    Ttx::Lexical::Anchor anchor) -> Bool {
  auto selected = body.select<Llvm::Body>();
  return selected && declare_local(*selected, local, anchor, {});
}

auto Llvm::Debug::finalize(Ttx::Concept::Abstract& program) -> Bool {
  auto selected = select_program(program);
  if (!selected) {
    return False;
  }

  if (level != Level::Full) {
    builder.visit(
        []() {},
        [](LLVMOpaqueDIBuilder& native) { llvm::unwrap(&native)->finalize(); });
    return True;
  }

  for (const Ttx::Concept::Reference<const Ttx::Model::Type>& retained :
       get_scope_types()) {
    const Ttx::Model::Type& type = retained.get();
    auto kind = selected->get_carriers().get_kind(type);
    if (!kind) {
      return selected->fail_backend(
          "LLVM lost a Type selected by one debug scope."_view);
    }

    if (*kind == Llvm::Carriers::Kind::Context) {
      auto builder = native_builder(*selected);
      auto file = native_file(*selected);
      auto scope = selected->get_debug().find_scope(type);
      if (!builder || !file || !scope) {
        return selected->fail_backend(
            "LLVM cannot complete one contextual Type debug scope."_view);
      }

      auto* temporary =
          llvm::dyn_cast<llvm::DICompositeType>(llvm::unwrap(*scope));
      if (!temporary) {
        return selected->fail_backend(
            "LLVM cannot complete one contextual Type debug scope."_view);
      }

      llvm::SmallVector<llvm::Metadata*, 16> members;
      for (LLVMMetadataRef member : get_members(type)) {
        members.push_back(llvm::unwrap(member));
      }

      Count size = 0;
      Unsigned_32 alignment = 0;
      llvm::DICompositeType* completed = builder->createStructType(
          &*file, temporary->getName(), &*file, temporary->getLine(), size,
          alignment, llvm::DINode::FlagZero, nullptr,
          builder->getOrCreateArray(members));
      completed = builder->replaceTemporary(
          llvm::TempDICompositeType(temporary), completed);
      if (!selected->get_debug().replace_scope(type, llvm::wrap(completed))) {
        return selected->fail_backend(
            "LLVM cannot publish one completed contextual Type debug scope."_view);
      }

      continue;
    }

    if (!create_debug_type(*selected, type)) {
      return selected->fail_backend(
          "LLVM cannot complete one Type debug scope."_view);
    }
  }

  builder.visit(
      []() {},
      [](LLVMOpaqueDIBuilder& native) { llvm::unwrap(&native)->finalize(); });
  return True;
}
