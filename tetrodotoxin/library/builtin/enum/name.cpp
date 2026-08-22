// Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/builtin/enum/name.hpp"

#include "perimortem/memory/dynamic/vector.hpp"

#include "tetrodotoxin/library/language/constants/bytes.hpp"
#include "tetrodotoxin/library/language/constants/enumeration.hpp"
#include "tetrodotoxin/library/llvm/builder.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin::Library;

auto Builtin::Enum::Name::create(
    Memory::Allocator::Arena& domain,
    const Language::Types::Enumeration& enumeration,
    const Language::Model::Type& result) -> Name& {
  Language::Parameter& self =
      Language::Parameter::create_synthetic(domain, "self"_view, enumeration);
  return domain.construct_from<Name>(
      [&]() -> Name { return Name(self, enumeration, result); });
}

auto Builtin::Enum::Name::lower_call(
    Llvm::Builder& body,
    const Ttx::Model::Pack& result,
    Core::View::Vector<LLVMValueRef> inputs,
    Core::Option<const Ttx::Model::Pack&>) const -> Bool {
  BAIL_IF(inputs.get_size() != 1);

  Memory::Dynamic::Vector<U64> values;
  Memory::Dynamic::Vector<Core::View::Bytes> names;
  values.resize(enumeration.get_cases().get_size());
  names.resize(enumeration.get_cases().get_size());
  for (Count index = 0; index < enumeration.get_cases().get_size(); index++) {
    auto value = enumeration.get_case_value(index);
    BAIL_IF(!value);
    values[index] = *value;
    names[index] = enumeration.get_case_name(index);
  }

  return body.enumeration_name(
      result, result_type, inputs[0], values.get_view(), names.get_view());
}

auto Builtin::Enum::Name::fold_call(
    Memory::Allocator::Arena& domain,
    Core::Option<const Language::Model::Pack&> receiver,
    const Language::Model::Pack& arguments) const
    -> Core::Option<Language::Model::Pack&> {
  BAIL_IF(!receiver || !arguments.get_layout().is_empty());

  auto constant = receiver->select<Language::Constants::Enumeration>();
  BAIL_IF(!constant || &constant->get_type() != &enumeration);
  Core::View::Bytes name = enumeration.find_case_name(constant->get_value());
  return Language::Constants::Bytes::create_synthetic(
      domain, result_type, name);
}
