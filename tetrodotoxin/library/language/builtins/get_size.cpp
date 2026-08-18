// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/builtins/get_size.hpp"

#include "tetrodotoxin/library/llvm/builder.hpp"
using namespace Perimortem;
using namespace Tetrodotoxin::Library;

auto Language::Builtins::GetSize::create(
    Memory::Allocator::Arena& domain,
    const Language::Model::Type& receiver,
    const Language::Model::Type& result) -> GetSize& {
  Language::Parameter& self =
      Language::Parameter::create_synthetic(domain, "self"_view, receiver);
  return domain.construct_from<GetSize>(
      [&]() -> GetSize { return GetSize(self, result); });
}

auto Language::Builtins::GetSize::reserve_declaration(
    Llvm::Program& program) const -> Bool {
  const auto& functions = program.get_functions();
  auto reserved = functions.reserve_get_size(program, *this);
  if (!reserved) {
    return False;
  }

  return !*reserved || Model::Callable::reserve_declaration(program);
}

auto Language::Builtins::GetSize::complete_declaration(
    Llvm::Program& program) const -> Bool {
  const auto& functions = program.get_functions();
  Bool signature_completed = Model::Callable::complete_declaration(program);
  return signature_completed && functions.complete(program, *this);
}

auto Language::Builtins::GetSize::lower_call(
    Llvm::Builder& body,
    const Ttx::Model::Pack& result,
    Core::View::Vector<LLVMValueRef> inputs,
    Core::Option<const Ttx::Model::Pack&>) const -> Bool {
  auto result_type = get_results().get_abstract(0).visit(
      []() -> Core::Option<const Ttx::Model::Type&> { return {}; },
      [](const Ttx::Concept::Abstract& selected)
          -> Core::Option<const Ttx::Model::Type&> {
        return selected.resolve().select<Ttx::Model::Type>();
      });
  auto parameter = get_parameters().get_abstract(0);
  auto receiver = parameter ? parameter->select<Ttx::Model::Addressable>()
                            : Core::Option<const Ttx::Model::Addressable&>();
  return result_type && receiver && inputs.get_size() == 1 &&
         body.get_size(
             result, *result_type, receiver->get_type(), inputs.get_data()[0]);
}
