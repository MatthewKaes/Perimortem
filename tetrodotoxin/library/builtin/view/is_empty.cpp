// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/builtin/view/is_empty.hpp"

#include "tetrodotoxin/library/language/constants/bytes.hpp"
#include "tetrodotoxin/library/language/constants/flag.hpp"
#include "tetrodotoxin/library/language/model/types/flag.hpp"
#include "tetrodotoxin/library/llvm/builder.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin::Library;

auto Builtin::View::IsEmpty::create(
    Memory::Allocator::Arena& domain,
    const Language::Model::Type& receiver,
    const Language::Model::Type& result) -> IsEmpty& {
  Language::Parameter& self =
      Language::Parameter::create_synthetic(domain, "self"_view, receiver);
  return domain.construct_from<IsEmpty>(
      [&]() -> IsEmpty { return IsEmpty(self, result); });
}

auto Builtin::View::IsEmpty::lower_call(
    Llvm::Builder& body,
    const Ttx::Model::Pack& result,
    Core::View::Vector<LLVMValueRef> inputs,
    Core::Option<const Ttx::Model::Pack&>) const -> Bool {
  auto parameter = get_parameters().get_abstract(0);
  auto receiver = parameter ? parameter->select<Ttx::Model::Addressable>()
                            : Core::Option<const Ttx::Model::Addressable&>();
  return receiver && inputs.get_size() == 1 &&
         body.contiguous_is_empty(
             result, result_type, receiver->get_type(), inputs.get_data()[0]);
}

auto Builtin::View::IsEmpty::fold_call(
    Memory::Allocator::Arena& domain,
    Core::Option<const Language::Model::Pack&> receiver,
    const Language::Model::Pack& arguments) const
    -> Core::Option<Language::Model::Pack&> {
  auto value = receiver ? receiver->select<Language::Constants::Bytes>()
                        : Core::Option<const Language::Constants::Bytes&>();
  auto flag = result_type.resolve().select<Language::Model::Types::Flag>();
  BAIL_IF(!value || !flag || !arguments.get_layout().is_empty());
  return Language::Constants::Flag::create_synthetic(
      domain, *flag, value->get_value().is_empty());
}
