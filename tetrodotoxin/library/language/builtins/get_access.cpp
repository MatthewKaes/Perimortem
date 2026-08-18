// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/builtins/get_access.hpp"

#include "tetrodotoxin/library/llvm/builder.hpp"
using namespace Perimortem;
using namespace Ttx::Concept;
using namespace Tetrodotoxin::Library;

auto Language::Builtins::GetAccess::create(
    Memory::Allocator::Arena& domain,
    const Language::Model::Type& receiver,
    const Language::Model::Type& result) -> GetAccess& {
  Language::Parameter& self =
      Language::Parameter::create_synthetic(domain, "self"_view, receiver);
  return domain.construct_from<GetAccess>(
      [&]() -> GetAccess { return GetAccess(self, result); });
}

auto Language::Builtins::GetAccess::accepts_receiver(
    const Abstract& receiver,
    const Abstract& host) const -> Bool {
  auto addressable = receiver.resolve().select<Language::Model::Addressable>();
  auto access_scope = host.resolve().select<Language::Model::Type>();
  return addressable && access_scope &&
         addressable->permits_write_from(*access_scope);
}

auto Language::Builtins::GetAccess::reserve_declaration(
    Llvm::Program& program) const -> Bool {
  const auto& functions = program.get_functions();
  auto reserved = functions.reserve_get_access(program, *this);
  if (!reserved) {
    return False;
  }

  return !*reserved || Model::Callable::reserve_declaration(program);
}

auto Language::Builtins::GetAccess::complete_declaration(
    Llvm::Program& program) const -> Bool {
  const auto& functions = program.get_functions();
  Bool signature_completed = Model::Callable::complete_declaration(program);
  return signature_completed && functions.complete(program, *this);
}

auto Language::Builtins::GetAccess::lower_call(
    Llvm::Builder& body,
    const Ttx::Model::Pack& result,
    Core::View::Vector<LLVMValueRef> inputs,
    Core::Option<const Ttx::Model::Pack&> receiver_source) const -> Bool {
  auto result_type = get_results().get_abstract(0).visit(
      []() -> Core::Option<const Ttx::Model::Type&> { return {}; },
      [](const Ttx::Concept::Abstract& selected)
          -> Core::Option<const Ttx::Model::Type&> {
        return selected.resolve().select<Ttx::Model::Type>();
      });
  auto parameter = get_parameters().get_abstract(0);
  auto receiver = parameter ? parameter->select<Ttx::Model::Addressable>()
                            : Core::Option<const Ttx::Model::Addressable&>();
  return result_type && receiver && receiver_source && inputs.get_size() == 1 &&
         body.get_access(
             result, *result_type, receiver->get_type(), *receiver_source,
             inputs.get_data()[0]);
}
