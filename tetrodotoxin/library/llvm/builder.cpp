// Perimortem Engine
// Copyright © Matt Kaes

// LLVM must enter before Perimortem so the standard placement declaration is
// visible before the freestanding fallback used by Perimortem headers.
#if __has_include("llvm/IR/IRBuilder.h")
#include "llvm/IR/IRBuilder.h"
#else
#error LLVM IRBuilder is required by the Library native compiler
#endif

#include "perimortem/core/static/vector.hpp"

#include "llvm-c/Core.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Module.h"
#include "perimortem/abi/core/object.hpp"
#include "tetrodotoxin/library/language/model/callable.hpp"
#include "tetrodotoxin/library/llvm/builder.hpp"
#include "tetrodotoxin/library/llvm/carriers.hpp"
#include "tetrodotoxin/library/llvm/functions.hpp"
#include "tetrodotoxin/library/llvm/globals.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin::Library;

// Arithmetic validates exact carriers before choosing signed, unsigned, or
// real LLVM instructions.

static auto arithmetic_find_scalar(
    const Tetrodotoxin::Library::Llvm::Body& body,
    const Ttx::Model::Pack& pack) -> Core::Option<LLVMValueRef> {
  auto values = body.find_values(pack);
  if (!values || values->get_size() != 1) {
    return {};
  }

  return values->get_data()[0];
}

static auto arithmetic_select_carriers(const Llvm::Body& body)
    -> Core::Option<const Tetrodotoxin::Library::Llvm::Carriers&> {
  return body.get_program().get_carriers();
}

static auto arithmetic_has_native_carrier(
    const Tetrodotoxin::Library::Llvm::Carriers& carriers,
    const Ttx::Model::Type& carrier,
    LLVMValueRef left,
    Core::Option<LLVMValueRef> right = {}) -> Bool {
  auto native = carriers.get_type(carrier);
  if (!native || LLVMTypeOf(left) != *native) {
    return False;
  }

  return !right || LLVMTypeOf(*right) == *native;
}

static auto arithmetic_publish_scalar(
    Tetrodotoxin::Library::Llvm::Body& body,
    const Ttx::Model::Pack& result,
    LLVMValueRef value) -> Bool {
  Core::Static::Vector<LLVMValueRef, 1> native = {{value}};
  return body.publish_values(result, native.get_view());
}

auto Tetrodotoxin::Library::Llvm::Builder::arithmetic(
    Arithmetic operation,
    const Ttx::Model::Type& carrier,
    const Ttx::Model::Pack& result,
    const Ttx::Model::Pack& left,
    const Ttx::Model::Pack& right) const -> Bool {
  Llvm::Body& native_body = body;
  auto carriers = arithmetic_select_carriers(body);
  if (!carriers) {
    return False;
  }

  auto left_value = arithmetic_find_scalar(native_body, left);
  auto right_value = arithmetic_find_scalar(native_body, right);
  if (!left_value || !right_value ||
      !arithmetic_has_native_carrier(
          *carriers, carrier, *left_value, *right_value)) {
    return False;
  }

  Bool real = carriers->is_real(carrier);
  Bool signed_value = carriers->is_signed(carrier);
  switch (operation) {
  case Arithmetic::Add:
    return arithmetic_publish_scalar(
        native_body, result,
        real ? LLVMBuildFAdd(
                   native_body.get_builder(), *left_value, *right_value, "")
             : LLVMBuildAdd(
                   native_body.get_builder(), *left_value, *right_value, ""));

  case Arithmetic::Subtract:
    return arithmetic_publish_scalar(
        native_body, result,
        real ? LLVMBuildFSub(
                   native_body.get_builder(), *left_value, *right_value, "")
             : LLVMBuildSub(
                   native_body.get_builder(), *left_value, *right_value, ""));

  case Arithmetic::Multiply:
    return arithmetic_publish_scalar(
        native_body, result,
        real ? LLVMBuildFMul(
                   native_body.get_builder(), *left_value, *right_value, "")
             : LLVMBuildMul(
                   native_body.get_builder(), *left_value, *right_value, ""));

  case Arithmetic::Divide:
    return arithmetic_publish_scalar(
        native_body, result,
        real ? LLVMBuildFDiv(
                   native_body.get_builder(), *left_value, *right_value, "")
        : signed_value
            ? LLVMBuildSDiv(
                  native_body.get_builder(), *left_value, *right_value, "")
            : LLVMBuildUDiv(
                  native_body.get_builder(), *left_value, *right_value, ""));

  case Arithmetic::Modulo:
    return arithmetic_publish_scalar(
        native_body, result,
        real ? LLVMBuildFRem(
                   native_body.get_builder(), *left_value, *right_value, "")
        : signed_value
            ? LLVMBuildSRem(
                  native_body.get_builder(), *left_value, *right_value, "")
            : LLVMBuildURem(
                  native_body.get_builder(), *left_value, *right_value, ""));
  }
}

auto Tetrodotoxin::Library::Llvm::Builder::negate(
    const Ttx::Model::Type& carrier,
    const Ttx::Model::Pack& result,
    const Ttx::Model::Pack& operand) const -> Bool {
  Llvm::Body& native_body = body;
  auto carriers = arithmetic_select_carriers(body);
  if (!carriers) {
    return False;
  }

  auto value = arithmetic_find_scalar(native_body, operand);
  if (!value || !arithmetic_has_native_carrier(*carriers, carrier, *value)) {
    return False;
  }

  LLVMValueRef selected =
      carriers->is_real(carrier)
          ? LLVMBuildFNeg(native_body.get_builder(), *value, "")
          : LLVMBuildNeg(native_body.get_builder(), *value, "");
  return arithmetic_publish_scalar(native_body, result, selected);
}

// Calls receive arguments in resolved parameter order. Call owns semantic
// fitting while Builder owns native parameter carriers and result transport.

static auto call_select_carriers(const Llvm::Body& body)
    -> Core::Option<const Tetrodotoxin::Library::Llvm::Carriers&> {
  return body.get_program().get_carriers();
}

static auto call_select_functions(const Llvm::Body& body)
    -> Core::Option<const Tetrodotoxin::Library::Llvm::Functions&> {
  return body.get_program().get_functions();
}

static auto call_fail_backend(Llvm::Body& body, Core::View::Bytes message)
    -> Bool {
  return body.get_program().fail_backend(message);
}

static auto call_select_result_type(
    const Ttx::Concept::Layout& layout,
    Count index) -> Core::Option<const Ttx::Model::Type&> {
  auto entry = layout.get_abstract(index);
  if (!entry) {
    return {};
  }

  auto addressable = entry->select<Ttx::Model::Addressable>();
  if (addressable) {
    return addressable->get_type();
  }

  return entry->resolve().select<Ttx::Model::Type>();
}

static auto call_select_input_values(
    const Tetrodotoxin::Library::Llvm::Body& body,
    const Ttx::Model::Pack& source,
    Count offset,
    Count size) -> Core::Option<Core::View::Vector<LLVMValueRef>> {
  auto values = body.find_values(source);
  if (!values || values->get_size() != source.get_layout().get_size() ||
      size == 0 || offset > values->get_size() ||
      size > values->get_size() - offset) {
    return {};
  }

  return Core::View::Vector<LLVMValueRef>(values->get_data() + offset, size);
}

static auto call_assemble_input(
    Tetrodotoxin::Library::Llvm::Body& body,
    const Tetrodotoxin::Library::Llvm::Carriers& carriers,
    const Ttx::Model::Addressable& parameter,
    const Ttx::Model::Pack& source,
    Count offset,
    Count size) -> Core::Option<LLVMValueRef> {
  auto values = call_select_input_values(body, source, offset, size);
  if (!values) {
    call_fail_backend(
        body,
        "LLVM cannot bind a Callable parameter to its exact source Pack segment."_view);
    return {};
  }

  const Ttx::Model::Type& type = parameter.get_type();
  Bool complete_source =
      Bool(offset == 0 && size == source.get_layout().get_size());
  return complete_source
             ? carriers.fit_and_assemble(body, type, source, *values)
             : carriers.assemble(body, type, *values);
}

static auto call_release_owned(
    Tetrodotoxin::Library::Llvm::Body& body,
    const Tetrodotoxin::Library::Llvm::Carriers& carriers,
    const Ttx::Model::Type& type,
    LLVMValueRef value) -> Bool {
  if (!body.take_owned(value)) {
    return True;
  }

  return carriers.release(body, type, value);
}

static auto call_publish_empty(
    Tetrodotoxin::Library::Llvm::Body& body,
    const Ttx::Model::Pack& result) -> Bool {
  return body.publish_values(result, Core::View::Vector<LLVMValueRef>());
}

static auto call_publish_results(
    Tetrodotoxin::Library::Llvm::Body& body,
    const Tetrodotoxin::Library::Llvm::Carriers& carriers,
    const Ttx::Model::Pack& result,
    const Ttx::Concept::Layout& signature,
    Core::Option<LLVMOpaqueValue&> returned) -> Bool {
  if (result.get_layout().get_size() != signature.get_size()) {
    return call_fail_backend(
        body,
        "LLVM Callable output does not match its completed result signature."_view);
  }

  if (signature.is_empty()) {
    return call_publish_empty(body, result);
  }

  if (!returned) {
    return call_fail_backend(
        body,
        "LLVM received no native value for a nonempty Callable result Layout."_view);
  }

  LLVMValueRef native_returned = &*returned;
  if (signature.get_size() == 1) {
    auto type = call_select_result_type(signature, 0);
    auto native = type ? carriers.get_type(*type) : Core::Option<LLVMTypeRef>();
    if (!type || !native || LLVMTypeOf(native_returned) != *native) {
      return call_fail_backend(
          body,
          "LLVM received a Callable result with the wrong physical carrier."_view);
    }

    Core::Static::Vector<LLVMValueRef, 1> values = {{native_returned}};
    body.mark_owned(*type, native_returned);
    return body.publish_values(result, values.get_view());
  }

  LLVMTypeRef aggregate = LLVMTypeOf(native_returned);
  if (LLVMGetTypeKind(aggregate) != LLVMStructTypeKind ||
      LLVMCountStructElementTypes(aggregate) != signature.get_size()) {
    return call_fail_backend(
        body,
        "LLVM received a multi-result Callable without its exact aggregate carrier."_view);
  }

  Tetrodotoxin::Library::Llvm::Body::NativeValues values(signature.get_size());
  for (Count index = 0; index < signature.get_size(); index++) {
    auto type = call_select_result_type(signature, index);
    auto native = type ? carriers.get_type(*type) : Core::Option<LLVMTypeRef>();
    LLVMValueRef value = LLVMBuildExtractValue(
        body.get_builder(), native_returned, U32(index), "call.result");
    if (!type || !native || !value || LLVMTypeOf(value) != *native) {
      return call_fail_backend(
          body,
          "LLVM could not extract one exact Callable result carrier."_view);
    }

    body.mark_owned(*type, value);
    values.insert(value);
  }

  return body.publish_values(result, values.get_view());
}

auto Tetrodotoxin::Library::Llvm::Builder::fit_input(
    const Ttx::Model::Addressable& parameter,
    const Ttx::Model::Pack& source,
    Count offset,
    Count size) const -> Core::Option<LLVMValueRef> {
  auto carriers = call_select_carriers(body);
  return carriers ? call_assemble_input(
                        body, *carriers, parameter, source, offset, size)
                  : Core::Option<LLVMValueRef>();
}

auto Tetrodotoxin::Library::Llvm::Builder::invoke(
    const Ttx::Model::Pack& result,
    const Ttx::Model::Callable& callable,
    Core::View::Vector<LLVMValueRef> inputs,
    Core::Option<const Ttx::Model::Pack&> receiver_source) const -> Bool {
  Llvm::Body& native_body = body;
  auto carriers = call_select_carriers(body);
  auto functions = call_select_functions(body);
  if (!carriers || !functions) {
    return False;
  }

  auto function = functions->find_function(callable);
  Core::View::Vector<Bool> indirect =
      functions->get_indirect_parameters(callable);
  const Ttx::Concept::Layout& parameters = callable.get_parameters();
  if (!function || inputs.get_size() != parameters.get_size() ||
      indirect.get_size() != inputs.get_size()) {
    return call_fail_backend(
        body,
        "LLVM cannot invoke a Callable before its complete physical signature."_view);
  }

  for (Count index = 0; index < inputs.get_size(); index++) {
    auto parameter = parameters.get_abstract(index);
    auto addressable = parameter
                           ? parameter->select<Ttx::Model::Addressable>()
                           : Core::Option<const Ttx::Model::Addressable&>();
    auto native = addressable ? carriers->get_type(addressable->get_type())
                              : Core::Option<LLVMTypeRef>();
    if (!addressable || !native ||
        LLVMTypeOf(inputs.get_data()[index]) != *native) {
      return call_fail_backend(
          body,
          "LLVM received a Call input with the wrong parameter carrier."_view);
    }
  }

  Tetrodotoxin::Library::Llvm::Body::NativeValues native_arguments(
      inputs.get_size() + 1);
  auto sret_type = functions->find_sret_type(callable);
  Core::Option<LLVMValueRef> returned_storage;
  Core::Option<const Ttx::Model::Type&> temporary_self_type;
  Core::Option<LLVMValueRef> temporary_self_address;
  if (sret_type) {
    returned_storage =
        native_body.create_entry_alloca(*sret_type, "call.result"_view);
    if (!returned_storage) {
      return call_fail_backend(
          body,
          "LLVM could not reserve the indirect Callable result storage."_view);
    }

    native_arguments.insert(*returned_storage);
  }

  for (Count index = 0; index < inputs.get_size(); index++) {
    LLVMValueRef argument = inputs.get_data()[index];
    if (indirect[index]) {
      auto parameter = parameters.get_abstract(index);
      auto addressable = parameter
                             ? parameter->select<Ttx::Model::Addressable>()
                             : Core::Option<const Ttx::Model::Addressable&>();
      if (!addressable) {
        return False;
      }

      const Ttx::Model::Type& type = addressable->get_type();
      auto native = carriers->get_type(type);
      if (!native) {
        return call_fail_backend(
            body,
            "LLVM cannot pass a parameter indirectly without its carrier."_view);
      }

      Bool self_reference =
          Bool(index == 0 && addressable->get_name() == "self"_view);
      Core::Option<LLVMValueRef> selected_address;
      if (self_reference && receiver_source) {
        auto target = native_body.find_target_address(*receiver_source);
        if (target && &target->get_type() == &type) {
          selected_address = target->get_address();
        }
      }

      LLVMValueRef address =
          selected_address
              ? *selected_address
              : native_body.create_entry_alloca(*native, "call.argument"_view);
      if (!address) {
        return call_fail_backend(
            body,
            "LLVM could not reserve one indirect Callable argument."_view);
      }

      if (!selected_address) {
        if (self_reference && carriers->owns_resources(type) &&
            !native_body.take_owned(argument) &&
            !carriers->retain(native_body, type, argument)) {
          return False;
        }

        LLVMValueRef stored =
            LLVMBuildStore(native_body.get_builder(), argument, address);
        if (!stored) {
          return call_fail_backend(
              body,
              "LLVM could not materialize one indirect Callable argument."_view);
        }

        if (self_reference) {
          temporary_self_type = type;
          temporary_self_address = address;
        }
      } else if (
          self_reference && carriers->owns_resources(type) &&
          native_body.take_owned(argument) &&
          !carriers->release(native_body, type, argument)) {
        return False;
      }

      argument = address;
    }

    native_arguments.insert(argument);
  }

  LLVMTypeRef signature = LLVMGlobalGetValueType(*function);
  LLVMTypeRef native_result = LLVMGetReturnType(signature);
  Bool returns_void = Bool(LLVMGetTypeKind(native_result) == LLVMVoidTypeKind);

  LLVMValueRef invoked = LLVMBuildCall2(
      native_body.get_builder(), signature, *function,
      native_arguments.get_data(), U32(native_arguments.get_size()),
      returns_void ? "" : "call");
  if (!invoked) {
    return call_fail_backend(
        body, "LLVM could not emit the selected Call."_view);
  }

  const Ttx::Concept::Layout& results = callable.get_results();
  auto library_callable = callable.select<Language::Model::Callable>();
  auto self_result = library_callable
                         ? library_callable->get_self_result()
                         : Core::Option<const Ttx::Model::Addressable&>();
  if (temporary_self_type && temporary_self_address) {
    if (self_result) {
      if (!native_body.register_storage(
              *temporary_self_type, *temporary_self_address)) {
        return call_fail_backend(
            body, "LLVM could not retain one returned Self reference."_view);
      }
    } else {
      auto native = carriers->get_type(*temporary_self_type);
      LLVMValueRef value = native
                               ? LLVMBuildLoad2(
                                     native_body.get_builder(), *native,
                                     *temporary_self_address, "self.temporary")
                               : nullptr;
      if (!value ||
          !carriers->release(native_body, *temporary_self_type, value)) {
        return call_fail_backend(
            body, "LLVM could not release one temporary Self receiver."_view);
      }
    }
  }

  if (self_result) {
    if (returns_void || returned_storage ||
        LLVMGetTypeKind(LLVMTypeOf(invoked)) != LLVMPointerTypeKind ||
        !native_body.publish_target_address(
            result, self_result->get_type(), invoked)) {
      return call_fail_backend(
          body, "LLVM could not publish one returned Self reference."_view);
    }
    return load(result);
  }

  // Callable parameters borrow their inputs. Body keeps an owned temporary
  // through the complete Statement so a returned View can still borrow that
  // storage while the enclosing expression consumes it. Receiving storage or
  // a Return explicitly takes ownership before Statement cleanup.

  if (result.get_layout().get_size() != results.get_size()) {
    return call_fail_backend(
        body,
        "LLVM Call Pack does not expose its completed Callable result count."_view);
  }

  if (results.is_empty()) {
    if (!returns_void || returned_storage) {
      return call_fail_backend(
          body,
          "LLVM retained a native value for an empty Callable result Layout."_view);
    }

    return call_publish_empty(native_body, result);
  }

  Core::Option<LLVMOpaqueValue&> returned;
  if (returned_storage && sret_type) {
    LLVMValueRef loaded = LLVMBuildLoad2(
        native_body.get_builder(), *sret_type, *returned_storage,
        "call.result");
    if (loaded) {
      returned = *loaded;
    }
  } else if (!returns_void) {
    returned = *invoked;
  }

  return call_publish_results(
      native_body, *carriers, result, results, returned);
}

auto Tetrodotoxin::Library::Llvm::Builder::get_size(
    const Ttx::Model::Pack& result,
    const Ttx::Model::Type& result_type,
    const Ttx::Model::Type& receiver_type,
    LLVMValueRef receiver) const -> Bool {
  Llvm::Body& native_body = body;
  auto carriers = call_select_carriers(body);
  if (!carriers) {
    return False;
  }

  auto native = carriers->get_type(result_type);
  if (!native || result.get_layout().get_size() != 1 ||
      LLVMGetTypeKind(LLVMTypeOf(receiver)) != LLVMStructTypeKind ||
      LLVMCountStructElementTypes(LLVMTypeOf(receiver)) < 2) {
    return call_fail_backend(
        body,
        "LLVM cannot read size from the completed contiguous receiver carrier."_view);
  }

  LLVMValueRef size =
      LLVMBuildExtractValue(native_body.get_builder(), receiver, 1, "size");
  if (!size || LLVMTypeOf(size) != *native ||
      !call_release_owned(native_body, *carriers, receiver_type, receiver)) {
    return False;
  }

  Core::Static::Vector<LLVMValueRef, 1> values = {{size}};
  native_body.mark_owned(result_type, size);
  return native_body.publish_values(result, values.get_view());
}

auto Tetrodotoxin::Library::Llvm::Builder::contiguous_is_empty(
    const Ttx::Model::Pack& result,
    const Ttx::Model::Type& result_type,
    const Ttx::Model::Type& receiver_type,
    LLVMValueRef receiver) const -> Bool {
  Llvm::Body& native_body = body;
  auto carriers = call_select_carriers(body);
  auto native =
      carriers ? carriers->get_type(result_type) : Core::Option<LLVMTypeRef>();
  if (!carriers || !native || result.get_layout().get_size() != 1 ||
      LLVMGetTypeKind(LLVMTypeOf(receiver)) != LLVMStructTypeKind ||
      LLVMCountStructElementTypes(LLVMTypeOf(receiver)) < 2) {
    return call_fail_backend(
        body,
        "LLVM cannot test the completed contiguous receiver carrier for emptiness."_view);
  }

  LLVMValueRef size =
      LLVMBuildExtractValue(native_body.get_builder(), receiver, 1, "size");
  LLVMValueRef zero = size ? LLVMConstNull(LLVMTypeOf(size)) : nullptr;
  LLVMValueRef empty =
      zero ? LLVMBuildICmp(
                 native_body.get_builder(), LLVMIntEQ, size, zero, "empty")
           : nullptr;
  if (!empty || LLVMTypeOf(empty) != *native ||
      !call_release_owned(native_body, *carriers, receiver_type, receiver)) {
    return False;
  }

  Core::Static::Vector<LLVMValueRef, 1> values = {{empty}};
  native_body.mark_owned(result_type, empty);
  return native_body.publish_values(result, values.get_view());
}

static auto object_element_size(
    Llvm::Body& body,
    const Llvm::Carriers& carriers,
    const Ttx::Model::Type& receiver_type) -> Core::Option<Count> {
  auto element = carriers.get_element(receiver_type);
  auto native =
      element ? carriers.get_type(*element) : Core::Option<LLVMTypeRef>();
  if (!element || !native) {
    return {};
  }

  auto& module = *llvm::unwrap(&body.get_program().get_module());
  return module.getDataLayout()
      .getTypeAllocSize(llvm::unwrap(*native))
      .getFixedValue();
}

static auto object_capacity_value(
    Llvm::Body& body,
    const Llvm::Carriers& carriers,
    const Ttx::Model::Type& receiver_type,
    LLVMValueRef receiver) -> Core::Option<LLVMValueRef> {
  auto element_size = object_element_size(body, carriers, receiver_type);
  if (!element_size || *element_size == 0) {
    return {};
  }

  auto& module = *llvm::unwrap(&body.get_program().get_module());
  llvm::LLVMContext& context = module.getContext();
  llvm::Type& pointer = *llvm::PointerType::getUnqual(context);
  llvm::Type& count = *llvm::Type::getInt64Ty(context);
  llvm::FunctionType& signature =
      *llvm::FunctionType::get(&count, {&pointer}, false);
  llvm::IRBuilder<>& builder =
      *reinterpret_cast<llvm::IRBuilder<>*>(body.get_builder());
  llvm::StringRef symbol(
      reinterpret_cast<const char*>(
          Abi::Core::object_capacity_symbol.get_data()),
      Abi::Core::object_capacity_symbol.get_size());
  llvm::Value& bytes = *builder.CreateCall(
      module.getOrInsertFunction(symbol, &signature), {llvm::unwrap(receiver)},
      "object.bytes");
  llvm::Value& divisor = *llvm::ConstantInt::get(&count, *element_size);
  return llvm::wrap(builder.CreateUDiv(&bytes, &divisor, "object.capacity"));
}

auto Tetrodotoxin::Library::Llvm::Builder::object_capacity(
    const Ttx::Model::Pack& result,
    const Ttx::Model::Type& result_type,
    const Ttx::Model::Type& receiver_type,
    LLVMValueRef receiver) const -> Bool {
  auto carriers = call_select_carriers(body);
  auto native_result =
      carriers ? carriers->get_type(result_type) : Core::Option<LLVMTypeRef>();
  auto capacity =
      carriers ? object_capacity_value(body, *carriers, receiver_type, receiver)
               : Core::Option<LLVMValueRef>();
  if (!carriers || !native_result || !capacity ||
      LLVMTypeOf(*capacity) != *native_result ||
      !call_release_owned(body, *carriers, receiver_type, receiver)) {
    return call_fail_backend(
        body, "LLVM cannot read capacity from Object[T]."_view);
  }

  Core::Static::Vector<LLVMValueRef, 1> values = {{*capacity}};
  body.mark_owned(result_type, *capacity);
  return body.publish_values(result, values.get_view());
}

auto Tetrodotoxin::Library::Llvm::Builder::object_is_shared(
    const Ttx::Model::Pack& result,
    const Ttx::Model::Type& result_type,
    const Ttx::Model::Type& receiver_type,
    const Ttx::Model::Pack& receiver_source,
    LLVMValueRef receiver) const -> Bool {
  auto carriers = call_select_carriers(body);
  auto native_result =
      carriers ? carriers->get_type(result_type) : Core::Option<LLVMTypeRef>();
  auto native_receiver = carriers ? carriers->get_type(receiver_type)
                                  : Core::Option<LLVMTypeRef>();
  if (!carriers || !native_result || result.get_layout().get_size() != 1 ||
      !native_receiver || LLVMTypeOf(receiver) != *native_receiver) {
    return call_fail_backend(
        body, "LLVM cannot query Object[T] sharing state."_view);
  }

  auto& module = *llvm::unwrap(&body.get_program().get_module());
  llvm::LLVMContext& context = module.getContext();
  llvm::Type& pointer = *llvm::PointerType::getUnqual(context);
  llvm::Type& count = *llvm::Type::getInt64Ty(context);
  llvm::FunctionType& signature =
      *llvm::FunctionType::get(&count, {&pointer}, false);
  llvm::IRBuilder<>& builder =
      *reinterpret_cast<llvm::IRBuilder<>*>(body.get_builder());
  llvm::StringRef symbol(
      reinterpret_cast<const char*>(
          Abi::Core::object_reservations_symbol.get_data()),
      Abi::Core::object_reservations_symbol.get_size());
  llvm::Value& reservations = *builder.CreateCall(
      module.getOrInsertFunction(symbol, &signature), {llvm::unwrap(receiver)},
      "object.reservations");

  // Loading a stored Object creates one owned evaluation value in addition to
  // its storage owner. A computed Object has only that evaluation owner. The
  // query excludes exactly those local reservations from its public result.
  U64 local_reservations = body.find_target_address(receiver_source) ? 2 : 1;
  llvm::Value& local = *llvm::ConstantInt::get(&count, local_reservations);
  LLVMValueRef shared =
      llvm::wrap(builder.CreateICmpUGT(&reservations, &local, "object.shared"));
  if (!shared || LLVMTypeOf(shared) != *native_result ||
      !call_release_owned(body, *carriers, receiver_type, receiver)) {
    return False;
  }

  Core::Static::Vector<LLVMValueRef, 1> values = {{shared}};
  body.mark_owned(result_type, shared);
  return body.publish_values(result, values.get_view());
}

auto Tetrodotoxin::Library::Llvm::Builder::object_clone(
    const Ttx::Model::Pack& result,
    const Ttx::Model::Type& receiver_type,
    const Ttx::Model::Pack& receiver_source,
    LLVMValueRef receiver) const -> Bool {
  auto carriers = call_select_carriers(body);
  auto target = body.find_target_address(receiver_source);
  auto element_size = carriers
                          ? object_element_size(body, *carriers, receiver_type)
                          : Core::Option<Count>();
  auto descriptor = carriers ? carriers->get_object_descriptor(
                                   body.get_program(), receiver_type)
                             : Core::Option<LLVMValueRef>();
  if (!carriers || !target || &target->get_type() != &receiver_type ||
      !element_size || !descriptor || !result.get_layout().is_empty()) {
    return call_fail_backend(
        body, "LLVM cannot clone the selected Object[T] buffer."_view);
  }

  if (body.take_owned(receiver) &&
      !carriers->release(body, receiver_type, receiver)) {
    return False;
  }

  auto& module = *llvm::unwrap(&body.get_program().get_module());
  llvm::LLVMContext& context = module.getContext();
  llvm::Type& pointer = *llvm::PointerType::getUnqual(context);
  llvm::Type& count = *llvm::Type::getInt64Ty(context);
  llvm::FunctionType& signature =
      *llvm::FunctionType::get(&pointer, {&pointer, &pointer, &count}, false);
  llvm::IRBuilder<>& builder =
      *reinterpret_cast<llvm::IRBuilder<>*>(body.get_builder());
  llvm::StringRef symbol(
      reinterpret_cast<const char*>(Abi::Core::object_clone_symbol.get_data()),
      Abi::Core::object_clone_symbol.get_size());
  llvm::Value& size = *llvm::ConstantInt::get(&count, *element_size);
  llvm::Value& cloned = *builder.CreateCall(
      module.getOrInsertFunction(symbol, &signature),
      {llvm::unwrap(receiver), llvm::unwrap(*descriptor), &size},
      "object.clone");
  builder.CreateStore(&cloned, llvm::unwrap(target->get_address()));
  return call_publish_empty(body, result);
}

auto Tetrodotoxin::Library::Llvm::Builder::object_view(
    const Ttx::Model::Pack& result,
    const Ttx::Model::Type& result_type,
    const Ttx::Model::Type& receiver_type,
    LLVMValueRef receiver) const -> Bool {
  auto carriers = call_select_carriers(body);
  auto native_result =
      carriers ? carriers->get_type(result_type) : Core::Option<LLVMTypeRef>();
  auto capacity =
      carriers ? object_capacity_value(body, *carriers, receiver_type, receiver)
               : Core::Option<LLVMValueRef>();
  if (!native_result || !capacity ||
      LLVMGetTypeKind(*native_result) != LLVMStructTypeKind ||
      LLVMCountStructElementTypes(*native_result) != 2) {
    return call_fail_backend(
        body, "LLVM cannot borrow Object[T] storage."_view);
  }

  LLVMValueRef view = LLVMGetUndef(*native_result);
  view = LLVMBuildInsertValue(
      body.get_builder(), view, receiver, 0, "object.data");
  view = LLVMBuildInsertValue(
      body.get_builder(), view, *capacity, 1, "object.size");
  Core::Static::Vector<LLVMValueRef, 1> values = {{view}};
  return view && body.publish_values(result, values.get_view());
}

static auto reserve_object(
    Llvm::Body& body,
    const Llvm::Carriers& carriers,
    const Ttx::Model::Type& receiver_type,
    const Ttx::Model::Pack& receiver_source,
    LLVMValueRef receiver,
    LLVMValueRef count,
    const Ttx::Model::Pack& element_default) -> Core::Option<LLVMValueRef> {
  auto target = body.find_target_address(receiver_source);
  auto element = carriers.get_element(receiver_type);
  auto native_element =
      element ? carriers.get_type(*element) : Core::Option<LLVMTypeRef>();
  auto element_size = object_element_size(body, carriers, receiver_type);
  auto defaults = body.find_values(element_default);
  auto default_value =
      element && defaults
          ? carriers.fit_and_assemble(
                body, *element, element_default, defaults->get_view())
          : Core::Option<LLVMValueRef>();
  auto descriptor = body.get_program().get_carriers().get_object_descriptor(
      body.get_program(), receiver_type);
  if (!target || &target->get_type() != &receiver_type || !element ||
      !native_element || !element_size || !default_value || !descriptor) {
    return {};
  }

  LLVMValueRef default_address =
      body.create_entry_alloca(*native_element, "object.default"_view);
  LLVMBuildStore(body.get_builder(), *default_value, default_address);

  if (body.take_owned(receiver) &&
      !carriers.release(body, receiver_type, receiver)) {
    return {};
  }

  auto& module = *llvm::unwrap(&body.get_program().get_module());
  llvm::LLVMContext& context = module.getContext();
  llvm::Type& pointer = *llvm::PointerType::getUnqual(context);
  llvm::Type& native_count = *llvm::Type::getInt64Ty(context);
  llvm::FunctionType& signature = *llvm::FunctionType::get(
      &pointer, {&pointer, &pointer, &native_count, &native_count, &pointer},
      false);
  llvm::IRBuilder<>& builder =
      *reinterpret_cast<llvm::IRBuilder<>*>(body.get_builder());
  llvm::StringRef symbol(
      reinterpret_cast<const char*>(
          Abi::Core::object_reserve_symbol.get_data()),
      Abi::Core::object_reserve_symbol.get_size());
  llvm::Value& size = *llvm::ConstantInt::get(&native_count, *element_size);
  llvm::Value& selected = *builder.CreateCall(
      module.getOrInsertFunction(symbol, &signature),
      {llvm::unwrap(receiver), llvm::unwrap(*descriptor), llvm::unwrap(count),
       &size, llvm::unwrap(default_address)},
      "object.reserve");
  builder.CreateStore(&selected, llvm::unwrap(target->get_address()));
  return llvm::wrap(&selected);
}

static auto publish_object_access(
    Llvm::Body& body,
    const Llvm::Carriers& carriers,
    const Ttx::Model::Pack& result,
    const Ttx::Model::Type& result_type,
    const Ttx::Model::Type& receiver_type,
    LLVMValueRef data) -> Bool {
  auto native_result = carriers.get_type(result_type);
  auto capacity = object_capacity_value(body, carriers, receiver_type, data);
  if (!native_result || !capacity ||
      LLVMGetTypeKind(*native_result) != LLVMStructTypeKind ||
      LLVMCountStructElementTypes(*native_result) != 2) {
    return False;
  }

  LLVMValueRef access = LLVMGetUndef(*native_result);
  access =
      LLVMBuildInsertValue(body.get_builder(), access, data, 0, "object.data");
  access = LLVMBuildInsertValue(
      body.get_builder(), access, *capacity, 1, "object.size");
  Core::Static::Vector<LLVMValueRef, 1> values = {{access}};
  return access && body.publish_values(result, values.get_view());
}

auto Tetrodotoxin::Library::Llvm::Builder::object_access(
    const Ttx::Model::Pack& result,
    const Ttx::Model::Type& result_type,
    const Ttx::Model::Type& receiver_type,
    const Ttx::Model::Pack& receiver_source,
    LLVMValueRef receiver,
    const Ttx::Model::Pack& element_default) const -> Bool {
  auto carriers = call_select_carriers(body);
  auto count =
      carriers ? object_capacity_value(body, *carriers, receiver_type, receiver)
               : Core::Option<LLVMValueRef>();
  auto selected = carriers && count
                      ? reserve_object(
                            body, *carriers, receiver_type, receiver_source,
                            receiver, *count, element_default)
                      : Core::Option<LLVMValueRef>();
  return carriers && selected &&
         publish_object_access(
             body, *carriers, result, result_type, receiver_type, *selected);
}

auto Tetrodotoxin::Library::Llvm::Builder::object_reserve(
    const Ttx::Model::Pack& result,
    const Ttx::Model::Type& result_type,
    const Ttx::Model::Type& receiver_type,
    const Ttx::Model::Pack& receiver_source,
    LLVMValueRef receiver,
    LLVMValueRef count,
    const Ttx::Model::Pack& element_default) const -> Bool {
  auto carriers = call_select_carriers(body);
  auto selected = carriers
                      ? reserve_object(
                            body, *carriers, receiver_type, receiver_source,
                            receiver, count, element_default)
                      : Core::Option<LLVMValueRef>();
  return carriers && selected &&
         publish_object_access(
             body, *carriers, result, result_type, receiver_type, *selected);
}

// A borrow needs storage that outlives receiver evaluation. An Addressable
// supplies that storage directly. A computed Fixed moves into one Body owned
// slot so the borrowed pointer remains valid through enclosing scope cleanup.
static auto create_fixed_borrow(
    Tetrodotoxin::Library::Llvm::Body& body,
    const Ttx::Model::Pack& result,
    const Ttx::Model::Type& result_type,
    const Ttx::Model::Type& receiver_type,
    const Ttx::Model::Pack& receiver_source,
    LLVMValueRef receiver) -> Bool {
  auto& native_body = body;
  auto carriers = call_select_carriers(body);
  if (!carriers) {
    return False;
  }

  auto storage = native_body.find_target_address(receiver_source);
  const Ttx::Model::Type& fixed_type = receiver_type;
  auto fixed_native = carriers->get_type(fixed_type);
  auto extent = carriers->get_extent(fixed_type);
  auto view_native = carriers->get_type(result_type);
  if ((storage && &storage->get_type() != &fixed_type) || !fixed_native ||
      !extent || !view_native || result.get_layout().get_size() != 1 ||
      LLVMGetTypeKind(*fixed_native) != LLVMArrayTypeKind ||
      LLVMGetArrayLength2(*fixed_native) != *extent ||
      LLVMGetTypeKind(*view_native) != LLVMStructTypeKind ||
      LLVMCountStructElementTypes(*view_native) != 2) {
    return call_fail_backend(
        body,
        "LLVM cannot borrow contiguous data from the exact Fixed storage carrier."_view);
  }

  Core::Option<LLVMValueRef> address;
  Bool releases_receiver = True;
  if (storage) {
    address = storage->get_address();
  } else {
    LLVMValueRef allocated =
        native_body.create_entry_alloca(*fixed_native, "fixed.borrow"_view);
    LLVMBuildStore(native_body.get_builder(), receiver, allocated);
    if (!native_body.acquire(fixed_type, receiver) ||
        !native_body.register_storage(fixed_type, allocated) ||
        !native_body.publish_target_address(
            receiver_source, fixed_type, allocated)) {
      return False;
    }

    address = allocated;
    releases_receiver = False;
  }

  LLVMContextRef context = LLVMGetTypeContext(*fixed_native);
  LLVMValueRef zero = LLVMConstInt(LLVMInt64TypeInContext(context), 0, 0);
  Core::Static::Vector<LLVMValueRef, 2> indices = {{zero, zero}};
  LLVMValueRef data = LLVMBuildInBoundsGEP2(
      native_body.get_builder(), *fixed_native, *address, indices.get_data(),
      U32(indices.get_size()), "fixed.data");
  if (!data) {
    return call_fail_backend(
        body, "LLVM could not select the first element of Fixed storage."_view);
  }

  LLVMValueRef count =
      LLVMConstInt(LLVMInt64TypeInContext(context), U64(*extent), 0);
  LLVMValueRef view = LLVMGetUndef(*view_native);
  view = LLVMBuildInsertValue(
      native_body.get_builder(), view, data, 0, "fixed.data");
  view = LLVMBuildInsertValue(
      native_body.get_builder(), view, count, 1, "fixed.size");
  if (!view) {
    return call_fail_backend(
        body,
        "LLVM could not construct a borrow over the selected Fixed storage."_view);
  }

  if (releases_receiver &&
      !call_release_owned(native_body, *carriers, fixed_type, receiver)) {
    return False;
  }

  Core::Static::Vector<LLVMValueRef, 1> values = {{view}};
  native_body.mark_owned(result_type, view);
  return native_body.publish_values(result, values.get_view());
}

auto Tetrodotoxin::Library::Llvm::Builder::borrow_fixed(
    const Ttx::Model::Pack& result,
    const Ttx::Model::Type& result_type,
    const Ttx::Model::Type& receiver_type,
    const Ttx::Model::Pack& receiver_source,
    LLVMValueRef receiver) const -> Bool {
  return create_fixed_borrow(
      body, result, result_type, receiver_type, receiver_source, receiver);
}

auto Tetrodotoxin::Library::Llvm::Builder::slice_view(
    const Ttx::Model::Pack& result,
    const Ttx::Model::Type& result_type,
    const Ttx::Model::Type& receiver_type,
    LLVMValueRef receiver,
    LLVMValueRef start,
    LLVMValueRef count) const -> Bool {
  Llvm::Body& native_body = body;
  auto carriers = call_select_carriers(body);
  if (!carriers) {
    return False;
  }

  auto element = carriers->get_element(receiver_type);
  auto native_element =
      element ? carriers->get_type(*element) : Core::Option<LLVMTypeRef>();
  auto native_result = carriers->get_type(result_type);
  Count receiver_fields =
      LLVMGetTypeKind(LLVMTypeOf(receiver)) == LLVMStructTypeKind
          ? LLVMCountStructElementTypes(LLVMTypeOf(receiver))
          : 0;
  if (!element || !native_element || !native_result ||
      result.get_layout().get_size() != 1 || receiver_fields != 2 ||
      LLVMGetTypeKind(*native_result) != LLVMStructTypeKind ||
      LLVMCountStructElementTypes(*native_result) != 2) {
    return call_fail_backend(
        body,
        "LLVM cannot slice the completed contiguous receiver carrier."_view);
  }

  LLVMBuilderRef builder = native_body.get_builder();
  LLVMValueRef data = LLVMBuildExtractValue(builder, receiver, 0, "slice.data");
  LLVMValueRef length =
      LLVMBuildExtractValue(builder, receiver, 1, "slice.length");
  if (!data || !length || LLVMTypeOf(start) != LLVMTypeOf(length) ||
      LLVMTypeOf(count) != LLVMTypeOf(length)) {
    return call_fail_backend(
        body, "LLVM cannot align slice indices with the receiver size."_view);
  }

  // Perimortem clipping keeps the available suffix and uses a canonical empty
  // View when the requested start is unavailable.
  LLVMValueRef present =
      LLVMBuildICmp(builder, LLVMIntULT, start, length, "slice.present");
  LLVMValueRef remaining =
      LLVMBuildSub(builder, length, start, "slice.remaining");
  LLVMValueRef short_request =
      LLVMBuildICmp(builder, LLVMIntULT, count, remaining, "slice.short");
  LLVMValueRef selected_count =
      LLVMBuildSelect(builder, short_request, count, remaining, "slice.count");
  LLVMValueRef zero = LLVMConstNull(LLVMTypeOf(length));
  LLVMValueRef size =
      LLVMBuildSelect(builder, present, selected_count, zero, "slice.size");
  LLVMValueRef offset = start;
  LLVMValueRef selected_data = LLVMBuildGEP2(
      builder, *native_element, data, &offset, 1, "slice.selected.data");
  LLVMValueRef empty_data = LLVMConstNull(LLVMTypeOf(data));
  LLVMValueRef view_data = LLVMBuildSelect(
      builder, present, selected_data, empty_data, "slice.view.data");

  LLVMValueRef view = LLVMGetUndef(*native_result);
  view = LLVMBuildInsertValue(builder, view, view_data, 0, "slice.view.data");
  view = LLVMBuildInsertValue(builder, view, size, 1, "slice.view.size");
  if (!view) {
    return False;
  }

  if (!call_release_owned(native_body, *carriers, receiver_type, receiver)) {
    return False;
  }

  Core::Static::Vector<LLVMValueRef, 1> values = {{view}};
  native_body.mark_owned(result_type, view);
  return native_body.publish_values(result, values.get_view());
}

// Comparison uses the operand Type supplied by the semantic owner to preserve
// signedness that cannot be recovered from an LLVM integer Type.

static auto comparison_find_scalar(
    const Tetrodotoxin::Library::Llvm::Body& body,
    const Ttx::Model::Pack& pack) -> Core::Option<LLVMValueRef> {
  auto values = body.find_values(pack);
  if (!values || values->get_size() != 1) {
    return {};
  }

  return values->get_data()[0];
}

static auto comparison_select_carriers(const Llvm::Body& body)
    -> Core::Option<const Tetrodotoxin::Library::Llvm::Carriers&> {
  return body.get_program().get_carriers();
}

static auto emit_comparison(
    Llvm::Body& body,
    const Ttx::Model::Type& carrier,
    const Ttx::Model::Pack& result,
    const Ttx::Model::Pack& left,
    const Ttx::Model::Pack& right,
    LLVMRealPredicate real,
    LLVMIntPredicate signed_integer,
    LLVMIntPredicate unsigned_integer) -> Bool {
  Llvm::Body& native_body = body;
  auto carriers = comparison_select_carriers(body);
  if (!carriers) {
    return False;
  }

  auto left_value = comparison_find_scalar(native_body, left);
  auto right_value = comparison_find_scalar(native_body, right);
  auto native = carriers->get_type(carrier);
  if (!left_value || !right_value || !native ||
      LLVMTypeOf(*left_value) != *native ||
      LLVMTypeOf(*right_value) != *native) {
    return False;
  }

  LLVMValueRef selected =
      carriers->is_real(carrier)
          ? LLVMBuildFCmp(
                native_body.get_builder(), real, *left_value, *right_value, "")
          : LLVMBuildICmp(
                native_body.get_builder(),
                carriers->is_signed(carrier) ? signed_integer
                                             : unsigned_integer,
                *left_value, *right_value, "");
  Core::Static::Vector<LLVMValueRef, 1> values = {{selected}};
  return native_body.publish_values(result, values.get_view());
}

auto Tetrodotoxin::Library::Llvm::Builder::compare(
    Comparison operation,
    const Ttx::Model::Type& carrier,
    const Ttx::Model::Pack& result,
    const Ttx::Model::Pack& left,
    const Ttx::Model::Pack& right) const -> Bool {
  switch (operation) {
  case Comparison::Equal:
    return emit_comparison(
        body, carrier, result, left, right, LLVMRealOEQ, LLVMIntEQ, LLVMIntEQ);

  case Comparison::NotEqual:
    return emit_comparison(
        body, carrier, result, left, right, LLVMRealUNE, LLVMIntNE, LLVMIntNE);

  case Comparison::Less:
    return emit_comparison(
        body, carrier, result, left, right, LLVMRealOLT, LLVMIntSLT,
        LLVMIntULT);

  case Comparison::LessEqual:
    return emit_comparison(
        body, carrier, result, left, right, LLVMRealOLE, LLVMIntSLE,
        LLVMIntULE);

  case Comparison::Greater:
    return emit_comparison(
        body, carrier, result, left, right, LLVMRealOGT, LLVMIntSGT,
        LLVMIntUGT);

  case Comparison::GreaterEqual:
    return emit_comparison(
        body, carrier, result, left, right, LLVMRealOGE, LLVMIntSGE,
        LLVMIntUGE);
  }
}

auto Tetrodotoxin::Library::Llvm::Builder::compare_bytes(
    Comparison operation,
    const Ttx::Model::Pack& result,
    const Ttx::Model::Pack& left,
    const Ttx::Model::Pack& right) const -> Bool {
  if (operation != Comparison::Equal && operation != Comparison::NotEqual) {
    return False;
  }

  auto left_value = comparison_find_scalar(body, left);
  auto right_value = comparison_find_scalar(body, right);
  if (!left_value || !right_value ||
      LLVMGetTypeKind(LLVMTypeOf(*left_value)) != LLVMStructTypeKind ||
      LLVMGetTypeKind(LLVMTypeOf(*right_value)) != LLVMStructTypeKind) {
    return False;
  }

  LLVMBuilderRef builder = body.get_builder();
  LLVMValueRef left_data = LLVMBuildExtractValue(builder, *left_value, 0, "");
  LLVMValueRef left_size = LLVMBuildExtractValue(builder, *left_value, 1, "");
  LLVMValueRef right_data = LLVMBuildExtractValue(builder, *right_value, 0, "");
  LLVMValueRef right_size = LLVMBuildExtractValue(builder, *right_value, 1, "");
  if (!left_data || !left_size || !right_data || !right_size) {
    return False;
  }

  LLVMValueRef function = body.get_function();
  LLVMContextRef context =
      LLVMGetModuleContext(&body.get_program().get_module());
  LLVMBasicBlockRef unequal = LLVMGetInsertBlock(builder);
  LLVMBasicBlockRef matching =
      LLVMAppendBasicBlockInContext(context, function, "bytes.matching");
  LLVMBasicBlockRef content =
      LLVMAppendBasicBlockInContext(context, function, "bytes.content");
  LLVMBasicBlockRef done =
      LLVMAppendBasicBlockInContext(context, function, "bytes.done");
  LLVMValueRef size_equal =
      LLVMBuildICmp(builder, LLVMIntEQ, left_size, right_size, "");
  LLVMBuildCondBr(builder, size_equal, matching, done);

  LLVMPositionBuilderAtEnd(builder, matching);
  LLVMValueRef zero_size = LLVMConstInt(LLVMTypeOf(left_size), 0, 0);
  LLVMValueRef empty =
      LLVMBuildICmp(builder, LLVMIntEQ, left_size, zero_size, "");
  LLVMBuildCondBr(builder, empty, done, content);

  LLVMPositionBuilderAtEnd(builder, content);
  LLVMModuleRef module = &body.get_program().get_module();
  LLVMValueRef compare_function = LLVMGetNamedFunction(module, "memcmp");
  LLVMTypeRef compare_parameters[] = {
    LLVMTypeOf(left_data), LLVMTypeOf(right_data), LLVMTypeOf(left_size)};
  LLVMTypeRef compare_type = LLVMFunctionType(
      LLVMInt32TypeInContext(context), compare_parameters, 3, 0);
  if (!compare_function) {
    compare_function = LLVMAddFunction(module, "memcmp", compare_type);
  }
  LLVMValueRef compare_arguments[] = {left_data, right_data, left_size};
  LLVMValueRef comparison = LLVMBuildCall2(
      builder, compare_type, compare_function, compare_arguments, 3, "");
  LLVMValueRef content_equal = LLVMBuildICmp(
      builder, LLVMIntEQ, comparison,
      LLVMConstInt(LLVMInt32TypeInContext(context), 0, 0), "");
  LLVMBuildBr(builder, done);

  LLVMPositionBuilderAtEnd(builder, done);
  LLVMValueRef equal =
      LLVMBuildPhi(builder, LLVMInt1TypeInContext(context), "");
  LLVMValueRef incoming_values[] = {
    LLVMConstInt(LLVMInt1TypeInContext(context), 0, 0),
    LLVMConstInt(LLVMInt1TypeInContext(context), 1, 0), content_equal};
  LLVMBasicBlockRef incoming_blocks[] = {unequal, matching, content};
  LLVMAddIncoming(equal, incoming_values, incoming_blocks, 3);
  LLVMValueRef selected =
      operation == Comparison::Equal ? equal : LLVMBuildNot(builder, equal, "");
  Core::Static::Vector<LLVMValueRef, 1> values = {{selected}};
  return body.publish_values(result, values.get_view());
}

// Construction assembles inline values or allocates Object payloads through
// the carrier selected by the exact source Type.

auto Tetrodotoxin::Library::Llvm::Builder::construct(
    const Ttx::Model::Pack& result,
    const Ttx::Model::Type& type,
    const Ttx::Model::Pack& values) const -> Bool {
  Llvm::Body& native_body = body;
  Llvm::Program& native_program = body.get_program();
  const Carriers& carriers = native_program.get_carriers();

  auto elements = native_body.find_values(values);
  if (!elements) {
    return native_program.fail_backend(
        "LLVM cannot construct a value before its completed initializer Pack is lowered."_view);
  }

  auto value = carriers.construct(native_body, type, elements->get_view());
  if (!value) {
    return False;
  }

  Core::Static::Vector<LLVMValueRef, 1> native = {{*value}};
  return native_body.publish_values(result, native.get_view());
}

auto Tetrodotoxin::Library::Llvm::Builder::construct_provider(
    const Ttx::Model::Pack& result,
    const Ttx::Model::Type& type,
    const Ttx::Model::Pack& arguments,
    Core::View::Vector<Ttx::Concept::Reference<const Ttx::Model::Addressable>>
        parameters) const -> Bool {
  auto& program = body.get_program();
  const auto& functions = program.get_functions();
  return functions.reserve_construction(program, type, False, parameters) &&
         functions.complete_construction(program, type) &&
         functions.call_construction(body, result, type, arguments);
}

// Control operations return transient block handles to their semantic owners.
// Body retains only loop targets needed by nested break and continue owners.

static auto control_select_carriers(const Llvm::Body& body)
    -> Core::Option<const Tetrodotoxin::Library::Llvm::Carriers&> {
  return body.get_program().get_carriers();
}

static auto control_native_builder(
    const Tetrodotoxin::Library::Llvm::Body& body) -> llvm::IRBuilder<>& {
  return *reinterpret_cast<llvm::IRBuilder<>*>(body.get_builder());
}

static auto control_native_function(
    const Tetrodotoxin::Library::Llvm::Body& body) -> llvm::Function& {
  return *llvm::unwrap<llvm::Function>(body.get_function());
}

static auto control_find_value(
    const Tetrodotoxin::Library::Llvm::Body& body,
    const Ttx::Model::Pack& pack) -> Core::Option<llvm::Value&> {
  auto found = body.find_value(pack);
  return found ? Core::Option<llvm::Value&>(*llvm::unwrap(*found))
               : Core::Option<llvm::Value&>();
}

static auto control_leave_loop(
    Llvm::Body& body,
    const Ttx::Concept::Abstract& target,
    Bool breaking) -> Bool {
  Llvm::Body& native_body = body;
  auto loop = native_body.find_loop(target);
  if (!loop || !native_body.emit_storage_cleanup(loop->get_storage_depth()) ||
      !native_body.emit_temporary_cleanup()) {
    return False;
  }

  LLVMBasicBlockRef destination =
      breaking ? loop->get_break_target() : loop->get_continue_target();
  control_native_builder(native_body).CreateBr(llvm::unwrap(destination));
  return True;
}

static auto control_return_values(
    Llvm::Body& native_body,
    const Ttx::Model::Pack& values,
    Bool preserve_tracking) -> Bool {
  auto carriers = control_select_carriers(native_body);
  if (!carriers) {
    return False;
  }

  auto callable = native_body.get_callable();
  auto returned = native_body.find_values(values);
  if (!callable || !returned) {
    return False;
  }

  const Ttx::Concept::Layout& results = callable->get_results();
  Core::Option<LLVMValueRef> native_return;
  if (results.is_empty()) {
    if (returned->get_size() != 0) {
      return False;
    }
  } else if (results.get_size() == 1) {
    auto entry = results.get_abstract(0);
    auto reference = entry ? entry->select<Ttx::Model::Addressable>()
                           : Core::Option<const Ttx::Model::Addressable&>();
    auto type =
        reference ? Core::Option<const Ttx::Model::Type&>(reference->get_type())
        : entry   ? entry->resolve().select<Ttx::Model::Type>()
                  : Core::Option<const Ttx::Model::Type&>();
    if (!type) {
      return False;
    }

    if (reference) {
      auto address = native_body.find_target_address(values);
      if (!address || &address->get_type() != &*type) {
        return False;
      }
      native_return = address->get_address();
    } else if (returned->get_size() == 0) {
      native_return = carriers->zero(native_body.get_program(), *type);
    } else {
      native_return = carriers->fit_and_assemble(
          native_body, *type, values, returned->get_view());
    }

    if (!native_return ||
        (!reference && !native_body.acquire(*type, *native_return))) {
      return False;
    }
  } else {
    if (returned->get_size() != results.get_size()) {
      return False;
    }

    auto fitted =
        carriers->fit(native_body, values, results, returned->get_view());
    if (!fitted) {
      return False;
    }

    Memory::Dynamic::Vector<LLVMValueRef> received(results.get_size());
    for (Count index = 0; index < results.get_size(); index++) {
      auto type =
          results.get_abstract(index)->resolve().select<Ttx::Model::Type>();
      if (!type) {
        return False;
      }

      Core::Static::Vector<LLVMValueRef, 1> source = {{
        fitted->get_data()[index],
      }};
      auto assembled =
          carriers->assemble(native_body, *type, source.get_view());
      if (!assembled || !native_body.acquire(*type, *assembled)) {
        return False;
      }

      received.insert(*assembled);
    }

    auto sret_type = native_body.get_sret_type();
    llvm::Type* aggregate_type =
        sret_type ? llvm::unwrap(*sret_type)
                  : control_native_function(native_body).getReturnType();
    llvm::Value* aggregate = llvm::UndefValue::get(aggregate_type);
    for (Count index = 0; index < received.get_size(); index++) {
      aggregate = control_native_builder(native_body)
                      .CreateInsertValue(
                          aggregate, llvm::unwrap(received[index]), U32(index));
    }

    native_return = llvm::wrap(aggregate);
  }

  if (!native_body.emit_storage_cleanup(0) ||
      !(preserve_tracking ? native_body.emit_temporary_cleanup()
                          : native_body.clear_temporary_cleanup())) {
    return False;
  }

  return native_body.create_return(native_return);
}

auto Tetrodotoxin::Library::Llvm::Builder::return_values(
    const Ttx::Model::Pack& values) const -> Bool {
  return control_return_values(body, values, False);
}

auto Tetrodotoxin::Library::Llvm::Builder::leave_loop(
    LoopAction action,
    const Ttx::Concept::Abstract& target) const -> Bool {
  return control_leave_loop(body, target, action == LoopAction::Break);
}

auto Tetrodotoxin::Library::Llvm::Builder::begin_branch(
    const Ttx::Model::Pack& condition) const -> Core::Option<Branch> {
  Llvm::Body& native_body = body;
  auto selected = control_find_value(native_body, condition);
  if (!selected) {
    return {};
  }

  llvm::IRBuilder<>& builder = control_native_builder(native_body);
  llvm::Function& function = control_native_function(native_body);
  llvm::BasicBlock& branch =
      *llvm::BasicBlock::Create(function.getContext(), "if.body", &function);
  llvm::BasicBlock& alternate = *llvm::BasicBlock::Create(
      function.getContext(), "if.alternate", &function);
  llvm::BasicBlock& done =
      *llvm::BasicBlock::Create(function.getContext(), "if.done", &function);
  if (!native_body.clear_temporary_cleanup()) {
    return {};
  }
  builder.CreateCondBr(&*selected, &branch, &alternate);
  builder.SetInsertPoint(&branch);
  return Branch(llvm::wrap(&alternate), llvm::wrap(&done));
}

auto Tetrodotoxin::Library::Llvm::Builder::begin_alternate(Branch& state) const
    -> Bool {
  Llvm::Body& native_body = body;
  if (state.has_alternate()) {
    return False;
  }

  llvm::IRBuilder<>& builder = control_native_builder(native_body);
  llvm::BasicBlock* current = builder.GetInsertBlock();
  Bool reaches_done = Bool(current && !current->getTerminator());
  if (reaches_done) {
    builder.CreateBr(llvm::unwrap(state.get_done()));
  }

  state.begin_alternate(reaches_done);
  builder.SetInsertPoint(llvm::unwrap(state.get_alternate()));
  return True;
}

auto Tetrodotoxin::Library::Llvm::Builder::end_branch(Branch state) const
    -> Bool {
  Llvm::Body& native_body = body;
  llvm::IRBuilder<>& builder = control_native_builder(native_body);
  llvm::BasicBlock* current = builder.GetInsertBlock();
  Bool current_reaches = Bool(current && !current->getTerminator());
  Bool body_reaches =
      state.has_alternate() ? state.body_reaches_done() : current_reaches;
  if (current_reaches) {
    builder.CreateBr(llvm::unwrap(state.get_done()));
  }

  Bool alternate_reaches = current_reaches;
  if (!state.has_alternate()) {
    builder.SetInsertPoint(llvm::unwrap(state.get_alternate()));
    builder.CreateBr(llvm::unwrap(state.get_done()));
    alternate_reaches = True;
  }

  builder.SetInsertPoint(llvm::unwrap(state.get_done()));
  if (!body_reaches && !alternate_reaches) {
    builder.CreateUnreachable();
  }

  return True;
}

auto Tetrodotoxin::Library::Llvm::Builder::begin_while(
    const Ttx::Concept::Abstract& owner) const -> Bool {
  Llvm::Body& native_body = body;
  llvm::IRBuilder<>& builder = control_native_builder(native_body);
  llvm::Function& function = control_native_function(native_body);
  llvm::BasicBlock& condition = *llvm::BasicBlock::Create(
      function.getContext(), "while.condition", &function);
  llvm::BasicBlock& done =
      *llvm::BasicBlock::Create(function.getContext(), "while.done", &function);
  builder.CreateBr(&condition);
  builder.SetInsertPoint(&condition);
  return native_body.publish_loop(
      owner, llvm::wrap(&done), llvm::wrap(&condition));
}

auto Tetrodotoxin::Library::Llvm::Builder::select_while(
    const Ttx::Concept::Abstract& owner,
    const Ttx::Model::Pack& condition) const -> Bool {
  Llvm::Body& native_body = body;
  auto loop = native_body.find_loop(owner);
  auto selected = control_find_value(native_body, condition);
  if (!loop || !selected) {
    return False;
  }

  llvm::Function& function = control_native_function(native_body);
  llvm::BasicBlock& branch =
      *llvm::BasicBlock::Create(function.getContext(), "while.body", &function);
  llvm::IRBuilder<>& builder = control_native_builder(native_body);
  if (!native_body.clear_temporary_cleanup()) {
    return False;
  }
  builder.CreateCondBr(
      &*selected, &branch, llvm::unwrap(loop->get_break_target()));
  builder.SetInsertPoint(&branch);
  return True;
}

auto Tetrodotoxin::Library::Llvm::Builder::end_while(
    const Ttx::Concept::Abstract& owner) const -> Bool {
  Llvm::Body& native_body = body;
  auto loop = native_body.find_loop(owner);
  if (!loop) {
    return False;
  }

  LLVMBasicBlockRef done = loop->get_break_target();
  LLVMBasicBlockRef condition = loop->get_continue_target();
  llvm::IRBuilder<>& builder = control_native_builder(native_body);
  llvm::BasicBlock* current = builder.GetInsertBlock();
  if (current && !current->getTerminator()) {
    builder.CreateBr(llvm::unwrap(condition));
  }

  if (!native_body.remove_loop(owner)) {
    return False;
  }

  builder.SetInsertPoint(llvm::unwrap(done));
  return True;
}

static auto create_bytes_view(
    Tetrodotoxin::Library::Llvm::Program& program,
    LLVMTypeRef type,
    Core::View::Bytes value) -> Core::Option<LLVMValueRef> {
  if (LLVMGetTypeKind(type) != LLVMStructTypeKind ||
      LLVMCountStructElementTypes(type) != 2) {
    return {};
  }

  LLVMContextRef context = &program.get_context();
  LLVMModuleRef module = &program.get_module();
  LLVMValueRef data = LLVMConstNull(LLVMPointerTypeInContext(context, 0));
  if (!value.is_empty()) {
    LLVMValueRef contents = LLVMConstStringInContext2(
        context, reinterpret_cast<const char*>(value.get_data()),
        value.get_size(), 1);
    LLVMValueRef global =
        LLVMAddGlobal(module, LLVMTypeOf(contents), "__ttx_bytes");
    LLVMSetGlobalConstant(global, 1);
    LLVMSetInitializer(global, contents);
    LLVMSetLinkage(global, LLVMPrivateLinkage);
    LLVMSetUnnamedAddress(global, LLVMGlobalUnnamedAddr);
    data = global;
  }

  LLVMValueRef count =
      LLVMConstInt(LLVMInt64TypeInContext(context), value.get_size(), 0);
  Core::Static::Vector<LLVMValueRef, 2> elements = {{data, count}};
  return LLVMConstNamedStruct(
      type, elements.get_data(), U32(elements.get_size()));
}

static auto select_enumeration_value(
    llvm::IRBuilder<>& builder,
    llvm::Value& index,
    llvm::IntegerType& type,
    Core::View::Vector<U64> values) -> llvm::Value& {
  llvm::Value* selected = llvm::ConstantInt::get(&type, 0);
  for (Count value_index = 0; value_index < values.get_size(); value_index++) {
    llvm::Value* matches = builder.CreateICmpEQ(
        &index, builder.getInt64(value_index), "enum.index");
    selected = builder.CreateSelect(
        matches, llvm::ConstantInt::get(&type, values[value_index]), selected,
        "enum.value");
  }

  return *selected;
}

static auto select_enumeration_name(
    Tetrodotoxin::Library::Llvm::Program& program,
    llvm::IRBuilder<>& builder,
    llvm::Value& index,
    llvm::StructType& type,
    Core::View::Vector<Core::View::Bytes> names) -> Core::Option<llvm::Value&> {
  auto empty = create_bytes_view(program, llvm::wrap(&type), {});
  BAIL_IF(!empty);

  llvm::Value* selected_data =
      builder.CreateExtractValue(llvm::unwrap(*empty), 0);
  llvm::Value* selected_size =
      builder.CreateExtractValue(llvm::unwrap(*empty), 1);
  for (Count name_index = 0; name_index < names.get_size(); name_index++) {
    auto candidate =
        create_bytes_view(program, llvm::wrap(&type), names[name_index]);
    BAIL_IF(!candidate);

    llvm::Value* matches = builder.CreateICmpEQ(
        &index, builder.getInt64(name_index), "enum.name.index");
    selected_data = builder.CreateSelect(
        matches, builder.CreateExtractValue(llvm::unwrap(*candidate), 0),
        selected_data, "enum.name.data");
    selected_size = builder.CreateSelect(
        matches, builder.CreateExtractValue(llvm::unwrap(*candidate), 1),
        selected_size, "enum.name.size");
  }

  llvm::Value* result = llvm::UndefValue::get(&type);
  result = builder.CreateInsertValue(result, selected_data, 0);
  result = builder.CreateInsertValue(result, selected_size, 1);
  return *result;
}

auto Tetrodotoxin::Library::Llvm::Builder::begin_sequence(
    const Ttx::Concept::Abstract& owner,
    const Ttx::Model::Addressable& binding,
    const Ttx::Model::Pack& input) const -> Bool {
  Llvm::Body& native_body = body;
  auto carriers = control_select_carriers(body);
  if (!carriers) {
    return False;
  }

  auto native_input = control_find_value(native_body, input);
  auto native_element = carriers->get_type(binding.get_type());
  if (!native_input || !native_element) {
    return False;
  }

  llvm::IRBuilder<>& builder = control_native_builder(native_body);
  Core::Option<llvm::Value&> start;
  Core::Option<llvm::Value&> end;
  Core::Option<llvm::Value&> data;
  Bool range = False;
  auto array = llvm::dyn_cast<llvm::ArrayType>(native_input->getType());
  auto structure = llvm::dyn_cast<llvm::StructType>(native_input->getType());
  if (array) {
    LLVMValueRef storage_handle =
        native_body.create_entry_alloca(llvm::wrap(array), "for.input"_view);
    llvm::Value* storage = llvm::unwrap(storage_handle);
    builder.CreateStore(&*native_input, storage);
    data = *builder.CreateInBoundsGEP(
        array, storage, {builder.getInt64(0), builder.getInt64(0)});
    start = *builder.getInt64(0);
    end = *builder.getInt64(array->getNumElements());
  } else if (
      structure && structure->getNumElements() == 2 &&
      structure->getElementType(0)->isPointerTy()) {
    data = *builder.CreateExtractValue(&*native_input, 0);
    start = *builder.getInt64(0);
    end = *builder.CreateExtractValue(&*native_input, 1);
  } else if (structure && structure->getNumElements() == 2) {
    start = *builder.CreateExtractValue(&*native_input, 0);
    end = *builder.CreateExtractValue(&*native_input, 1);
    range = True;
  } else {
    return False;
  }

  if (!start || !end || (!range && !data)) {
    return False;
  }

  LLVMValueRef binding_handle =
      native_body.create_entry_alloca(*native_element, "for.entry"_view);
  LLVMValueRef index_handle = native_body.create_entry_alloca(
      llvm::wrap(start->getType()), "for.index"_view);
  llvm::Value* binding_address = llvm::unwrap(binding_handle);
  llvm::Value* index_address = llvm::unwrap(index_handle);
  builder.CreateStore(&*start, index_address);
  if (!native_body.publish_address(binding, binding_handle)) {
    return False;
  }

  llvm::Function& function = control_native_function(native_body);
  llvm::BasicBlock& condition = *llvm::BasicBlock::Create(
      function.getContext(), "for.condition", &function);
  llvm::BasicBlock& branch =
      *llvm::BasicBlock::Create(function.getContext(), "for.body", &function);
  llvm::BasicBlock& step =
      *llvm::BasicBlock::Create(function.getContext(), "for.step", &function);
  llvm::BasicBlock& done =
      *llvm::BasicBlock::Create(function.getContext(), "for.done", &function);
  builder.CreateBr(&condition);

  builder.SetInsertPoint(&condition);
  llvm::Value* current = builder.CreateLoad(start->getType(), index_address);
  llvm::Value* active = range && carriers->is_signed(binding.get_type())
                            ? builder.CreateICmpSLT(current, &*end)
                            : builder.CreateICmpULT(current, &*end);
  builder.CreateCondBr(active, &branch, &done);

  builder.SetInsertPoint(&branch);
  if (range) {
    builder.CreateStore(current, binding_address);
  } else {
    llvm::Value* address =
        builder.CreateGEP(llvm::unwrap(*native_element), &*data, current);
    llvm::Value* selected =
        builder.CreateLoad(llvm::unwrap(*native_element), address);
    builder.CreateStore(selected, binding_address);
  }

  llvm::IRBuilder<> step_builder(&step);
  llvm::Value* stepped =
      step_builder.CreateLoad(start->getType(), index_address);
  step_builder.CreateStore(
      step_builder.CreateAdd(
          stepped, llvm::ConstantInt::get(start->getType(), 1)),
      index_address);
  step_builder.CreateBr(&condition);
  return native_body.publish_loop(owner, llvm::wrap(&done), llvm::wrap(&step));
}

auto Tetrodotoxin::Library::Llvm::Builder::begin_enumeration(
    const Ttx::Concept::Abstract& owner,
    const Ttx::Concept::Layout& bindings,
    Core::View::Vector<U64> values,
    Core::View::Vector<Core::View::Bytes> names) const -> Bool {
  Llvm::Body& native_body = body;
  auto carriers = control_select_carriers(body);
  if (!carriers || (bindings.get_size() != 1 && bindings.get_size() != 2) ||
      (!names.is_empty() && names.get_size() != values.get_size())) {
    return False;
  }

  auto value_entry = bindings.get_abstract(0);
  auto value_binding = value_entry
                           ? value_entry->select<Ttx::Model::Addressable>()
                           : Core::Option<const Ttx::Model::Addressable&>();
  auto value_type = value_binding
                        ? carriers->get_type(value_binding->get_type())
                        : Core::Option<LLVMTypeRef>();
  Core::Option<llvm::IntegerType&> native_value;
  if (value_type) {
    auto selected =
        llvm::dyn_cast<llvm::IntegerType>(llvm::unwrap(*value_type));
    if (selected) {
      native_value = *selected;
    }
  }

  if (!value_binding || !native_value) {
    return False;
  }

  Core::Option<const Ttx::Model::Addressable&> name_binding;
  Core::Option<llvm::StructType&> name_type;
  if (bindings.get_size() == 2) {
    auto name_entry = bindings.get_abstract(1);
    name_binding = name_entry ? name_entry->select<Ttx::Model::Addressable>()
                              : Core::Option<const Ttx::Model::Addressable&>();
    auto native_name = name_binding
                           ? carriers->get_type(name_binding->get_type())
                           : Core::Option<LLVMTypeRef>();
    Core::Option<llvm::StructType&> selected_name;
    if (native_name) {
      auto selected =
          llvm::dyn_cast<llvm::StructType>(llvm::unwrap(*native_name));
      if (selected) {
        selected_name = *selected;
      }
    }

    if (!name_binding || !selected_name ||
        names.is_empty() != values.is_empty()) {
      return False;
    }

    name_type = *selected_name;
  }

  llvm::IRBuilder<>& builder = control_native_builder(native_body);
  LLVMValueRef index_handle = native_body.create_entry_alloca(
      llvm::wrap(builder.getInt64Ty()), "for.index"_view);
  LLVMValueRef value_handle =
      native_body.create_entry_alloca(*value_type, "for.value"_view);
  llvm::Value* index_address = llvm::unwrap(index_handle);
  llvm::Value* value_address = llvm::unwrap(value_handle);
  builder.CreateStore(builder.getInt64(0), index_address);
  if (!native_body.publish_address(*value_binding, value_handle)) {
    return False;
  }

  Core::Option<LLVMValueRef> name_handle;
  if (name_binding && name_type) {
    name_handle = native_body.create_entry_alloca(
        llvm::wrap(&*name_type), "for.name"_view);
    if (!native_body.publish_address(*name_binding, *name_handle)) {
      return False;
    }
  }

  llvm::Function& function = control_native_function(native_body);
  llvm::BasicBlock& condition = *llvm::BasicBlock::Create(
      function.getContext(), "for.condition", &function);
  llvm::BasicBlock& branch =
      *llvm::BasicBlock::Create(function.getContext(), "for.body", &function);
  llvm::BasicBlock& step =
      *llvm::BasicBlock::Create(function.getContext(), "for.step", &function);
  llvm::BasicBlock& done =
      *llvm::BasicBlock::Create(function.getContext(), "for.done", &function);
  builder.CreateBr(&condition);

  builder.SetInsertPoint(&condition);
  llvm::Value* current =
      builder.CreateLoad(builder.getInt64Ty(), index_address);
  llvm::Value* active =
      builder.CreateICmpULT(current, builder.getInt64(values.get_size()));
  builder.CreateCondBr(active, &branch, &done);

  builder.SetInsertPoint(&branch);
  llvm::Value& selected =
      select_enumeration_value(builder, *current, *native_value, values);
  builder.CreateStore(&selected, value_address);
  if (name_handle && name_type) {
    auto selected_name = select_enumeration_name(
        native_body.get_program(), builder, *current, *name_type, names);
    if (!selected_name) {
      return False;
    }

    builder.CreateStore(&*selected_name, llvm::unwrap(*name_handle));
  }

  llvm::IRBuilder<> step_builder(&step);
  llvm::Value* stepped =
      step_builder.CreateLoad(builder.getInt64Ty(), index_address);
  step_builder.CreateStore(
      step_builder.CreateAdd(stepped, step_builder.getInt64(1)), index_address);
  step_builder.CreateBr(&condition);
  return native_body.publish_loop(owner, llvm::wrap(&done), llvm::wrap(&step));
}

auto Tetrodotoxin::Library::Llvm::Builder::end_iteration(
    const Ttx::Concept::Abstract& owner) const -> Bool {
  Llvm::Body& native_body = body;
  auto loop = native_body.find_loop(owner);
  if (!loop) {
    return False;
  }

  LLVMBasicBlockRef done = loop->get_break_target();
  LLVMBasicBlockRef step = loop->get_continue_target();
  llvm::IRBuilder<>& builder = control_native_builder(native_body);
  llvm::BasicBlock* current = builder.GetInsertBlock();
  if (current && !current->getTerminator()) {
    builder.CreateBr(llvm::unwrap(step));
  }

  if (!native_body.remove_loop(owner)) {
    return False;
  }

  builder.SetInsertPoint(llvm::unwrap(done));
  return True;
}

auto Tetrodotoxin::Library::Llvm::Builder::begin_match(
    const Ttx::Model::Pack& input) const -> Core::Option<Match> {
  Llvm::Body& native_body = body;
  auto native_input = native_body.find_value(input);
  if (!native_input) {
    return {};
  }

  llvm::Function& function = control_native_function(native_body);
  llvm::BasicBlock& done =
      *llvm::BasicBlock::Create(function.getContext(), "match.done", &function);
  return Match(*native_input, llvm::wrap(&done));
}

auto Tetrodotoxin::Library::Llvm::Builder::begin_constant_case(
    Match& state,
    const Ttx::Model::Pack& constant) const -> Core::Option<MatchCase> {
  Llvm::Body& native_body = body;
  auto selected_constant = control_find_value(native_body, constant);
  if (!selected_constant) {
    return {};
  }

  llvm::Value* input = llvm::unwrap(state.get_input());
  llvm::IRBuilder<>& builder = control_native_builder(native_body);
  llvm::Value* matches = input->getType()->isFloatingPointTy()
                             ? builder.CreateFCmpOEQ(input, &*selected_constant)
                             : builder.CreateICmpEQ(input, &*selected_constant);
  llvm::Function& function = control_native_function(native_body);
  llvm::BasicBlock& selected =
      *llvm::BasicBlock::Create(function.getContext(), "match.case", &function);
  llvm::BasicBlock& next =
      *llvm::BasicBlock::Create(function.getContext(), "match.next", &function);
  builder.CreateCondBr(matches, &selected, &next);
  builder.SetInsertPoint(&selected);
  return MatchCase(native_body.get_storage_depth(), llvm::wrap(&next));
}

auto Tetrodotoxin::Library::Llvm::Builder::begin_value_case(
    Match& state,
    const Ttx::Model::Addressable& payload,
    Ttx::Lexical::Anchor) const -> Core::Option<MatchCase> {
  Llvm::Body& native_body = body;
  auto carriers = control_select_carriers(body);
  if (!carriers) {
    return {};
  }

  auto native_type = carriers->get_type(payload.get_type());
  if (!native_type) {
    return {};
  }

  llvm::IRBuilder<>& builder = control_native_builder(native_body);
  llvm::Value* input = llvm::unwrap(state.get_input());
  llvm::Value* present = builder.CreateExtractValue(input, 1);
  llvm::Function& function = control_native_function(native_body);
  llvm::BasicBlock& selected =
      *llvm::BasicBlock::Create(function.getContext(), "match.case", &function);
  llvm::BasicBlock& next =
      *llvm::BasicBlock::Create(function.getContext(), "match.next", &function);
  builder.CreateCondBr(present, &selected, &next);
  builder.SetInsertPoint(&selected);

  Count storage_depth = native_body.get_storage_depth();
  LLVMValueRef address =
      native_body.create_entry_alloca(*native_type, "match.payload"_view);
  LLVMValueRef value = llvm::wrap(builder.CreateExtractValue(input, 0));
  if (!carriers->retain(native_body, payload.get_type(), value)) {
    return {};
  }

  builder.CreateStore(llvm::unwrap(value), llvm::unwrap(address));
  if (!native_body.publish_address(payload, address) ||
      !native_body.register_storage(payload.get_type(), address)) {
    return {};
  }

  return MatchCase(storage_depth, llvm::wrap(&next));
}

auto Tetrodotoxin::Library::Llvm::Builder::end_match_case(
    Match& state,
    MatchCase selected) const -> Bool {
  Llvm::Body& native_body = body;
  llvm::IRBuilder<>& builder = control_native_builder(native_body);
  llvm::BasicBlock* current = builder.GetInsertBlock();
  Bool reaches_done = Bool(current && !current->getTerminator());
  Bool cleaned = !reaches_done ||
                 native_body.emit_storage_cleanup(selected.get_storage_depth());
  if (reaches_done && cleaned) {
    builder.CreateBr(llvm::unwrap(state.get_done()));
    state.set_reaches_done();
  }

  Bool resized = native_body.resize_storage(selected.get_storage_depth());
  auto next = selected.get_next();
  if (next) {
    builder.SetInsertPoint(llvm::unwrap(*next));
  }

  return cleaned && resized;
}

auto Tetrodotoxin::Library::Llvm::Builder::begin_default_case() const
    -> MatchCase {
  return MatchCase(body.get_storage_depth(), {});
}

auto Tetrodotoxin::Library::Llvm::Builder::end_match(
    Match state,
    Bool unmatched_reaches_next) const -> Bool {
  Llvm::Body& native_body = body;
  llvm::IRBuilder<>& builder = control_native_builder(native_body);
  llvm::BasicBlock* current = builder.GetInsertBlock();
  Bool unmatched_active = Bool(current && !current->getTerminator());
  if (unmatched_active && unmatched_reaches_next) {
    builder.CreateBr(llvm::unwrap(state.get_done()));
    state.set_reaches_done();
  } else if (unmatched_active) {
    builder.CreateUnreachable();
  }

  builder.SetInsertPoint(llvm::unwrap(state.get_done()));
  if (!state.reaches_done()) {
    builder.CreateUnreachable();
  }

  return True;
}

// Flow storage follows authored Block and Local lifetimes while temporary
// values are cleared at each completed Statement boundary.

static auto flow_select_carriers(const Llvm::Body& body)
    -> Core::Option<const Tetrodotoxin::Library::Llvm::Carriers&> {
  return body.get_program().get_carriers();
}

static auto flow_native_builder(const Tetrodotoxin::Library::Llvm::Body& body)
    -> llvm::IRBuilder<>& {
  return *reinterpret_cast<llvm::IRBuilder<>*>(body.get_builder());
}

auto Tetrodotoxin::Library::Llvm::Builder::end_block(
    const Ttx::Concept::Abstract& block) const -> Bool {
  Llvm::Body& native_body = body;
  auto scope = native_body.take_block_scope(block);
  if (!scope) {
    return False;
  }

  llvm::BasicBlock* selected =
      flow_native_builder(native_body).GetInsertBlock();
  Bool cleaned = !selected || selected->getTerminator() ||
                 native_body.emit_storage_cleanup(scope->get_storage_depth());
  Bool resized = native_body.resize_storage(scope->get_storage_depth());
  Bool debug = !native_body.get_debug_scope() || native_body.pop_debug_scope();
  return cleaned && resized && debug;
}

auto Tetrodotoxin::Library::Llvm::Builder::end_statement() const -> Bool {
  Llvm::Body& native_body = body;
  return native_body.clear_temporary_cleanup();
}

auto Tetrodotoxin::Library::Llvm::Builder::bind_local(
    const Ttx::Model::Addressable& local,
    const Ttx::Model::Pack& value) const -> Bool {
  Llvm::Body& native_body = body;
  auto carriers = flow_select_carriers(body);
  Llvm::Program& program = body.get_program();
  if (!carriers) {
    return False;
  }

  const Ttx::Model::Type& type = local.get_type();
  auto native_type = carriers->get_type(type);
  auto values = native_body.find_values(value);
  if (!native_type) {
    return program.fail_backend(
        "LLVM cannot bind a Local without its completed carrier."_view);
  }

  if (!values) {
    return program.fail_backend(
        "LLVM cannot bind a Local before its initializer values are published."_view);
  }

  auto stored =
      carriers->fit_and_assemble(native_body, type, value, values->get_view());
  if (!stored) {
    return program.fail_backend(
        "LLVM cannot assemble the value selected for one Local."_view);
  }

  if (!native_body.acquire(type, *stored)) {
    return program.fail_backend(
        "LLVM cannot transfer ownership into one Local."_view);
  }

  LLVMValueRef address =
      native_body.create_entry_alloca(*native_type, "local"_view);
  flow_native_builder(native_body)
      .CreateStore(llvm::unwrap(*stored), llvm::unwrap(address));
  if (!native_body.publish_address(local, address) ||
      !native_body.register_storage(type, address)) {
    return program.fail_backend(
        "LLVM cannot publish storage for one Local."_view);
  }

  return True;
}

// Literals materialize constants with the carrier already reserved by their
// exact semantic Type.

class LiteralSelection {
 public:
  constexpr LiteralSelection(
      Tetrodotoxin::Library::Llvm::Body& body,
      Tetrodotoxin::Library::Llvm::Program& program,
      const Tetrodotoxin::Library::Llvm::Carriers& carriers)
      : body(body), program(program), carriers(carriers) {}

  Tetrodotoxin::Library::Llvm::Body& body;
  Tetrodotoxin::Library::Llvm::Program& program;
  const Tetrodotoxin::Library::Llvm::Carriers& carriers;
};

static auto select_literal_target(Llvm::Body& body)
    -> Core::Option<LiteralSelection> {
  return LiteralSelection(
      body, body.get_program(), body.get_program().get_carriers());
}

static auto publish_literal(
    Tetrodotoxin::Library::Llvm::Body& body,
    const Ttx::Model::Pack& result,
    LLVMValueRef value) -> Bool {
  BAIL_IF(!value);

  Core::Static::Vector<LLVMValueRef, 1> values = {{value}};
  return body.publish_values(result, values.get_view());
}

auto Tetrodotoxin::Library::Llvm::Builder::unsigned_value(
    const Ttx::Model::Type& carrier,
    const Ttx::Model::Pack& result,
    U64 value) const -> Bool {
  auto selected = select_literal_target(body);
  BAIL_IF(!selected);

  auto type = selected->carriers.get_type(carrier);
  BAIL_IF(!type);

  return publish_literal(selected->body, result, LLVMConstInt(*type, value, 0));
}

auto Tetrodotoxin::Library::Llvm::Builder::signed_value(
    const Ttx::Model::Type& carrier,
    const Ttx::Model::Pack& result,
    S64 value) const -> Bool {
  auto selected = select_literal_target(body);
  BAIL_IF(!selected);

  auto type = selected->carriers.get_type(carrier);
  BAIL_IF(!type);

  return publish_literal(
      selected->body, result, LLVMConstInt(*type, U64(value), 1));
}

auto Tetrodotoxin::Library::Llvm::Builder::real_value(
    const Ttx::Model::Type& carrier,
    const Ttx::Model::Pack& result,
    R64 value) const -> Bool {
  auto selected = select_literal_target(body);
  BAIL_IF(!selected);

  auto type = selected->carriers.get_type(carrier);
  BAIL_IF(!type);

  return publish_literal(selected->body, result, LLVMConstReal(*type, value));
}

auto Tetrodotoxin::Library::Llvm::Builder::bytes_value(
    const Ttx::Model::Type& carrier,
    const Ttx::Model::Pack& result,
    Core::View::Bytes value) const -> Bool {
  auto selected = select_literal_target(body);
  BAIL_IF(!selected);

  auto type = selected->carriers.get_type(carrier);
  BAIL_IF(!type);

  LLVMContextRef context = &selected->program.get_context();
  LLVMTypeKind kind = LLVMGetTypeKind(*type);
  if (kind == LLVMStructTypeKind) {
    auto bytes = create_bytes_view(selected->program, *type, value);
    BAIL_IF(!bytes);
    return publish_literal(selected->body, result, *bytes);
  }

  if (kind != LLVMArrayTypeKind ||
      LLVMGetArrayLength2(*type) != value.get_size()) {
    return selected->program.fail_backend(
        "LLVM found incompatible byte Constant storage."_view);
  }

  LLVMValueRef constant = LLVMConstStringInContext2(
      context, reinterpret_cast<const char*>(value.get_data()),
      value.get_size(), 1);
  return publish_literal(selected->body, result, constant);
}

auto Tetrodotoxin::Library::Llvm::Builder::object_value(
    const Ttx::Model::Type& carrier,
    const Ttx::Model::Pack& result) const -> Bool {
  auto selected = select_literal_target(body);
  BAIL_IF(!selected);

  auto type = selected->carriers.get_type(carrier);
  BAIL_IF(!type || LLVMGetTypeKind(*type) != LLVMPointerTypeKind);
  return publish_literal(selected->body, result, LLVMConstNull(*type));
}

auto Tetrodotoxin::Library::Llvm::Builder::enumeration_name(
    const Ttx::Model::Pack& result,
    const Ttx::Model::Type& result_type,
    LLVMValueRef value,
    Core::View::Vector<U64> values,
    Core::View::Vector<Core::View::Bytes> names) const -> Bool {
  auto selected = select_literal_target(body);
  BAIL_IF(!selected || !value || values.get_size() != names.get_size());

  auto native_result = selected->carriers.get_type(result_type);
  Core::Option<llvm::StructType&> view_type;
  if (native_result) {
    auto selected =
        llvm::dyn_cast<llvm::StructType>(llvm::unwrap(*native_result));
    if (selected) {
      view_type = *selected;
    }
  }

  Core::Option<llvm::IntegerType&> native_value;
  auto selected_value =
      llvm::dyn_cast<llvm::IntegerType>(llvm::unwrap(LLVMTypeOf(value)));
  if (selected_value) {
    native_value = *selected_value;
  }
  BAIL_IF(!view_type || !native_value);

  llvm::IRBuilder<>& builder = control_native_builder(selected->body);
  auto empty = create_bytes_view(selected->program, *native_result, {});
  BAIL_IF(!empty);
  llvm::Value* selected_data =
      builder.CreateExtractValue(llvm::unwrap(*empty), 0);
  llvm::Value* selected_size =
      builder.CreateExtractValue(llvm::unwrap(*empty), 1);
  for (Count remaining = values.get_size(); remaining != 0; remaining--) {
    Count index = remaining - 1;
    auto candidate =
        create_bytes_view(selected->program, *native_result, names[index]);
    BAIL_IF(!candidate);

    llvm::Value* matches = builder.CreateICmpEQ(
        llvm::unwrap(value),
        llvm::ConstantInt::get(&*native_value, values[index]),
        "enum.name.value");
    selected_data = builder.CreateSelect(
        matches, builder.CreateExtractValue(llvm::unwrap(*candidate), 0),
        selected_data, "enum.name.data");
    selected_size = builder.CreateSelect(
        matches, builder.CreateExtractValue(llvm::unwrap(*candidate), 1),
        selected_size, "enum.name.size");
  }

  llvm::Value* native = llvm::UndefValue::get(&*view_type);
  native = builder.CreateInsertValue(native, selected_data, 0);
  native = builder.CreateInsertValue(native, selected_size, 1);
  return publish_literal(selected->body, result, llvm::wrap(native));
}

// Short circuit logic returns its merge blocks to And or Or so no hidden
// operation state survives between their left and right inputs.

static auto logic_find_scalar(
    const Tetrodotoxin::Library::Llvm::Body& body,
    const Ttx::Model::Pack& pack) -> Core::Option<LLVMValueRef> {
  auto values = body.find_values(pack);
  if (!values || values->get_size() != 1) {
    return {};
  }

  return values->get_data()[0];
}

auto Tetrodotoxin::Library::Llvm::Builder::begin_logic(
    Logical operation,
    const Ttx::Model::Pack& left) const -> Core::Option<Logic> {
  Llvm::Body& native_body = body;
  auto left_value = logic_find_scalar(native_body, left);
  LLVMBasicBlockRef left_block = LLVMGetInsertBlock(native_body.get_builder());
  if (!left_value || !left_block) {
    return {};
  }

  LLVMContextRef context =
      LLVMGetModuleContext(LLVMGetGlobalParent(native_body.get_function()));
  LLVMBasicBlockRef right = LLVMAppendBasicBlockInContext(
      context, native_body.get_function(), "logical.right");
  LLVMBasicBlockRef merge = LLVMAppendBasicBlockInContext(
      context, native_body.get_function(), "logical.merge");
  if (operation == Logical::And) {
    LLVMBuildCondBr(native_body.get_builder(), *left_value, right, merge);
  } else {
    LLVMBuildCondBr(native_body.get_builder(), *left_value, merge, right);
  }

  LLVMPositionBuilderAtEnd(native_body.get_builder(), right);
  return Logic(left_block, merge);
}

auto Tetrodotoxin::Library::Llvm::Builder::end_logic(
    Logic state,
    const Ttx::Model::Pack& result,
    const Ttx::Model::Pack& left,
    const Ttx::Model::Pack& right) const -> Bool {
  Llvm::Body& native_body = body;
  auto left_value = logic_find_scalar(native_body, left);
  auto right_value = logic_find_scalar(native_body, right);
  LLVMBasicBlockRef right_block = LLVMGetInsertBlock(native_body.get_builder());
  if (!left_value || !right_value || !right_block) {
    return False;
  }

  LLVMBuildBr(native_body.get_builder(), state.get_merge());
  LLVMPositionBuilderAtEnd(native_body.get_builder(), state.get_merge());
  LLVMValueRef selected =
      LLVMBuildPhi(native_body.get_builder(), LLVMTypeOf(*left_value), "");
  Core::Static::Vector<LLVMValueRef, 2> incoming_values = {{
    *left_value,
    *right_value,
  }};
  Core::Static::Vector<LLVMBasicBlockRef, 2> incoming_blocks = {{
    state.get_left(),
    right_block,
  }};
  LLVMAddIncoming(
      selected, incoming_values.get_data(), incoming_blocks.get_data(),
      U32(incoming_values.get_size()));

  Core::Static::Vector<LLVMValueRef, 1> values = {{selected}};
  return native_body.publish_values(result, values.get_view());
}

auto Tetrodotoxin::Library::Llvm::Builder::logical_not(
    const Ttx::Model::Pack& result,
    const Ttx::Model::Pack& operand) const -> Bool {
  Llvm::Body& native_body = body;
  auto value = logic_find_scalar(native_body, operand);
  if (!value) {
    return False;
  }

  LLVMValueRef selected = LLVMBuildNot(native_body.get_builder(), *value, "");
  Core::Static::Vector<LLVMValueRef, 1> values = {{selected}};
  return native_body.publish_values(result, values.get_view());
}

// Option lowering follows the Perimortem inline payload and presence flag
// carrier. Only an engaged payload participates in ownership.

static auto option_select_carriers(const Llvm::Body& body)
    -> Core::Option<const Tetrodotoxin::Library::Llvm::Carriers&> {
  return body.get_program().get_carriers();
}

static auto option_native_builder(const Tetrodotoxin::Library::Llvm::Body& body)
    -> llvm::IRBuilder<>& {
  return *reinterpret_cast<llvm::IRBuilder<>*>(body.get_builder());
}

static auto option_native_function(
    const Tetrodotoxin::Library::Llvm::Body& body) -> llvm::Function& {
  return *llvm::unwrap<llvm::Function>(body.get_function());
}

static auto publish_option(
    Tetrodotoxin::Library::Llvm::Body& body,
    const Ttx::Model::Pack& result,
    LLVMValueRef value) -> Bool {
  Core::Static::Vector<LLVMValueRef, 1> values = {{value}};
  return body.publish_values(result, values.get_view());
}

auto Tetrodotoxin::Library::Llvm::Builder::absent(
    const Ttx::Model::Type& carrier,
    const Ttx::Model::Pack& result) const -> Bool {
  Llvm::Body& native_body = body;
  auto carriers = option_select_carriers(body);
  if (!carriers) {
    return False;
  }

  auto value = carriers->zero(body.get_program(), carrier);
  return value && publish_option(native_body, result, *value);
}

auto Tetrodotoxin::Library::Llvm::Builder::present(
    const Ttx::Model::Type& carrier,
    const Ttx::Model::Type& element,
    const Ttx::Model::Pack& result,
    const Ttx::Model::Pack& payload) const -> Bool {
  Llvm::Body& native_body = body;
  auto carriers = option_select_carriers(body);
  if (!carriers) {
    return False;
  }

  auto native_carrier = carriers->get_type(carrier);
  auto payload_values = native_body.find_values(payload);
  if (!native_carrier || !payload_values) {
    return False;
  }

  auto native_payload = carriers->fit_and_assemble(
      native_body, element, payload, payload_values->get_view());
  if (!native_payload || !native_body.acquire(element, *native_payload)) {
    return False;
  }

  if (carriers->is_object(element)) {
    native_body.mark_owned(carrier, *native_payload);
    return publish_option(native_body, result, *native_payload);
  }

  llvm::IRBuilder<>& builder = option_native_builder(native_body);
  llvm::Value& empty = *llvm::UndefValue::get(llvm::unwrap(*native_carrier));
  llvm::Value& with_payload =
      *builder.CreateInsertValue(&empty, llvm::unwrap(*native_payload), 0);
  llvm::Value& selected =
      *builder.CreateInsertValue(&with_payload, builder.getTrue(), 1);
  LLVMValueRef selected_handle = llvm::wrap(&selected);
  native_body.mark_owned(carrier, selected_handle);
  return publish_option(native_body, result, selected_handle);
}

auto Tetrodotoxin::Library::Llvm::Builder::result(
    const Ttx::Model::Type& carrier,
    const Ttx::Model::Pack& result,
    const Ttx::Model::Pack& payload) const -> Bool {
  Llvm::Body& native_body = body;
  auto carriers = option_select_carriers(body);
  auto payload_values = native_body.find_values(payload);
  if (!carriers || !payload_values) {
    return False;
  }

  auto selected = carriers->fit_and_assemble(
      native_body, carrier, payload, payload_values->get_view());
  return selected && publish_option(native_body, result, *selected);
}

auto Tetrodotoxin::Library::Llvm::Builder::begin_unwrap(
    const Ttx::Model::Type& carrier,
    const Ttx::Model::Type& element,
    const Ttx::Model::Pack& option) const -> Core::Option<Choice> {
  Llvm::Body& native_body = body;
  auto carriers = option_select_carriers(body);
  if (!carriers) {
    return {};
  }

  auto native_option = native_body.find_value(option);
  auto native_carrier = carriers->get_type(carrier);
  if (!native_option || !native_carrier ||
      llvm::unwrap(*native_option)->getType() !=
          llvm::unwrap(*native_carrier)) {
    return {};
  }

  llvm::IRBuilder<>& builder = option_native_builder(native_body);
  llvm::Function& function = option_native_function(native_body);
  Bool niche = carriers->is_object(element);
  llvm::Value& present =
      niche ? *builder.CreateIsNotNull(llvm::unwrap(*native_option))
            : *builder.CreateExtractValue(llvm::unwrap(*native_option), 1);
  llvm::BasicBlock& payload_block = *llvm::BasicBlock::Create(
      function.getContext(), "option.payload", &function);
  llvm::BasicBlock& default_block = *llvm::BasicBlock::Create(
      function.getContext(), "option.default", &function);
  llvm::BasicBlock& merge = *llvm::BasicBlock::Create(
      function.getContext(), "option.merge", &function);
  builder.CreateCondBr(&present, &payload_block, &default_block);

  builder.SetInsertPoint(&payload_block);
  llvm::Value& payload =
      niche ? *llvm::unwrap(*native_option)
            : *builder.CreateExtractValue(llvm::unwrap(*native_option), 0);
  if (!carriers->retain(native_body, element, llvm::wrap(&payload))) {
    return {};
  }

  builder.CreateBr(&merge);
  llvm::BasicBlock* payload_end = builder.GetInsertBlock();
  if (!payload_end) {
    return {};
  }

  builder.SetInsertPoint(&default_block);
  return Choice(
      llvm::wrap(&payload), llvm::wrap(payload_end), llvm::wrap(&merge));
}

auto Tetrodotoxin::Library::Llvm::Builder::end_unwrap(
    Choice state,
    const Ttx::Model::Type& element,
    const Ttx::Model::Pack& result,
    const Ttx::Model::Pack& fallback) const -> Bool {
  Llvm::Body& native_body = body;
  auto carriers = option_select_carriers(body);
  if (!carriers) {
    return False;
  }

  auto fallback_values = native_body.find_values(fallback);
  if (!fallback_values) {
    return False;
  }

  auto default_value = carriers->fit_and_assemble(
      native_body, element, fallback, fallback_values->get_view());
  if (!default_value || !native_body.acquire(element, *default_value)) {
    return False;
  }

  llvm::IRBuilder<>& builder = option_native_builder(native_body);
  builder.CreateBr(llvm::unwrap(state.get_merge()));
  llvm::BasicBlock* default_end = builder.GetInsertBlock();
  if (!default_end) {
    return False;
  }

  builder.SetInsertPoint(llvm::unwrap(state.get_merge()));
  llvm::PHINode& selected =
      *builder.CreatePHI(llvm::unwrap(*default_value)->getType(), 2);
  selected.addIncoming(
      llvm::unwrap(state.get_selected()),
      llvm::unwrap(state.get_selected_end()));
  selected.addIncoming(llvm::unwrap(*default_value), default_end);
  LLVMValueRef selected_handle = llvm::wrap(&selected);
  native_body.mark_owned(element, selected_handle);
  return publish_option(native_body, result, selected_handle);
}

auto Tetrodotoxin::Library::Llvm::Builder::propagate_option(
    const Ttx::Model::Type& carrier,
    const Ttx::Model::Type& element,
    const Ttx::Model::Pack& result,
    const Ttx::Model::Pack& option,
    const Ttx::Model::Pack& escape) const -> Bool {
  Llvm::Body& native_body = body;
  auto carriers = option_select_carriers(body);
  if (!carriers) {
    return False;
  }

  auto native_option = native_body.find_value(option);
  auto native_carrier = carriers->get_type(carrier);
  if (!native_option || !native_carrier ||
      llvm::unwrap(*native_option)->getType() !=
          llvm::unwrap(*native_carrier)) {
    return False;
  }

  llvm::IRBuilder<>& builder = option_native_builder(native_body);
  llvm::Function& function = option_native_function(native_body);
  llvm::BasicBlock& payload_block = *llvm::BasicBlock::Create(
      function.getContext(), "propagate.payload", &function);
  llvm::BasicBlock& absent_block = *llvm::BasicBlock::Create(
      function.getContext(), "propagate.absent", &function);
  llvm::BasicBlock& continued = *llvm::BasicBlock::Create(
      function.getContext(), "propagate.continue", &function);
  Bool niche = carriers->is_object(element);
  llvm::Value& present =
      niche ? *builder.CreateIsNotNull(llvm::unwrap(*native_option))
            : *builder.CreateExtractValue(llvm::unwrap(*native_option), 1);
  builder.CreateCondBr(&present, &payload_block, &absent_block);

  builder.SetInsertPoint(&absent_block);
  if (!native_body.publish_values(escape, Core::View::Vector<LLVMValueRef>()) ||
      !control_return_values(native_body, escape, True)) {
    return False;
  }

  builder.SetInsertPoint(&payload_block);
  llvm::Value& payload =
      niche ? *llvm::unwrap(*native_option)
            : *builder.CreateExtractValue(llvm::unwrap(*native_option), 0);
  if (!carriers->retain(native_body, element, llvm::wrap(&payload))) {
    return False;
  }

  builder.CreateBr(&continued);
  builder.SetInsertPoint(&continued);
  LLVMValueRef payload_handle = llvm::wrap(&payload);
  native_body.mark_owned(element, payload_handle);
  return publish_option(native_body, result, payload_handle);
}

auto Tetrodotoxin::Library::Llvm::Builder::propagate_flag(
    const Ttx::Model::Type& carrier,
    const Ttx::Model::Pack& result,
    const Ttx::Model::Pack& flag,
    const Ttx::Model::Pack& escape) const -> Bool {
  Llvm::Body& native_body = body;
  auto carriers = option_select_carriers(body);
  auto native_flag = native_body.find_value(flag);
  auto native_carrier =
      carriers ? carriers->get_type(carrier) : Core::Option<LLVMTypeRef>();
  if (!carriers || !native_flag || !native_carrier ||
      !carriers->is_flag(carrier) ||
      LLVMTypeOf(*native_flag) != *native_carrier) {
    return False;
  }

  llvm::IRBuilder<>& builder = option_native_builder(native_body);
  llvm::Function& function = option_native_function(native_body);
  llvm::BasicBlock& continued = *llvm::BasicBlock::Create(
      function.getContext(), "propagate.continue", &function);
  llvm::BasicBlock& inactive = *llvm::BasicBlock::Create(
      function.getContext(), "propagate.inactive", &function);
  builder.CreateCondBr(llvm::unwrap(*native_flag), &continued, &inactive);

  builder.SetInsertPoint(&inactive);
  if (!native_body.publish_values(escape, Core::View::Vector<LLVMValueRef>()) ||
      !control_return_values(native_body, escape, True)) {
    return False;
  }

  builder.SetInsertPoint(&continued);
  Core::Static::Vector<LLVMValueRef, 1> value = {{*native_flag}};
  return native_body.publish_values(result, value.get_view());
}

auto Tetrodotoxin::Library::Llvm::Builder::propagate_result(
    const Ttx::Model::Type& carrier,
    const Ttx::Model::Type& value,
    const Ttx::Model::Type& error,
    const Ttx::Model::Pack& result,
    const Ttx::Model::Pack& source,
    const Ttx::Model::Pack& escape) const -> Bool {
  Llvm::Body& native_body = body;
  auto carriers = option_select_carriers(body);
  auto native_result = native_body.find_value(source);
  auto native_carrier =
      carriers ? carriers->get_type(carrier) : Core::Option<LLVMTypeRef>();
  if (!carriers || !native_result || !native_carrier ||
      LLVMTypeOf(*native_result) != *native_carrier) {
    return False;
  }

  llvm::IRBuilder<>& builder = option_native_builder(native_body);
  llvm::Function& function = option_native_function(native_body);
  llvm::Value& value_selected =
      *builder.CreateExtractValue(llvm::unwrap(*native_result), U32(1));
  llvm::BasicBlock& value_block = *llvm::BasicBlock::Create(
      function.getContext(), "propagate.value", &function);
  llvm::BasicBlock& error_block = *llvm::BasicBlock::Create(
      function.getContext(), "propagate.error", &function);
  llvm::BasicBlock& continued = *llvm::BasicBlock::Create(
      function.getContext(), "propagate.continue", &function);
  builder.CreateCondBr(&value_selected, &value_block, &error_block);

  builder.SetInsertPoint(&error_block);
  auto native_error =
      carriers->select_result(native_body, carrier, *native_result, False);
  if (!native_error || !carriers->retain(native_body, error, *native_error)) {
    return False;
  }

  native_body.mark_owned(error, *native_error);
  Core::Static::Vector<LLVMValueRef, 1> escaped = {{*native_error}};
  if (!native_body.publish_values(escape, escaped.get_view()) ||
      !control_return_values(native_body, escape, True)) {
    return False;
  }

  builder.SetInsertPoint(&value_block);
  auto native_value =
      carriers->select_result(native_body, carrier, *native_result, True);
  if (!native_value || !carriers->retain(native_body, value, *native_value)) {
    return False;
  }

  builder.CreateBr(&continued);
  builder.SetInsertPoint(&continued);
  native_body.mark_owned(value, *native_value);
  Core::Static::Vector<LLVMValueRef, 1> continued_value = {{*native_value}};
  return native_body.publish_values(result, continued_value.get_view());
}

// Sequence operations preserve contiguous pointer and length carriers while
// their semantic owners retain scalar or ranged selection state.

struct ContiguousSelection {
  llvm::Value& data;
  llvm::Value& length;
};

static auto sequence_select_carriers(const Llvm::Body& body)
    -> Core::Option<const Tetrodotoxin::Library::Llvm::Carriers&> {
  return body.get_program().get_carriers();
}

static auto sequence_native_builder(
    const Tetrodotoxin::Library::Llvm::Body& body) -> llvm::IRBuilder<>& {
  return *reinterpret_cast<llvm::IRBuilder<>*>(body.get_builder());
}

static auto sequence_native_function(
    const Tetrodotoxin::Library::Llvm::Body& body) -> llvm::Function& {
  return *llvm::unwrap<llvm::Function>(body.get_function());
}

static auto sequence_find_value(
    const Tetrodotoxin::Library::Llvm::Body& body,
    const Ttx::Model::Pack& pack) -> Core::Option<llvm::Value&> {
  auto found = body.find_value(pack);
  return found ? Core::Option<llvm::Value&>(*llvm::unwrap(*found))
               : Core::Option<llvm::Value&>();
}

static auto select_contiguous_value(
    Tetrodotoxin::Library::Llvm::Body& body,
    const Ttx::Model::Pack& receiver) -> Core::Option<ContiguousSelection> {
  auto value = sequence_find_value(body, receiver);
  if (!value) {
    return {};
  }

  llvm::IRBuilder<>& builder = sequence_native_builder(body);
  auto array = llvm::dyn_cast<llvm::ArrayType>(value->getType());
  if (array) {
    LLVMValueRef storage_handle =
        body.create_entry_alloca(llvm::wrap(array), "sequence.storage"_view);
    llvm::Value& storage = *llvm::unwrap(storage_handle);
    builder.CreateStore(&*value, &storage);
    llvm::Value& data = *builder.CreateInBoundsGEP(
        array, &storage, {builder.getInt64(0), builder.getInt64(0)});
    llvm::Value& length = *builder.getInt64(array->getNumElements());
    return ContiguousSelection{data, length};
  }

  auto structure = llvm::dyn_cast<llvm::StructType>(value->getType());
  if (!structure || structure->getNumElements() != 2) {
    return {};
  }

  llvm::Value& data = *builder.CreateExtractValue(&*value, 0);
  llvm::Value& length = *builder.CreateExtractValue(&*value, 1);
  return ContiguousSelection{data, length};
}

static auto publish_sequence(
    Tetrodotoxin::Library::Llvm::Body& body,
    const Ttx::Model::Pack& result,
    LLVMValueRef value) -> Bool {
  Core::Static::Vector<LLVMValueRef, 1> values = {{value}};
  return body.publish_values(result, values.get_view());
}

auto Tetrodotoxin::Library::Llvm::Builder::range(
    const Ttx::Model::Type& carrier,
    const Ttx::Model::Pack& result,
    const Ttx::Model::Pack& start,
    const Ttx::Model::Pack& end) const -> Bool {
  Llvm::Body& native_body = body;
  auto carriers = sequence_select_carriers(body);
  if (!carriers) {
    return False;
  }

  auto first = sequence_find_value(native_body, start);
  auto last = sequence_find_value(native_body, end);
  if (!first || !last) {
    return False;
  }

  Core::Static::Vector<LLVMValueRef, 2> elements = {{
    llvm::wrap(&*first),
    llvm::wrap(&*last),
  }};
  auto assembled =
      carriers->assemble(native_body, carrier, elements.get_view());
  return assembled && publish_sequence(native_body, result, *assembled);
}

auto Tetrodotoxin::Library::Llvm::Builder::empty_range(
    const Ttx::Model::Type& carrier,
    const Ttx::Model::Pack& result) const -> Bool {
  Llvm::Body& native_body = body;
  auto carriers = sequence_select_carriers(body);
  if (!carriers) {
    return False;
  }

  auto value = carriers->zero(body.get_program(), carrier);
  return value && publish_sequence(native_body, result, *value);
}

auto Tetrodotoxin::Library::Llvm::Builder::select_index(
    const Ttx::Model::Type& element,
    const Ttx::Model::Pack& result,
    const Ttx::Model::Pack& receiver,
    const Ttx::Model::Pack& index) const -> Bool {
  Llvm::Body& native_body = body;
  auto carriers = sequence_select_carriers(body);
  if (!carriers) {
    return False;
  }

  auto contiguous = select_contiguous_value(native_body, receiver);
  auto first = sequence_find_value(native_body, index);
  auto native_element = carriers->get_type(element);
  if (!contiguous || !first || !native_element) {
    return False;
  }

  return native_body.publish_indexed_target(
      result, element, *native_element, llvm::wrap(&contiguous->data),
      llvm::wrap(&contiguous->length), llvm::wrap(&*first), {});
}

auto Tetrodotoxin::Library::Llvm::Builder::select_range(
    const Ttx::Model::Type& element,
    const Ttx::Model::Pack& result,
    const Ttx::Model::Pack& receiver,
    const Ttx::Model::Pack& start,
    const Ttx::Model::Pack& count,
    Count size) const -> Bool {
  Llvm::Body& native_body = body;
  auto carriers = sequence_select_carriers(body);
  if (!carriers) {
    return False;
  }

  auto contiguous = select_contiguous_value(native_body, receiver);
  auto first = sequence_find_value(native_body, start);
  auto native_count = sequence_find_value(native_body, count);
  auto native_element = carriers->get_type(element);
  if (!contiguous || !first || !native_count || !native_element) {
    return False;
  }

  return native_body.publish_indexed_target(
      result, element, *native_element, llvm::wrap(&contiguous->data),
      llvm::wrap(&contiguous->length), llvm::wrap(&*first), size);
}

auto Tetrodotoxin::Library::Llvm::Builder::begin_slice(
    const Ttx::Model::Type& element,
    const Ttx::Model::Pack& receiver,
    const Ttx::Model::Pack& index) const -> Core::Option<Choice> {
  Llvm::Body& native_body = body;
  auto carriers = sequence_select_carriers(body);
  if (!carriers) {
    return {};
  }

  auto contiguous = select_contiguous_value(native_body, receiver);
  auto first = sequence_find_value(native_body, index);
  auto native_element = carriers->get_type(element);
  if (!contiguous || !first || !native_element) {
    return {};
  }

  llvm::IRBuilder<>& builder = sequence_native_builder(native_body);
  llvm::Function& function = sequence_native_function(native_body);
  llvm::BasicBlock& selected_block = *llvm::BasicBlock::Create(
      function.getContext(), "slice.selected", &function);
  llvm::BasicBlock& missing_block = *llvm::BasicBlock::Create(
      function.getContext(), "slice.missing", &function);
  llvm::BasicBlock& merge = *llvm::BasicBlock::Create(
      function.getContext(), "slice.merge", &function);
  llvm::Value& in_bounds = *builder.CreateICmpULT(&*first, &contiguous->length);
  builder.CreateCondBr(&in_bounds, &selected_block, &missing_block);

  builder.SetInsertPoint(&selected_block);
  llvm::Value& address = *builder.CreateGEP(
      llvm::unwrap(*native_element), &contiguous->data, &*first);
  llvm::Value& selected =
      *builder.CreateLoad(llvm::unwrap(*native_element), &address);
  if (!carriers->retain(native_body, element, llvm::wrap(&selected))) {
    return {};
  }

  builder.CreateBr(&merge);
  llvm::BasicBlock* selected_end = builder.GetInsertBlock();
  if (!selected_end) {
    return {};
  }

  builder.SetInsertPoint(&missing_block);
  return Choice(
      llvm::wrap(&selected), llvm::wrap(selected_end), llvm::wrap(&merge));
}

auto Tetrodotoxin::Library::Llvm::Builder::end_slice(
    Choice state,
    const Ttx::Model::Type& element,
    const Ttx::Model::Pack& result,
    const Ttx::Model::Pack& fallback) const -> Bool {
  Llvm::Body& native_body = body;
  auto carriers = sequence_select_carriers(body);
  if (!carriers) {
    return False;
  }

  auto fallback_values = native_body.find_values(fallback);
  if (!fallback_values) {
    return False;
  }

  auto default_value = carriers->fit_and_assemble(
      native_body, element, fallback, fallback_values->get_view());
  if (!default_value || !native_body.acquire(element, *default_value)) {
    return False;
  }

  llvm::IRBuilder<>& builder = sequence_native_builder(native_body);
  builder.CreateBr(llvm::unwrap(state.get_merge()));
  llvm::BasicBlock* default_end = builder.GetInsertBlock();
  if (!default_end) {
    return False;
  }

  builder.SetInsertPoint(llvm::unwrap(state.get_merge()));
  llvm::PHINode& selected =
      *builder.CreatePHI(llvm::unwrap(*default_value)->getType(), 2);
  selected.addIncoming(
      llvm::unwrap(state.get_selected()),
      llvm::unwrap(state.get_selected_end()));
  selected.addIncoming(llvm::unwrap(*default_value), default_end);
  LLVMValueRef selected_handle = llvm::wrap(&selected);
  native_body.mark_owned(element, selected_handle);
  return publish_sequence(native_body, result, selected_handle);
}

auto Tetrodotoxin::Library::Llvm::Builder::begin_slice_range(
    const Ttx::Model::Type& element,
    const Ttx::Model::Pack& receiver,
    const Ttx::Model::Pack& start) const -> Core::Option<SliceRange> {
  Llvm::Body& native_body = body;
  auto carriers = sequence_select_carriers(body);
  if (!carriers || !carriers->get_type(element)) {
    return {};
  }

  auto contiguous = select_contiguous_value(native_body, receiver);
  auto first = sequence_find_value(native_body, start);
  if (!contiguous || !first) {
    return {};
  }

  return SliceRange(
      element, llvm::wrap(&contiguous->data), llvm::wrap(&contiguous->length),
      llvm::wrap(&*first));
}

auto Tetrodotoxin::Library::Llvm::Builder::begin_slice_slot(
    const SliceRange& range,
    Count offset) const -> Core::Option<Choice> {
  Llvm::Body& native_body = body;
  auto carriers = sequence_select_carriers(body);
  if (!carriers) {
    return {};
  }

  auto native_element = carriers->get_type(range.get_element());
  if (!native_element) {
    return {};
  }

  llvm::IRBuilder<>& builder = sequence_native_builder(native_body);
  llvm::Function& function = sequence_native_function(native_body);
  llvm::Value& native_offset = *llvm::ConstantInt::get(
      llvm::unwrap(range.get_first())->getType(), offset);
  llvm::Value& index =
      *builder.CreateAdd(llvm::unwrap(range.get_first()), &native_offset);
  llvm::Value& in_bounds =
      *builder.CreateICmpULT(&index, llvm::unwrap(range.get_length()));
  llvm::BasicBlock& selected_block = *llvm::BasicBlock::Create(
      function.getContext(), "slice.selected", &function);
  llvm::BasicBlock& missing_block = *llvm::BasicBlock::Create(
      function.getContext(), "slice.missing", &function);
  llvm::BasicBlock& merge = *llvm::BasicBlock::Create(
      function.getContext(), "slice.merge", &function);
  builder.CreateCondBr(&in_bounds, &selected_block, &missing_block);

  builder.SetInsertPoint(&selected_block);
  llvm::Value& address = *builder.CreateGEP(
      llvm::unwrap(*native_element), llvm::unwrap(range.get_data()), &index);
  llvm::Value& selected =
      *builder.CreateLoad(llvm::unwrap(*native_element), &address);
  if (!carriers->retain(
          native_body, range.get_element(), llvm::wrap(&selected))) {
    return {};
  }

  builder.CreateBr(&merge);
  llvm::BasicBlock* selected_end = builder.GetInsertBlock();
  if (!selected_end) {
    return {};
  }

  builder.SetInsertPoint(&missing_block);
  return Choice(
      llvm::wrap(&selected), llvm::wrap(selected_end), llvm::wrap(&merge));
}

auto Tetrodotoxin::Library::Llvm::Builder::end_slice_slot(
    Choice state,
    const Ttx::Model::Type& element,
    const Ttx::Model::Pack& fallback) const -> Core::Option<LLVMValueRef> {
  Llvm::Body& native_body = body;
  auto carriers = sequence_select_carriers(body);
  if (!carriers) {
    return {};
  }

  auto fallback_values = native_body.find_values(fallback);
  if (!fallback_values) {
    return {};
  }

  auto default_value = carriers->fit_and_assemble(
      native_body, element, fallback, fallback_values->get_view());
  if (!default_value || !native_body.acquire(element, *default_value)) {
    return {};
  }

  llvm::IRBuilder<>& builder = sequence_native_builder(native_body);
  builder.CreateBr(llvm::unwrap(state.get_merge()));
  llvm::BasicBlock* default_end = builder.GetInsertBlock();
  if (!default_end) {
    return {};
  }

  builder.SetInsertPoint(llvm::unwrap(state.get_merge()));
  llvm::PHINode& selected =
      *builder.CreatePHI(llvm::unwrap(*default_value)->getType(), 2);
  selected.addIncoming(
      llvm::unwrap(state.get_selected()),
      llvm::unwrap(state.get_selected_end()));
  selected.addIncoming(llvm::unwrap(*default_value), default_end);
  LLVMValueRef selected_value = llvm::wrap(&selected);
  native_body.mark_owned(element, selected_value);
  return selected_value;
}

auto Tetrodotoxin::Library::Llvm::Builder::end_slice_range(
    const Ttx::Model::Pack& result,
    Core::View::Vector<LLVMValueRef> values) const -> Bool {
  Llvm::Body& native_body = body;
  return native_body.publish_values(result, values);
}

// Storage selection maps exact Addressable and Pack identities to local,
// global, member, or indexed native addresses.

class StorageSelection {
 public:
  constexpr StorageSelection(
      Tetrodotoxin::Library::Llvm::Body& body,
      Tetrodotoxin::Library::Llvm::Program& program,
      const Tetrodotoxin::Library::Llvm::Carriers& carriers)
      : body(body), program(program), carriers(carriers) {}

  Tetrodotoxin::Library::Llvm::Body& body;
  Tetrodotoxin::Library::Llvm::Program& program;
  const Tetrodotoxin::Library::Llvm::Carriers& carriers;
};

static auto select_storage_value(Llvm::Body& body)
    -> Core::Option<StorageSelection> {
  return StorageSelection(
      body, body.get_program(), body.get_program().get_carriers());
}

static auto select_global_storage(const Llvm::Body& body)
    -> Core::Option<const Tetrodotoxin::Library::Llvm::Globals&> {
  return body.get_program().get_globals();
}

static auto publish_storage(
    Tetrodotoxin::Library::Llvm::Body& body,
    const Ttx::Model::Pack& result,
    LLVMValueRef value) -> Bool {
  Core::Static::Vector<LLVMValueRef, 1> values = {{value}};
  return value && body.publish_values(result, values.get_view());
}

auto Tetrodotoxin::Library::Llvm::Builder::select(
    const Ttx::Model::Pack& result,
    const Ttx::Model::Addressable& addressable) const -> Bool {
  auto selected = select_storage_value(body);
  if (!selected) {
    return False;
  }

  auto address = selected->body.find_address(addressable);
  if (!address) {
    auto globals = select_global_storage(body);
    if (globals) {
      address = globals->find_address(addressable);
    }
  }

  if (!address) {
    return selected->program.fail_backend(
        "LLVM cannot select storage before its exact Addressable publishes an address."_view);
  }

  return selected->body.publish_target_address(
      result, addressable.get_type(), *address);
}

auto Tetrodotoxin::Library::Llvm::Builder::select_member(
    const Ttx::Model::Pack& result,
    const Ttx::Model::Addressable& addressable,
    const Ttx::Model::Pack& receiver) const -> Bool {
  auto selected = select_storage_value(body);
  if (!selected) {
    return False;
  }

  auto host = selected->carriers.get_field_host(addressable);
  auto field_index = selected->carriers.get_field_index(addressable);
  if (!host || !field_index) {
    return selected->program.fail_backend(
        "LLVM cannot select a member before its host carrier publishes the Field index."_view);
  }

  auto payload = selected->carriers.get_payload(*host);
  if (!payload) {
    return selected->program.fail_backend(
        "LLVM cannot address a member without its completed host payload carrier."_view);
  }

  Core::Option<LLVMValueRef> base;
  if (selected->carriers.is_object(*host)) {
    auto value = selected->body.find_value(receiver);
    auto native_host = selected->carriers.get_type(*host);
    if (!value || !native_host || LLVMTypeOf(*value) != *native_host) {
      return selected->program.fail_backend(
          "LLVM Object member access requires one exact receiver handle."_view);
    }

    base = *value;
  } else {
    auto target = selected->body.find_target_address(receiver);
    if (!target || &target->get_type() != &*host) {
      return selected->program.fail_backend(
          "LLVM inline member access requires the receiver's exact storage address."_view);
    }

    base = target->get_address();
  }

  if (!base) {
    return False;
  }

  LLVMValueRef member = LLVMBuildStructGEP2(
      selected->body.get_builder(), *payload, *base, U32(*field_index),
      "member");
  if (!member) {
    return selected->program.fail_backend(
        "LLVM could not create the selected member address."_view);
  }

  if (!selected->carriers.is_object(*host)) {
    auto receiver_value = selected->body.find_value(receiver);
    if (receiver_value &&
        !call_release_owned(
            selected->body, selected->carriers, *host, *receiver_value)) {
      return False;
    }
  }

  return selected->body.publish_target_address(
      result, addressable.get_type(), member);
}

auto Tetrodotoxin::Library::Llvm::Builder::load(
    const Ttx::Model::Pack& result) const -> Bool {
  auto selected = select_storage_value(body);
  if (!selected) {
    return False;
  }

  auto target = selected->body.find_target_address(result);
  if (!target) {
    return selected->program.fail_backend(
        "LLVM cannot load a Pack before its exact storage target is selected."_view);
  }

  auto native_type = selected->carriers.get_type(target->get_type());
  if (!native_type) {
    return selected->program.fail_backend(
        "LLVM cannot load storage without its completed value carrier."_view);
  }

  LLVMValueRef value = LLVMBuildLoad2(
      selected->body.get_builder(), *native_type, target->get_address(),
      "load");
  if (!value ||
      !selected->carriers.retain(selected->body, target->get_type(), value)) {
    return False;
  }

  selected->body.mark_owned(target->get_type(), value);
  return publish_storage(selected->body, result, value);
}

// Value composition publishes native results under the original Pack identity
// without creating another lowered graph.

auto Tetrodotoxin::Library::Llvm::Builder::compose(
    const Ttx::Model::Pack& result) const -> Bool {
  Llvm::Body& native_body = body;
  Memory::Dynamic::Vector<LLVMValueRef> composed;
  Count output_count = result.get_layout().get_size();
  for (Count output = 0; output < output_count; output++) {
    auto produced = result.get_produced(output);
    if (!produced) {
      return False;
    }

    auto source = native_body.find_values(produced->producer);
    if (!source || produced->local_index >= source->get_size()) {
      return False;
    }

    composed.insert(source->get_data()[produced->local_index]);
  }

  return native_body.publish_values(result, composed.get_view());
}

auto Tetrodotoxin::Library::Llvm::Builder::alias(
    const Ttx::Model::Pack& result,
    const Ttx::Model::Pack& source) const -> Bool {
  Llvm::Body& native_body = body;
  auto source_values = native_body.find_values(source);
  if (!source_values) {
    return False;
  }

  return native_body.publish_values(result, source_values->get_view());
}

// Writes complete bounds checks before mutation and transfer ownership only
// after the complete target accepts the source value.

class WriteSelection {
 public:
  constexpr WriteSelection(
      Tetrodotoxin::Library::Llvm::Body& body,
      Tetrodotoxin::Library::Llvm::Program& program,
      const Tetrodotoxin::Library::Llvm::Carriers& carriers)
      : body(body), program(program), carriers(carriers) {}

  Tetrodotoxin::Library::Llvm::Body& body;
  Tetrodotoxin::Library::Llvm::Program& program;
  const Tetrodotoxin::Library::Llvm::Carriers& carriers;
};

class WriteBranches {
 public:
  constexpr explicit WriteBranches(LLVMBasicBlockRef done) : done(done) {}

  LLVMBasicBlockRef done;
};

enum class StoreOwnership : U8 {
  ConsumeTemporary,
  RetainTemporary,
};

static auto select_write_target(Llvm::Body& body)
    -> Core::Option<WriteSelection> {
  return WriteSelection(
      body, body.get_program(), body.get_program().get_carriers());
}

static auto publish_write(
    Tetrodotoxin::Library::Llvm::Body& body,
    const Ttx::Model::Pack& result) -> Bool {
  Core::View::Vector<LLVMValueRef> values;
  return body.publish_values(result, values);
}

static auto replace_written_value(
    WriteSelection& selected,
    const Ttx::Model::Type& type,
    LLVMValueRef address,
    LLVMValueRef value,
    StoreOwnership ownership) -> Bool {
  auto native_type = selected.carriers.get_type(type);
  if (!native_type || !address || !value || LLVMTypeOf(value) != *native_type) {
    return selected.program.fail_backend(
        "LLVM cannot store a value that does not match its exact target carrier."_view);
  }

  Bool acquired = ownership == StoreOwnership::ConsumeTemporary
                      ? selected.body.acquire(type, value)
                      : selected.carriers.retain(selected.body, type, value);
  if (!acquired) {
    return False;
  }

  LLVMValueRef previous = LLVMBuildLoad2(
      selected.body.get_builder(), *native_type, address, "previous");
  if (!previous || !selected.carriers.release(selected.body, type, previous)) {
    return False;
  }

  return Bool(LLVMBuildStore(selected.body.get_builder(), value, address));
}

static auto begin_indexed_store(
    WriteSelection& selected,
    LLVMValueRef first,
    LLVMValueRef length,
    Core::Option<Count> range_size) -> Core::Option<WriteBranches> {
  Core::Option<LLVMValueRef> condition;
  if (!range_size) {
    condition = LLVMBuildICmp(
        selected.body.get_builder(), LLVMIntULT, first, length, "index.valid");
  } else {
    LLVMValueRef begins = LLVMBuildICmp(
        selected.body.get_builder(), LLVMIntULE, first, length,
        "range.start.valid");
    LLVMValueRef remaining = LLVMBuildSub(
        selected.body.get_builder(), length, first, "range.remaining");
    LLVMValueRef count =
        LLVMConstInt(LLVMTypeOf(first), U64(*range_size), LLVMBool(0));
    LLVMValueRef fits = LLVMBuildICmp(
        selected.body.get_builder(), LLVMIntULE, count, remaining,
        "range.count.valid");
    condition =
        LLVMBuildAnd(selected.body.get_builder(), begins, fits, "range.valid");
  }

  LLVMValueRef function = selected.body.get_function();
  LLVMModuleRef module = LLVMGetGlobalParent(function);
  if (!condition || !module) {
    return {};
  }

  LLVMContextRef context = LLVMGetModuleContext(module);
  LLVMBasicBlockRef write =
      LLVMAppendBasicBlockInContext(context, function, "write.selected");
  LLVMBasicBlockRef done =
      LLVMAppendBasicBlockInContext(context, function, "write.done");
  if (!write || !done ||
      !LLVMBuildCondBr(selected.body.get_builder(), *condition, write, done)) {
    return {};
  }

  LLVMPositionBuilderAtEnd(selected.body.get_builder(), write);
  return WriteBranches(done);
}

static auto end_indexed_store(
    WriteSelection& selected,
    const WriteBranches& blocks) -> Bool {
  LLVMBasicBlockRef current = LLVMGetInsertBlock(selected.body.get_builder());
  if (!current) {
    return False;
  }

  if (!LLVMGetBasicBlockTerminator(current) &&
      !LLVMBuildBr(selected.body.get_builder(), blocks.done)) {
    return False;
  }

  LLVMPositionBuilderAtEnd(selected.body.get_builder(), blocks.done);
  return True;
}

static auto get_indexed_address(
    WriteSelection& selected,
    LLVMTypeRef native_type,
    LLVMValueRef data,
    LLVMValueRef first,
    Count offset) -> Core::Option<LLVMValueRef> {
  LLVMValueRef index = first;
  if (offset != 0) {
    LLVMValueRef displacement =
        LLVMConstInt(LLVMTypeOf(first), U64(offset), LLVMBool(0));
    index = LLVMBuildAdd(
        selected.body.get_builder(), first, displacement, "write.index");
  }

  LLVMValueRef address = LLVMBuildGEP2(
      selected.body.get_builder(), native_type, data, &index, 1,
      "write.address");
  return address ? Core::Option<LLVMValueRef>(address)
                 : Core::Option<LLVMValueRef>();
}

static auto assign_to_address(
    WriteSelection& selected,
    const Ttx::Model::Pack& target,
    const Ttx::Model::Pack& source) -> Bool {
  auto address = selected.body.find_target_address(target);
  auto values = selected.body.find_values(source);
  if (!address || !values) {
    return False;
  }

  auto stored = selected.carriers.fit_and_assemble(
      selected.body, address->get_type(), source, values->get_view());
  return stored && replace_written_value(
                       selected, address->get_type(), address->get_address(),
                       *stored, StoreOwnership::ConsumeTemporary);
}

static auto assign_to_index(
    WriteSelection& selected,
    const Ttx::Model::Pack& target,
    const Ttx::Model::Pack& source) -> Bool {
  auto indexed = selected.body.find_indexed_target(target);
  auto values = selected.body.find_values(source);
  if (!indexed || !values) {
    return False;
  }

  Count count = indexed->get_range_size().visit(
      []() -> Count { return 1; },
      [](Count selected) -> Count { return selected; });
  if (values->get_size() != count) {
    return selected.program.fail_backend(
        "LLVM indexed assignment did not receive the exact selected value count."_view);
  }

  Memory::Dynamic::Vector<LLVMValueRef> stored_values(count);
  for (Count offset = 0; offset < count; offset++) {
    LLVMValueRef source_value = values->get_data()[offset];
    Core::Static::Vector<LLVMValueRef, 1> elements = {{source_value}};
    auto stored =
        indexed->get_range_size()
            ? selected.carriers.assemble(
                  selected.body, indexed->get_type(), elements.get_view())
            : selected.carriers.fit_and_assemble(
                  selected.body, indexed->get_type(), source,
                  elements.get_view());
    if (!stored) {
      return False;
    }

    stored_values.insert(*stored);
  }

  auto blocks = begin_indexed_store(
      selected, indexed->get_first(), indexed->get_length(),
      indexed->get_range_size());
  if (!blocks) {
    return False;
  }

  for (Count offset = 0; offset < count; offset++) {
    auto address = get_indexed_address(
        selected, indexed->get_native_type(), indexed->get_data(),
        indexed->get_first(), offset);
    if (!address ||
        !replace_written_value(
            selected, indexed->get_type(), *address, stored_values[offset],
            StoreOwnership::RetainTemporary)) {
      return False;
    }
  }

  return end_indexed_store(selected, *blocks);
}

static auto create_compound_result(
    WriteSelection& selected,
    const Ttx::Model::Type& type,
    LLVMValueRef left,
    LLVMValueRef right,
    Llvm::Builder::Write operation) -> LLVMValueRef {
  if (selected.carriers.is_real(type)) {
    if (operation == Llvm::Builder::Write::Subtract) {
      return LLVMBuildFSub(
          selected.body.get_builder(), left, right, "compound.value");
    }

    return LLVMBuildFAdd(
        selected.body.get_builder(), left, right, "compound.value");
  }

  if (operation == Llvm::Builder::Write::Subtract) {
    return LLVMBuildSub(
        selected.body.get_builder(), left, right, "compound.value");
  }

  return LLVMBuildAdd(
      selected.body.get_builder(), left, right, "compound.value");
}

static auto compound_to_address(
    WriteSelection& selected,
    const Ttx::Model::Pack& target,
    const Ttx::Model::Pack& right,
    Llvm::Builder::Write operation) -> Bool {
  auto address = selected.body.find_target_address(target);
  auto right_value = selected.body.find_value(right);
  if (!address || !right_value) {
    return False;
  }

  auto native_type = selected.carriers.get_type(address->get_type());
  if (!native_type || LLVMTypeOf(*right_value) != *native_type) {
    return selected.program.fail_backend(
        "LLVM compound assignment requires the right value's exact target carrier."_view);
  }

  LLVMValueRef left = LLVMBuildLoad2(
      selected.body.get_builder(), *native_type, address->get_address(),
      "compound.left");
  LLVMValueRef value = create_compound_result(
      selected, address->get_type(), left, *right_value, operation);
  return replace_written_value(
      selected, address->get_type(), address->get_address(), value,
      StoreOwnership::ConsumeTemporary);
}

static auto compound_to_index(
    WriteSelection& selected,
    const Ttx::Model::Pack& target,
    const Ttx::Model::Pack& right,
    Llvm::Builder::Write operation) -> Bool {
  auto indexed = selected.body.find_indexed_target(target);
  auto right_value = selected.body.find_value(right);
  if (!indexed || indexed->get_range_size() || !right_value ||
      LLVMTypeOf(*right_value) != indexed->get_native_type()) {
    return False;
  }

  auto blocks = begin_indexed_store(
      selected, indexed->get_first(), indexed->get_length(), {});
  auto address = blocks ? get_indexed_address(
                              selected, indexed->get_native_type(),
                              indexed->get_data(), indexed->get_first(), 0)
                        : Core::Option<LLVMValueRef>();
  if (!blocks || !address) {
    return False;
  }

  LLVMValueRef left = LLVMBuildLoad2(
      selected.body.get_builder(), indexed->get_native_type(), *address,
      "compound.left");
  LLVMValueRef value = create_compound_result(
      selected, indexed->get_type(), left, *right_value, operation);
  if (!replace_written_value(
          selected, indexed->get_type(), *address, value,
          StoreOwnership::RetainTemporary)) {
    return False;
  }

  return end_indexed_store(selected, *blocks);
}

auto Tetrodotoxin::Library::Llvm::Builder::write(
    Write operation,
    const Ttx::Model::Pack& result,
    const Ttx::Model::Pack& target,
    const Ttx::Model::Pack& source) const -> Bool {
  auto selected = select_write_target(body);
  if (!selected) {
    return False;
  }

  Bool direct = Bool(selected->body.find_target_address(target));
  Bool indexed = Bool(selected->body.find_indexed_target(target));
  if (!direct && !indexed) {
    return selected->program.fail_backend(
        "LLVM write requires one selected direct or indexed target."_view);
  }

  Bool stored = False;
  switch (operation) {
  case Write::Assign:
    stored = direct ? assign_to_address(*selected, target, source)
                    : assign_to_index(*selected, target, source);
    break;

  case Write::Add:
  case Write::Subtract:
    stored = direct ? compound_to_address(*selected, target, source, operation)
                    : compound_to_index(*selected, target, source, operation);
    break;
  }

  return stored && publish_write(selected->body, result);
}

auto Tetrodotoxin::Library::Llvm::Builder::begin_function(
    const Ttx::Model::Callable& callable,
    const Tetrodotoxin::Language::Definition& definition) const -> Bool {
  return get_program().get_debug().begin_function(body, callable, definition);
}

auto Tetrodotoxin::Library::Llvm::Builder::parameter(
    const Ttx::Model::Addressable& parameter,
    Ttx::Lexical::Anchor anchor,
    Count index) const -> Bool {
  return get_program().get_debug().parameter(body, parameter, anchor, index);
}

auto Tetrodotoxin::Library::Llvm::Builder::end_function() const -> Bool {
  return get_program().get_debug().end_function(body);
}

auto Tetrodotoxin::Library::Llvm::Builder::begin_block(
    const Ttx::Concept::Abstract& block,
    Ttx::Lexical::Anchor anchor) const -> Bool {
  return get_program().get_debug().begin_block(body, block, anchor);
}

auto Tetrodotoxin::Library::Llvm::Builder::statement(
    Ttx::Lexical::Anchor anchor) const -> Bool {
  return get_program().get_debug().statement(body, anchor);
}

auto Tetrodotoxin::Library::Llvm::Builder::local(
    const Ttx::Model::Addressable& local,
    Ttx::Lexical::Anchor anchor) const -> Bool {
  return get_program().get_debug().local(body, local, anchor);
}

auto Tetrodotoxin::Library::Llvm::Builder::has_full_debug() const -> Bool {
  return get_program().get_debug().get_level() == Llvm::Debug::Level::Full;
}

auto Tetrodotoxin::Library::Llvm::Builder::constant_local(
    const Ttx::Model::Addressable& local,
    const Ttx::Model::Pack& value,
    Ttx::Lexical::Anchor anchor) const -> Bool {
  Llvm::Program& program = get_program();
  if (program.get_debug().get_level() != Llvm::Debug::Level::Full) {
    return True;
  }

  auto values = body.find_values(value);
  if (!values) {
    return program.fail_backend(
        "LLVM lost one const Local value before debug emission."_view);
  }

  const Carriers& carriers = program.get_carriers();
  auto assembled = carriers.fit_and_assemble(
      body, local.get_type(), value, values->get_view());
  return assembled &&
         program.get_debug().value(body, local, anchor, *assembled);
}
