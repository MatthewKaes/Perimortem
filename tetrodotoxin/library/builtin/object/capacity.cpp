// Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/builtin/object/capacity.hpp"

#include "tetrodotoxin/library/llvm/builder.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin::Library;

auto Builtin::Object::Capacity::create(
    Memory::Allocator::Arena& domain,
    const Language::Model::Type& receiver,
    const Language::Model::Type& result) -> Capacity& {
  auto& self =
      Language::Parameter::create_synthetic(domain, "self"_view, receiver);
  return domain.construct_from<Capacity>(
      [&]() -> Capacity { return Capacity(self, result); });
}

auto Builtin::Object::Capacity::lower_call(
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
         body.object_capacity(
             result, *result_type, receiver->get_type(), inputs.get_data()[0]);
}
