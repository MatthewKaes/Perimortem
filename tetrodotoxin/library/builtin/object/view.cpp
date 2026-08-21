// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/builtin/object/view.hpp"

#include "tetrodotoxin/library/llvm/builder.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin::Library;

auto Builtin::Object::View::create(
    Memory::Allocator::Arena& domain,
    const Language::Model::Type& receiver,
    const Language::Model::Type& result) -> View& {
  auto& self =
      Language::Parameter::create_synthetic(domain, "self"_view, receiver);
  return domain.construct_from<View>(
      [&]() -> View { return View(self, result); });
}

auto Builtin::Object::View::lower_call(
    Llvm::Builder& body,
    const Ttx::Model::Pack& result,
    Core::View::Vector<LLVMValueRef> inputs,
    Core::Option<const Ttx::Model::Pack&>) const -> Bool {
  auto parameter = get_parameters().get_abstract(0);
  auto receiver = parameter ? parameter->select<Ttx::Model::Addressable>()
                            : Core::Option<const Ttx::Model::Addressable&>();
  return receiver && inputs.get_size() == 1 &&
         body.object_view(
             result, result_type, receiver->get_type(), inputs.get_data()[0]);
}
