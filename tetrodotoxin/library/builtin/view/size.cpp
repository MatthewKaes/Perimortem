// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/builtin/view/size.hpp"

#include "tetrodotoxin/library/llvm/builder.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin::Library;

auto Builtin::View::Size::create(
    Memory::Allocator::Arena& domain,
    const Language::Model::Type& receiver,
    const Language::Model::Type& result) -> Size& {
  Language::Parameter& self =
      Language::Parameter::create_synthetic(domain, "self"_view, receiver);
  return domain.construct_from<Size>(
      [&]() -> Size { return Size(self, result); });
}

auto Builtin::View::Size::lower_call(
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
