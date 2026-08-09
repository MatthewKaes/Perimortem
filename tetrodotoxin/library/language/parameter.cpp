// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/parameter.hpp"

using namespace Ttx::Concept;
using namespace Tetrodotoxin::Library;

auto Language::Parameter::create_authored(
    Perimortem::Memory::Allocator::Arena& domain,
    const Signature& signature,
    Count index,
    const Ttx::Model::Type& type) -> Perimortem::Core::Option<Parameter&> {
  auto linked_type = signature.get_parameter_type(index);
  auto source = signature.get_parameter_anchor(index);
  if (!linked_type || &*linked_type != &type || !source ||
      signature.get_parameter_name(index).is_empty()) {
    return {};
  }

  return domain.construct_from<Parameter>(
      [&]() -> Parameter { return Parameter(signature, index, type); });
}

auto Language::Parameter::get_documentation() const -> const Documentation& {
  return Documentation::get_empty();
}

auto Language::Parameter::get_type() const -> const Ttx::Model::Type& {
  return type;
}

auto Language::Parameter::get_name_token() const -> Ttx::Lexical::Token {
  return signature.get_parameter_anchor(index).visit(
      []() -> Ttx::Lexical::Token { return {}; },
      [](const Ttx::Lexical::Anchor& anchor) { return anchor.get_token(); });
}

auto Language::Parameter::get_span() const -> Ttx::Lexical::Span {
  return signature.get_parameter_anchor(index).visit(
      []() -> Ttx::Lexical::Span { return {}; },
      [](const Ttx::Lexical::Anchor& anchor) { return anchor.get_span(); });
}

auto Language::Parameter::get_type_span() const -> Ttx::Lexical::Span {
  return signature.get_parameter_type_anchor(index).visit(
      []() -> Ttx::Lexical::Span { return {}; },
      [](const Ttx::Lexical::Anchor& anchor) { return anchor.get_span(); });
}
