// Perimortem Engine
// Copyright © Matt Kaes

// The native bridge enters LLVM before the Perimortem owner so LLVM's standard
// declarations remain confined to this implementation unit.
// clang-format off
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "tetrodotoxin/library/llvm/carriers.hpp"
// clang-format on

#include "perimortem/core/static/vector.hpp"

#include "perimortem/abi/memory/dynamic/object.hpp"
#include "tetrodotoxin/library/language/model/type.hpp"
#include "tetrodotoxin/library/language/model/types/flag.hpp"
#include "tetrodotoxin/library/language/model/types/real.hpp"
#include "tetrodotoxin/library/language/model/types/signed.hpp"
#include "tetrodotoxin/library/language/model/types/value.hpp"
#include "tetrodotoxin/library/language/types/access.hpp"
#include "tetrodotoxin/library/language/types/enumeration.hpp"
#include "tetrodotoxin/library/language/types/fixed.hpp"
#include "tetrodotoxin/library/language/types/object.hpp"
#include "tetrodotoxin/library/language/types/option.hpp"
#include "tetrodotoxin/library/language/types/range.hpp"
#include "tetrodotoxin/library/language/types/structure.hpp"
#include "tetrodotoxin/library/language/types/view.hpp"
#include "tetrodotoxin/library/llvm/body.hpp"
#include "tetrodotoxin/library/llvm/program.hpp"
#include "tetrodotoxin/library/llvm/symbol.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin::Library;

static auto llvm_text(Core::View::Bytes value) -> llvm::StringRef {
  return llvm::StringRef(
      reinterpret_cast<const char*>(value.get_data()), value.get_size());
}

static auto get_target(Ttx::Concept::Abstract& program)
    -> Core::Option<Tetrodotoxin::Library::Llvm::Program&> {
  return program.select<Tetrodotoxin::Library::Llvm::Program>();
}

static auto get_body(Ttx::Concept::Abstract& body)
    -> Core::Option<Tetrodotoxin::Library::Llvm::Body&> {
  return body.select<Tetrodotoxin::Library::Llvm::Body>();
}

static auto get_program(Ttx::Concept::Abstract& body)
    -> Ttx::Concept::Abstract& {
  auto selected = get_body(body);
  return selected ? selected->get_program() : body;
}

static auto get_context(Tetrodotoxin::Library::Llvm::Program& program)
    -> llvm::LLVMContext& {
  return *llvm::unwrap(&program.get_context());
}

static auto get_module(Tetrodotoxin::Library::Llvm::Program& program)
    -> llvm::Module& {
  return *llvm::unwrap(&program.get_module());
}

static auto get_builder(Tetrodotoxin::Library::Llvm::Body& body)
    -> llvm::IRBuilder<>& {
  return *llvm::unwrap(body.get_builder());
}

static auto get_function(Tetrodotoxin::Library::Llvm::Body& body)
    -> llvm::Function& {
  return *llvm::unwrap<llvm::Function>(body.get_function());
}

static auto fail_backend(
    Ttx::Concept::Abstract& program,
    Core::View::Bytes message) -> Bool {
  auto target = get_target(program);
  return target ? target->fail_backend(message) : False;
}

static auto fail_type(
    Ttx::Concept::Abstract& program,
    const Ttx::Model::Type& type,
    Core::View::Bytes message,
    Core::View::Bytes hint = {}) -> Bool {
  auto target = get_target(program);
  auto library_type = type.select<Language::Model::Type>();
  auto anchor = library_type ? library_type->get_declaration_anchor()
                             : Core::Option<Ttx::Lexical::Anchor>();
  if (target && anchor) {
    return target->fail_source(*anchor, message, hint);
  }

  return fail_backend(program, message);
}

template <typename contract>
static auto select_contract(
    Ttx::Concept::Abstract& program,
    const Ttx::Model::Type& type) -> Core::Option<const contract&> {
  auto selected = type.select<contract>();
  if (!selected) {
    fail_backend(
        program,
        "LLVM caller selected a carrier contract that does not match its source Type."_view);
  }

  return selected;
}

static auto select_field_type(const Ttx::Concept::Layout& fields, Count index)
    -> Core::Option<const Ttx::Model::Type&> {
  auto entry = fields.get_abstract(index);
  auto field = entry ? entry->select<Ttx::Model::Addressable>()
                     : Core::Option<const Ttx::Model::Addressable&>();
  return field ? Core::Option<const Ttx::Model::Type&>(field->get_type())
               : Core::Option<const Ttx::Model::Type&>();
}

auto Tetrodotoxin::Library::Llvm::Carriers::publish(
    Ttx::Concept::Abstract& program,
    const Ttx::Model::Type& type,
    Carrier carrier) const -> Core::Option<Bool> {
  auto found = carriers.find(&type);
  if (found) {
    if (found->value.kind != carrier.kind) {
      fail_backend(
          program,
          "LLVM received different carrier owners for one Type identity."_view);
      return {};
    }

    return False;
  }

  carriers.insert(&type, carrier);
  return True;
}

auto Tetrodotoxin::Library::Llvm::Carriers::reserve(
    Ttx::Concept::Abstract& program,
    const Ttx::Model::Type& type,
    Kind kind) const -> Core::Option<Bool> {
  auto target = get_target(program);
  if (!target) {
    return {};
  }

  switch (kind) {
  case Kind::Value: {
    auto value = select_contract<Language::Model::Types::Value>(program, type);
    if (!value) {
      return {};
    }

    Count width = value->get_width();
    Bool real = value->is<Language::Model::Types::Real>();
    Bool signed_value = value->is<Language::Model::Types::Signed>();
    Bool flag = value->is<Language::Model::Types::Flag>();
    Core::Option<llvm::Type&> native;
    if (real && width == 32) {
      native = *llvm::Type::getFloatTy(get_context(*target));
    } else if (real && width == 64) {
      native = *llvm::Type::getDoubleTy(get_context(*target));
    } else if (!real && width != 0 && width <= Unsigned_32(-1)) {
      native =
          *llvm::IntegerType::get(get_context(*target), Unsigned_32(width));
    } else {
      fail_type(
          program, type,
          "LLVM cannot represent the selected scalar width."_view,
          "Select a nonzero integer width or a 32 or 64 bit Real Type."_view);
      return {};
    }

    Carrier carrier{
      .kind = kind,
      .native = llvm::wrap(&*native),
      .real = real,
      .signed_value = signed_value,
      .flag_value = flag,
      .width = width,
    };
    return publish(program, type, carrier);
  }

  case Kind::Enumeration:
  case Kind::Fixed:
  case Kind::Range:
  case Kind::Context:
    return publish(program, type, Carrier{.kind = kind});

  case Kind::Option: {
    Symbol name(target->get_arena(), type, Symbol::Kind::OptionType);
    llvm::StructType& native = *llvm::StructType::create(
        get_context(*target), llvm_text(name.get_view()));
    return publish(
        program, type, Carrier{.kind = kind, .native = llvm::wrap(&native)});
  }

  case Kind::View:
  case Kind::Access: {
    Core::Static::Vector<llvm::Type*, 2> members = {{
      llvm::PointerType::getUnqual(get_context(*target)),
      llvm::Type::getInt64Ty(get_context(*target)),
    }};
    llvm::StructType& native = *llvm::StructType::get(
        get_context(*target),
        llvm::ArrayRef<llvm::Type*>(members.get_data(), members.get_size()));
    return publish(
        program, type, Carrier{.kind = kind, .native = llvm::wrap(&native)});
  }

  case Kind::Structure: {
    Symbol name(target->get_arena(), type, Symbol::Kind::StructureType);
    llvm::StructType& native = *llvm::StructType::create(
        get_context(*target), llvm_text(name.get_view()));
    return publish(
        program, type,
        Carrier{
          .kind = kind,
          .native = llvm::wrap(&native),
          .payload = llvm::wrap(&native),
        });
  }

  case Kind::Object: {
    Symbol payload_name(target->get_arena(), type, Symbol::Kind::ObjectType);
    llvm::Type& native = *llvm::PointerType::getUnqual(get_context(*target));
    llvm::StructType& payload = *llvm::StructType::create(
        get_context(*target), llvm_text(payload_name.get_view()));
    return publish(
        program, type,
        Carrier{
          .kind = kind,
          .native = llvm::wrap(&native),
          .payload = llvm::wrap(&payload),
        });
  }
  }
}

auto Tetrodotoxin::Library::Llvm::Carriers::begin_completion(
    Ttx::Concept::Abstract& program,
    const Ttx::Model::Type& type) const -> Core::Option<Bool> {
  auto found = carriers.find(&type);
  if (!found) {
    fail_backend(
        program,
        "LLVM cannot complete a Type before its owner reserves a carrier."_view);
    return {};
  }

  if (found->value.phase != Phase::Reserved) {
    return False;
  }

  found->value.phase = Phase::Completing;
  return True;
}

auto Tetrodotoxin::Library::Llvm::Carriers::select_completion(
    Ttx::Concept::Abstract& program,
    const Ttx::Model::Type& type,
    Kind kind) const -> Core::Option<Carrier&> {
  auto found = carriers.find(&type);
  if (!found || found->value.kind != kind ||
      found->value.phase != Phase::Completing) {
    fail_backend(
        program,
        "LLVM received a carrier completion outside its reserved phase."_view);
    return {};
  }

  return found->value;
}

auto Tetrodotoxin::Library::Llvm::Carriers::complete(
    Ttx::Concept::Abstract& program,
    const Ttx::Model::Type& type,
    Kind kind) const -> Bool {
  switch (kind) {
  case Kind::Value: {
    auto carrier = select_completion(program, type, kind);
    if (!carrier || !carrier->native) {
      return False;
    }

    carrier->phase = Phase::Complete;
    return True;
  }

  case Kind::Enumeration: {
    auto enumeration =
        select_contract<Language::Types::Enumeration>(program, type);
    if (!enumeration) {
      return False;
    }

    auto storage = enumeration->get_storage_type();
    if (!storage) {
      return fail_type(
          program, type,
          "LLVM received the Enumeration carrier contract without its storage Type."_view,
          "Complete the exact Enumeration before lowering its carrier."_view);
    }

    auto carrier = select_completion(program, type, kind);
    auto storage_carrier = carriers.find(&*storage);
    auto native = get_type(*storage);
    if (!carrier || !storage_carrier ||
        storage_carrier->value.phase != Phase::Complete || !native) {
      return fail_backend(
          program,
          "LLVM cannot complete an Enumeration before its storage carrier."_view);
    }

    carrier->native = *native;
    carrier->element = *storage;
    carrier->real = is_real(*storage);
    carrier->signed_value = is_signed(*storage);
    carrier->phase = Phase::Complete;
    return True;
  }

  case Kind::Fixed: {
    auto fixed = select_contract<Language::Types::Fixed>(program, type);
    if (!fixed) {
      return False;
    }

    const Ttx::Model::Type& element = fixed->get_element_type();
    Count extent = Count(fixed->get_extent());
    auto carrier = select_completion(program, type, kind);
    auto element_carrier = carriers.find(&element);
    auto native = get_type(element);
    Bool element_ready = Bool(
        element_carrier && (element_carrier->value.phase == Phase::Complete ||
                            element_carrier->value.kind == Kind::Object));
    if (!carrier || !element_ready || !native || extent == 0) {
      return fail_backend(
          program,
          "LLVM cannot complete Fixed before its element carrier and extent."_view);
    }

    carrier->native =
        llvm::wrap(llvm::ArrayType::get(llvm::unwrap(*native), extent));
    carrier->element = element;
    carrier->extent = extent;
    carrier->phase = Phase::Complete;
    return True;
  }

  case Kind::Option: {
    auto option = select_contract<Language::Types::Option>(program, type);
    if (!option) {
      return False;
    }

    const Ttx::Model::Type& element = option->get_element_type();
    const Ttx::Model::Type& flag = option->get_flag_type();
    auto carrier = select_completion(program, type, kind);
    auto element_carrier = carriers.find(&element);
    auto flag_carrier = carriers.find(&flag);
    auto payload = get_type(element);
    auto selected = get_type(flag);
    if (element_carrier && element_carrier->value.phase == Phase::Completing &&
        element_carrier->value.kind != Kind::Object) {
      return fail_type(
          program, element,
          "Library Type recursively contains itself through inline target storage."_view,
          "Break the value cycle with Object or another reference carrier."_view);
    }

    Bool element_ready = Bool(
        element_carrier && (element_carrier->value.phase == Phase::Complete ||
                            element_carrier->value.kind == Kind::Object));
    if (!carrier || !carrier->native || !element_ready || !flag_carrier ||
        flag_carrier->value.phase != Phase::Complete || !payload || !selected) {
      return fail_backend(
          program,
          "LLVM cannot complete Option before its payload and flag carriers."_view);
    }

    auto& native =
        *llvm::cast<llvm::StructType>(llvm::unwrap(*carrier->native));
    native.setBody({llvm::unwrap(*payload), llvm::unwrap(*selected)}, false);
    carrier->element = element;
    carrier->flag = flag;
    carrier->phase = Phase::Complete;
    return True;
  }

  case Kind::Range: {
    auto range = select_contract<Language::Types::Range>(program, type);
    if (!range) {
      return False;
    }

    const Ttx::Model::Type& element = range->get_element_type();
    auto target = get_target(program);
    auto carrier = select_completion(program, type, kind);
    auto element_carrier = carriers.find(&element);
    auto native = get_type(element);
    if (!target || !carrier || !element_carrier ||
        element_carrier->value.phase != Phase::Complete || !native) {
      return fail_backend(
          program,
          "LLVM cannot complete Range before its element carrier."_view);
    }

    carrier->native = llvm::wrap(
        llvm::StructType::get(
            get_context(*target),
            {llvm::unwrap(*native), llvm::unwrap(*native)}));
    carrier->element = element;
    carrier->phase = Phase::Complete;
    return True;
  }

  case Kind::View: {
    auto view = select_contract<Language::Types::View>(program, type);
    if (!view) {
      return False;
    }

    return complete_contiguous(program, type, view->get_element_type(), kind);
  }

  case Kind::Access: {
    auto access = select_contract<Language::Types::Access>(program, type);
    if (!access) {
      return False;
    }

    return complete_contiguous(program, type, access->get_element_type(), kind);
  }

  case Kind::Structure: {
    auto structure = select_contract<Language::Types::Structure>(program, type);
    if (!structure) {
      return False;
    }

    return complete_aggregate(program, type, structure->get_layout(), kind);
  }

  case Kind::Object: {
    auto object = select_contract<Language::Types::Object>(program, type);
    if (!object) {
      return False;
    }

    return complete_aggregate(program, type, object->get_layout(), kind);
  }

  case Kind::Context: {
    auto carrier = select_completion(program, type, kind);
    if (!carrier) {
      return False;
    }

    carrier->phase = Phase::Complete;
    return True;
  }
  }
}

auto Tetrodotoxin::Library::Llvm::Carriers::complete_contiguous(
    Ttx::Concept::Abstract& program,
    const Ttx::Model::Type& type,
    const Ttx::Model::Type& element,
    Kind kind) const -> Bool {
  auto carrier = select_completion(program, type, kind);
  auto native = get_type(element);
  if (!carrier || !carrier->native || !native) {
    return fail_backend(
        program,
        "LLVM cannot complete contiguous storage before its element carrier."_view);
  }

  carrier->element = element;
  carrier->phase = Phase::Complete;
  return True;
}

auto Tetrodotoxin::Library::Llvm::Carriers::complete_aggregate(
    Ttx::Concept::Abstract& program,
    const Ttx::Model::Type& type,
    const Ttx::Concept::Layout& fields,
    Kind kind) const -> Bool {
  auto carrier = select_completion(program, type, kind);
  if (!carrier || !carrier->payload) {
    return False;
  }

  Memory::Dynamic::Vector<LLVMTypeRef> native_fields(fields.get_size());
  for (Count index = 0; index < fields.get_size(); index++) {
    auto entry = fields.get_abstract(index);
    auto field = entry ? entry->select<Ttx::Model::Addressable>()
                       : Core::Option<const Ttx::Model::Addressable&>();
    if (!field) {
      return fail_backend(
          program,
          "LLVM received an aggregate Layout entry without an Addressable."_view);
    }

    const Ttx::Model::Type& field_type = field->get_type();
    auto field_carrier = carriers.find(&field_type);
    if (!field_carrier || !field_carrier->value.native) {
      return fail_backend(
          program,
          "LLVM cannot complete an aggregate before every Field carrier."_view);
    }

    Bool recursive_inline = Bool(
        field_carrier->value.phase == Phase::Completing &&
        field_carrier->value.kind != Kind::Object && kind != Kind::Object);
    if (recursive_inline) {
      return fail_type(
          program, type,
          "LLVM cannot lay out an inline Type that recursively contains itself."_view,
          "Break recursive inline storage with an Object reference or remove the recursive Field."_view);
    }

    native_fields.insert(*field_carrier->value.native);
    field_indices.insert(field.operator->(), index);
    field_hosts.insert(field.operator->(), &type);
  }

  llvm::cast<llvm::StructType>(llvm::unwrap(*carrier->payload))
      ->setBody(
          llvm::ArrayRef<llvm::Type*>(
              llvm::unwrap(native_fields.get_data()), native_fields.get_size()),
          false);
  carrier->fields = fields;
  carrier->phase = Phase::Complete;
  return True;
}

auto Tetrodotoxin::Library::Llvm::Carriers::get_type(
    const Ttx::Model::Type& type) const -> Core::Option<LLVMTypeRef> {
  auto found = carriers.find(&type);
  return found && found->value.native ? found->value.native
                                      : Core::Option<LLVMTypeRef>();
}

auto Tetrodotoxin::Library::Llvm::Carriers::get_payload(
    const Ttx::Model::Type& type) const -> Core::Option<LLVMTypeRef> {
  auto found = carriers.find(&type);
  return found && found->value.payload ? found->value.payload
                                       : Core::Option<LLVMTypeRef>();
}

auto Tetrodotoxin::Library::Llvm::Carriers::get_kind(
    const Ttx::Model::Type& type) const -> Core::Option<Kind> {
  auto found = carriers.find(&type);
  if (!found || found->value.phase != Phase::Complete) {
    return {};
  }

  return found->value.kind;
}

auto Tetrodotoxin::Library::Llvm::Carriers::get_width(
    const Ttx::Model::Type& type) const -> Core::Option<Count> {
  auto found = carriers.find(&type);
  if (!found || found->value.phase != Phase::Complete ||
      found->value.kind != Kind::Value || found->value.width == 0) {
    return {};
  }

  return found->value.width;
}

auto Tetrodotoxin::Library::Llvm::Carriers::get_element(
    const Ttx::Model::Type& type) const
    -> Core::Option<const Ttx::Model::Type&> {
  auto found = carriers.find(&type);
  if (!found || found->value.phase != Phase::Complete ||
      !found->value.element) {
    return {};
  }

  return *found->value.element;
}

auto Tetrodotoxin::Library::Llvm::Carriers::get_flag(
    const Ttx::Model::Type& type) const
    -> Core::Option<const Ttx::Model::Type&> {
  auto found = carriers.find(&type);
  if (!found || found->value.phase != Phase::Complete || !found->value.flag) {
    return {};
  }

  return *found->value.flag;
}

auto Tetrodotoxin::Library::Llvm::Carriers::get_extent(
    const Ttx::Model::Type& type) const -> Core::Option<Count> {
  auto found = carriers.find(&type);
  if (!found || found->value.phase != Phase::Complete ||
      found->value.kind != Kind::Fixed || found->value.extent == 0) {
    return {};
  }

  return found->value.extent;
}

auto Tetrodotoxin::Library::Llvm::Carriers::get_fields(
    const Ttx::Model::Type& type) const
    -> Core::Option<const Ttx::Concept::Layout&> {
  auto found = carriers.find(&type);
  if (!found || found->value.phase != Phase::Complete || !found->value.fields) {
    return {};
  }

  return *found->value.fields;
}

auto Tetrodotoxin::Library::Llvm::Carriers::get_field_index(
    const Ttx::Model::Addressable& field) const -> Core::Option<Count> {
  auto found = field_indices.find(&field);
  return found ? Core::Option<Count>(found->value) : Core::Option<Count>();
}

auto Tetrodotoxin::Library::Llvm::Carriers::get_field_host(
    const Ttx::Model::Addressable& field) const
    -> Core::Option<const Ttx::Model::Type&> {
  auto found = field_hosts.find(&field);
  return found ? Core::Option<const Ttx::Model::Type&>(*found->value)
               : Core::Option<const Ttx::Model::Type&>();
}

auto Tetrodotoxin::Library::Llvm::Carriers::is_real(
    const Ttx::Model::Type& type) const -> Bool {
  auto found = carriers.find(&type);
  return found ? found->value.real : False;
}

auto Tetrodotoxin::Library::Llvm::Carriers::is_signed(
    const Ttx::Model::Type& type) const -> Bool {
  auto found = carriers.find(&type);
  return found ? found->value.signed_value : False;
}

auto Tetrodotoxin::Library::Llvm::Carriers::is_flag(
    const Ttx::Model::Type& type) const -> Bool {
  auto found = carriers.find(&type);
  return found ? found->value.flag_value : False;
}

auto Tetrodotoxin::Library::Llvm::Carriers::is_object(
    const Ttx::Model::Type& type) const -> Bool {
  auto found = carriers.find(&type);
  return Bool(found && found->value.kind == Kind::Object);
}

auto Tetrodotoxin::Library::Llvm::Carriers::zero(
    Ttx::Concept::Abstract& program,
    const Ttx::Model::Type& type) const -> Core::Option<LLVMValueRef> {
  auto native = get_type(type);
  if (!native) {
    fail_backend(
        program,
        "LLVM cannot create a zero value without a completed carrier."_view);
    return {};
  }

  return llvm::wrap(llvm::Constant::getNullValue(llvm::unwrap(*native)));
}

auto Tetrodotoxin::Library::Llvm::Carriers::owns_resources(
    const Ttx::Model::Type& type) const -> Bool {
  Memory::Dynamic::Vector<Ttx::Concept::Reference<const Ttx::Model::Type>>
      active;
  return owns_resources(type, active);
}

auto Tetrodotoxin::Library::Llvm::Carriers::owns_resources(
    const Ttx::Model::Type& type,
    Memory::Dynamic::Vector<Ttx::Concept::Reference<const Ttx::Model::Type>>&
        active) const -> Bool {
  auto found = carriers.find(&type);
  if (!found) {
    return False;
  }

  const Carrier& carrier = found->value;
  if (carrier.kind == Kind::Object) {
    return True;
  }

  Ttx::Concept::Reference<const Ttx::Model::Type> retained(type);
  if (active.contains(retained)) {
    return False;
  }

  active.insert(retained);
  Bool result = False;
  if ((carrier.kind == Kind::Option || carrier.kind == Kind::Fixed) &&
      carrier.element) {
    result = owns_resources(*carrier.element, active);
  } else if (carrier.kind == Kind::Structure && carrier.fields) {
    for (Count index = 0; index < carrier.fields->get_size(); index++) {
      auto field = select_field_type(*carrier.fields, index);
      if (field && owns_resources(*field, active)) {
        result = True;
        break;
      }
    }
  }

  active.remove(active.get_size() - 1);
  return result;
}

auto Tetrodotoxin::Library::Llvm::Carriers::retain(
    Ttx::Concept::Abstract& body,
    const Ttx::Model::Type& type,
    LLVMValueRef value) const -> Bool {
  auto native_body = get_body(body);
  auto found = carriers.find(&type);
  if (!native_body || !found || !value) {
    return fail_backend(
        get_program(body),
        "LLVM cannot retain a value without its completed carrier."_view);
  }

  const Carrier& carrier = found->value;
  llvm::IRBuilder<>& builder = get_builder(*native_body);
  llvm::Value& native_value = *llvm::unwrap(value);
  if (!owns_resources(type)) {
    return True;
  }

  if (carrier.kind == Kind::Object) {
    auto target = get_target(get_program(body));
    if (!target) {
      return False;
    }

    llvm::FunctionType& signature = *llvm::FunctionType::get(
        llvm::Type::getVoidTy(get_context(*target)),
        {llvm::PointerType::getUnqual(get_context(*target))}, false);
    builder.CreateCall(
        get_module(*target).getOrInsertFunction(
            llvm_text(Abi::Memory::Dynamic::Object::retain_symbol), &signature),
        {&native_value});
  } else if (carrier.kind == Kind::Option && carrier.element) {
    llvm::BasicBlock& copy = *llvm::BasicBlock::Create(
        get_function(*native_body).getContext(), "option.copy",
        &get_function(*native_body));
    llvm::BasicBlock& done = *llvm::BasicBlock::Create(
        get_function(*native_body).getContext(), "option.copy.done",
        &get_function(*native_body));
    llvm::Value& selected =
        *builder.CreateExtractValue(&native_value, Unsigned_32(1));
    builder.CreateCondBr(&selected, &copy, &done);
    builder.SetInsertPoint(&copy);
    llvm::Value& payload =
        *builder.CreateExtractValue(&native_value, Unsigned_32(0));
    if (!retain(body, *carrier.element, llvm::wrap(&payload))) {
      return False;
    }

    builder.CreateBr(&done);
    builder.SetInsertPoint(&done);
  } else if (carrier.kind == Kind::Fixed && carrier.element) {
    for (Count index = 0; index < carrier.extent; index++) {
      llvm::Value& element =
          *builder.CreateExtractValue(&native_value, Unsigned_32(index));
      if (!retain(body, *carrier.element, llvm::wrap(&element))) {
        return False;
      }
    }
  } else if (carrier.kind == Kind::Structure) {
    if (!carrier.fields) {
      return fail_backend(
          get_program(body),
          "LLVM cannot retain a Structure without its completed Layout."_view);
    }

    for (Count index = 0; index < carrier.fields->get_size(); index++) {
      auto field = select_field_type(*carrier.fields, index);
      if (!field) {
        return fail_backend(
            get_program(body),
            "LLVM cannot retain a Structure with an invalid Field edge."_view);
      }

      if (!owns_resources(*field)) {
        continue;
      }

      llvm::Value& selected =
          *builder.CreateExtractValue(&native_value, Unsigned_32(index));
      if (!retain(body, *field, llvm::wrap(&selected))) {
        return False;
      }
    }
  }

  return True;
}

auto Tetrodotoxin::Library::Llvm::Carriers::release(
    Ttx::Concept::Abstract& body,
    const Ttx::Model::Type& type,
    LLVMValueRef value) const -> Bool {
  auto native_body = get_body(body);
  auto found = carriers.find(&type);
  if (!native_body || !found || !value) {
    return fail_backend(
        get_program(body),
        "LLVM cannot release a value without its completed carrier."_view);
  }

  const Carrier& carrier = found->value;
  llvm::IRBuilder<>& builder = get_builder(*native_body);
  llvm::Value& native_value = *llvm::unwrap(value);
  if (!owns_resources(type)) {
    return True;
  }

  if (carrier.kind == Kind::Object) {
    auto target = get_target(get_program(body));
    if (!target) {
      return False;
    }

    llvm::FunctionType& signature = *llvm::FunctionType::get(
        llvm::Type::getVoidTy(get_context(*target)),
        {llvm::PointerType::getUnqual(get_context(*target))}, false);
    builder.CreateCall(
        get_module(*target).getOrInsertFunction(
            llvm_text(Abi::Memory::Dynamic::Object::release_symbol),
            &signature),
        {&native_value});
  } else if (carrier.kind == Kind::Option && carrier.element) {
    llvm::BasicBlock& drop = *llvm::BasicBlock::Create(
        get_function(*native_body).getContext(), "option.drop",
        &get_function(*native_body));
    llvm::BasicBlock& done = *llvm::BasicBlock::Create(
        get_function(*native_body).getContext(), "option.drop.done",
        &get_function(*native_body));
    llvm::Value& selected =
        *builder.CreateExtractValue(&native_value, Unsigned_32(1));
    builder.CreateCondBr(&selected, &drop, &done);
    builder.SetInsertPoint(&drop);
    llvm::Value& payload =
        *builder.CreateExtractValue(&native_value, Unsigned_32(0));
    if (!release(body, *carrier.element, llvm::wrap(&payload))) {
      return False;
    }

    builder.CreateBr(&done);
    builder.SetInsertPoint(&done);
  } else if (carrier.kind == Kind::Fixed && carrier.element) {
    for (Count index = carrier.extent; index != 0; index--) {
      llvm::Value& element =
          *builder.CreateExtractValue(&native_value, Unsigned_32(index - 1));
      if (!release(body, *carrier.element, llvm::wrap(&element))) {
        return False;
      }
    }
  } else if (carrier.kind == Kind::Structure) {
    if (!carrier.fields) {
      return fail_backend(
          get_program(body),
          "LLVM cannot release a Structure without its completed Layout."_view);
    }

    for (Count index = carrier.fields->get_size(); index != 0; index--) {
      auto field = select_field_type(*carrier.fields, index - 1);
      if (!field) {
        return fail_backend(
            get_program(body),
            "LLVM cannot release a Structure with an invalid Field edge."_view);
      }

      if (!owns_resources(*field)) {
        continue;
      }

      llvm::Value& selected =
          *builder.CreateExtractValue(&native_value, Unsigned_32(index - 1));
      if (!release(body, *field, llvm::wrap(&selected))) {
        return False;
      }
    }
  }

  return True;
}

auto Tetrodotoxin::Library::Llvm::Carriers::assemble(
    Ttx::Concept::Abstract& body,
    const Ttx::Model::Type& type,
    Core::View::Vector<LLVMValueRef> elements) const
    -> Core::Option<LLVMValueRef> {
  auto native_body = get_body(body);
  auto found = carriers.find(&type);
  auto native = get_type(type);
  if (!native_body || !found || !native) {
    fail_backend(
        get_program(body),
        "LLVM cannot assemble a value without its completed carrier."_view);
    return {};
  }

  for (LLVMValueRef element : elements) {
    if (!element) {
      fail_backend(
          get_program(body),
          "LLVM received an absent native value during assembly."_view);
      return {};
    }
  }

  llvm::Type& native_type = *llvm::unwrap(*native);
  if (elements.get_size() == 1 &&
      llvm::unwrap(elements[0])->getType() == &native_type) {
    return elements[0];
  }

  const Carrier& carrier = found->value;
  llvm::IRBuilder<>& builder = get_builder(*native_body);
  if (carrier.kind == Kind::Option && carrier.element) {
    if (elements.get_size() == 0) {
      return zero(get_program(body), type);
    }

    auto payload = assemble(body, *carrier.element, elements);
    if (!payload || !native_body->acquire(*carrier.element, *payload)) {
      return {};
    }

    llvm::Value& aggregate = *llvm::UndefValue::get(&native_type);
    llvm::Value& with_payload = *builder.CreateInsertValue(
        &aggregate, llvm::unwrap(*payload), Unsigned_32(0));
    llvm::Value& selected = *builder.CreateInsertValue(
        &with_payload, builder.getTrue(), Unsigned_32(1));
    native_body->mark_owned(type, llvm::wrap(&selected));
    return llvm::wrap(&selected);
  } else if (carrier.kind == Kind::Object) {
    fail_backend(
        get_program(body),
        "LLVM cannot assemble an Object handle from inline payload values."_view);
    return {};
  } else if (carrier.kind == Kind::Fixed && carrier.element) {
    if (elements.get_size() != carrier.extent) {
      fail_backend(
          get_program(body),
          "LLVM cannot align Fixed values with its exact extent."_view);
      return {};
    }

    llvm::Value* aggregate = llvm::UndefValue::get(&native_type);
    for (Count index = 0; index < elements.get_size(); index++) {
      Core::View::Vector<LLVMValueRef> selected(elements.get_data() + index, 1);
      auto element = assemble(body, *carrier.element, selected);
      if (!element || !native_body->acquire(*carrier.element, *element)) {
        return {};
      }

      aggregate = builder.CreateInsertValue(
          aggregate, llvm::unwrap(*element), Unsigned_32(index));
    }

    native_body->mark_owned(type, llvm::wrap(aggregate));
    return llvm::wrap(aggregate);
  } else if (carrier.kind == Kind::Structure) {
    if (!carrier.fields || elements.get_size() != carrier.fields->get_size()) {
      fail_backend(
          get_program(body),
          "LLVM cannot align Structure values with its instance Fields."_view);
      return {};
    }

    llvm::Value* aggregate = llvm::UndefValue::get(&native_type);
    for (Count index = 0; index < elements.get_size(); index++) {
      auto field_type = select_field_type(*carrier.fields, index);
      if (!field_type) {
        fail_backend(
            get_program(body),
            "LLVM cannot assemble a Structure with an invalid Field edge."_view);
        return {};
      }

      Core::View::Vector<LLVMValueRef> selected(elements.get_data() + index, 1);
      auto field = assemble(body, *field_type, selected);
      if (!field || !native_body->acquire(*field_type, *field)) {
        return {};
      }

      aggregate = builder.CreateInsertValue(
          aggregate, llvm::unwrap(*field), Unsigned_32(index));
    }

    native_body->mark_owned(type, llvm::wrap(aggregate));
    return llvm::wrap(aggregate);
  }

  llvm::Value* aggregate = llvm::UndefValue::get(&native_type);
  for (Count index = 0; index < elements.get_size(); index++) {
    if (native_type.isSingleValueType()) {
      fail_backend(
          get_program(body),
          "LLVM cannot assemble one scalar carrier from multiple values."_view);
      return {};
    }

    aggregate = builder.CreateInsertValue(
        aggregate, llvm::unwrap(elements[index]), Unsigned_32(index));
  }

  return llvm::wrap(aggregate);
}

auto Tetrodotoxin::Library::Llvm::Carriers::fit_values(
    Ttx::Concept::Abstract& body,
    const Ttx::Model::Pack& source,
    const Ttx::Concept::Layout& target,
    Core::View::Vector<LLVMValueRef> values) const
    -> Core::Option<Memory::Dynamic::Vector<LLVMValueRef>> {
  const Ttx::Concept::Layout& supplied = source.get_layout();
  if (values.get_size() != supplied.get_size() ||
      values.get_size() != target.get_size()) {
    fail_backend(
        get_program(body),
        "LLVM cannot align a completed Pack with its fitted target size."_view);
    return {};
  }

  Bool named = False;
  for (Count index = 0; index < supplied.get_size(); index++) {
    if (supplied.get_name(index)) {
      named = True;
      break;
    }
  }

  Memory::Dynamic::Vector<LLVMValueRef> fitted(values.get_size());
  if (!named) {
    for (LLVMValueRef value : values) {
      fitted.insert(value);
    }

    return fitted;
  }

  for (Count target_index = 0; target_index < target.get_size();
       target_index++) {
    Count selected = 0;
    Count matches = 0;
    for (Count source_index = 0; source_index < values.get_size();
         source_index++) {
      if (supplied.fits_entry(target, source_index, target_index)) {
        selected = source_index;
        matches++;
      }
    }

    if (matches != 1) {
      fail_backend(
          get_program(body),
          "LLVM cannot select one producer for a fitted target entry."_view);
      return {};
    }

    fitted.insert(values[selected]);
  }

  return fitted;
}

auto Tetrodotoxin::Library::Llvm::Carriers::fit_and_assemble(
    Ttx::Concept::Abstract& body,
    const Ttx::Model::Type& type,
    const Ttx::Model::Pack& source,
    Core::View::Vector<LLVMValueRef> elements) const
    -> Core::Option<LLVMValueRef> {
  auto found = carriers.find(&type);
  auto native = get_type(type);
  if (!found || !native) {
    fail_backend(
        get_program(body),
        "LLVM cannot fit values without a completed target carrier."_view);
    return {};
  }

  if (elements.get_size() == 1 &&
      llvm::unwrap(elements[0])->getType() == llvm::unwrap(*native)) {
    return elements[0];
  }

  const Carrier& carrier = found->value;
  if (carrier.kind == Kind::Fixed && elements.get_size() == carrier.extent) {
    return assemble(body, type, elements);
  }

  if (carrier.kind == Kind::Option && carrier.element) {
    if (elements.get_size() == 0) {
      return assemble(body, type, elements);
    }

    auto payload_type = get_type(*carrier.element);
    if (elements.get_size() == 1 && payload_type &&
        llvm::unwrap(elements[0])->getType() == llvm::unwrap(*payload_type)) {
      return assemble(body, type, elements);
    }

    auto fitted =
        fit_values(body, source, carrier.element->get_layout(), elements);
    return fitted ? assemble(body, type, fitted->get_view())
                  : Core::Option<LLVMValueRef>();
  }

  auto fitted = fit_values(body, source, type.get_layout(), elements);
  return fitted ? assemble(body, type, fitted->get_view())
                : Core::Option<LLVMValueRef>();
}

auto Tetrodotoxin::Library::Llvm::Carriers::fit(
    Ttx::Concept::Abstract& body,
    const Ttx::Model::Pack& source,
    const Ttx::Concept::Layout& target,
    Core::View::Vector<LLVMValueRef> values) const
    -> Core::Option<Memory::Dynamic::Vector<LLVMValueRef>> {
  return fit_values(body, source, target, values);
}

auto Tetrodotoxin::Library::Llvm::Carriers::get_object_finalizer(
    Ttx::Concept::Abstract& program,
    const Ttx::Model::Type& type) const -> Core::Option<LLVMValueRef> {
  auto found = carriers.find(&type);
  if (!found || found->value.kind != Kind::Object ||
      found->value.phase != Phase::Complete || !found->value.payload ||
      !found->value.fields) {
    fail_backend(
        program,
        "LLVM cannot select an Object finalizer before carrier completion."_view);
    return {};
  }

  Carrier& carrier = found->value;
  auto target = get_target(program);
  if (!target) {
    return {};
  }

  if (!carrier.finalizer) {
    Symbol name(target->get_arena(), type, Symbol::Kind::ObjectFinalizer);
    llvm::FunctionType& signature = *llvm::FunctionType::get(
        llvm::Type::getVoidTy(get_context(*target)),
        {llvm::PointerType::getUnqual(get_context(*target))}, false);
    carrier.finalizer = llvm::wrap(
        llvm::Function::Create(
            &signature, llvm::GlobalValue::InternalLinkage,
            llvm_text(name.get_view()), get_module(*target)));
  }

  llvm::Function& finalizer =
      *llvm::cast<llvm::Function>(llvm::unwrap(*carrier.finalizer));
  if (!finalizer.empty()) {
    return llvm::wrap(&finalizer);
  }

  Body body(*target, type, llvm::wrap(&finalizer));
  llvm::IRBuilder<>& builder = get_builder(body);
  llvm::BasicBlock& entry =
      *llvm::BasicBlock::Create(finalizer.getContext(), "entry", &finalizer);
  builder.SetInsertPoint(&entry);
  llvm::Value& payload = *finalizer.getArg(0);
  for (Count index = carrier.fields->get_size(); index != 0; index--) {
    auto field = select_field_type(*carrier.fields, index - 1);
    if (!field) {
      fail_backend(
          program,
          "LLVM cannot finalize an Object with an invalid Field edge."_view);
      return {};
    }

    if (!owns_resources(*field)) {
      continue;
    }

    auto native = get_type(*field);
    if (!native) {
      fail_backend(
          program,
          "LLVM cannot emit an Object finalizer without every Field carrier."_view);
      return {};
    }

    llvm::Value& address = *builder.CreateStructGEP(
        llvm::unwrap(*carrier.payload), &payload, Unsigned_32(index - 1));
    llvm::Value& value = *builder.CreateLoad(llvm::unwrap(*native), &address);
    if (!release(body, *field, llvm::wrap(&value))) {
      return {};
    }
  }

  builder.CreateRetVoid();
  return llvm::wrap(&finalizer);
}

auto Tetrodotoxin::Library::Llvm::Carriers::construct(
    Ttx::Concept::Abstract& body,
    const Ttx::Model::Type& type,
    Core::View::Vector<LLVMValueRef> values) const
    -> Core::Option<LLVMValueRef> {
  auto native_body = get_body(body);
  auto found = carriers.find(&type);
  if (!found || found->value.kind != Kind::Object) {
    return assemble(body, type, values);
  }

  Carrier& carrier = found->value;
  auto target = get_target(get_program(body));
  auto finalizer = get_object_finalizer(get_program(body), type);
  if (!native_body || !target || !carrier.payload || !finalizer ||
      !carrier.fields || values.get_size() != carrier.fields->get_size()) {
    fail_backend(
        get_program(body),
        "LLVM cannot pair an Object initializer with its payload Layout."_view);
    return {};
  }

  llvm::FunctionType& signature = *llvm::FunctionType::get(
      llvm::PointerType::getUnqual(get_context(*target)),
      {llvm::Type::getInt64Ty(get_context(*target)),
       llvm::PointerType::getUnqual(get_context(*target))},
      false);
  llvm::IRBuilder<>& builder = get_builder(*native_body);
  llvm::Value& payload = *builder.CreateCall(
      get_module(*target).getOrInsertFunction(
          llvm_text(Abi::Memory::Dynamic::Object::allocate_symbol), &signature),
      {builder.getInt64(get_module(*target)
                            .getDataLayout()
                            .getTypeAllocSize(llvm::unwrap(*carrier.payload))
                            .getFixedValue()),
       llvm::unwrap(*finalizer)},
      "object");
  for (Count index = 0; index < carrier.fields->get_size(); index++) {
    auto field_type = select_field_type(*carrier.fields, index);
    if (!field_type) {
      fail_backend(
          get_program(body),
          "LLVM cannot construct an Object with an invalid Field edge."_view);
      return {};
    }

    Core::View::Vector<LLVMValueRef> selected(values.get_data() + index, 1);
    auto field = assemble(body, *field_type, selected);
    if (!field || !native_body->acquire(*field_type, *field)) {
      return {};
    }

    llvm::Value& address = *builder.CreateStructGEP(
        llvm::unwrap(*carrier.payload), &payload, Unsigned_32(index));
    builder.CreateStore(llvm::unwrap(*field), &address);
  }

  native_body->mark_owned(type, llvm::wrap(&payload));
  return llvm::wrap(&payload);
}
