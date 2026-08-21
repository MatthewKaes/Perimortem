// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/access/type.hpp"

#include "ttx/concept/invalid.hpp"

using namespace Perimortem;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin::Library;

auto Language::Access::Type::parse(
    const Abstract&,
    Cursor& cursor,
    Expression& receiver) -> Core::Option<Expression&> {
  Memory::Allocator::Arena& domain = cursor.get_arena();
  Token operation = cursor.consume();
  Token type = cursor.require(
      Code::Type::Type, "Type access requires one Type name after `::`."_view);
  BAIL_IF(!type);

  auto receiver_anchor = receiver.get_anchor();
  if (!receiver_anchor) {
    cursor.create_expression_error(
        Anchor::create(type, Span(operation, type)),
        "Type access requires an authored receiver Anchor."_view);
    return {};
  }

  Core::View::Bytes name = type.caculate_text(cursor.get_source_text());
  Anchor anchor = Anchor::create(type, receiver_anchor->get_span(), Span(type));
  Type& access = Expression::create_authored<Type>(
      domain, anchor, [&](Core::Option<Anchor> source) -> Type {
        return Type(receiver, type, name, source);
      });
  return access;
}

auto Language::Access::Type::link(
    Ttx::Lexical::Cursor& cursor,
    const Abstract& lexical_context,
    Core::Option<const Abstract&> access_scope) -> Bool {
  BAIL_IF(!receiver.link(cursor, lexical_context, access_scope));

  const Abstract& receiver_result = receiver.get_result();
  const Abstract& binding = receiver_result.resolve_context(name).resolve();
  // Package Source Aliases retain their Monograph as promised. A source may
  // publish one matching root Type under that authored route. Expression Type
  // access selects that Type while declaration and using queries still observe
  // the real Monograph binding.
  const Abstract& nested = binding.resolve_context(name).resolve();
  const Abstract& result =
      nested.is<Language::Model::Type>() ? nested : binding;
  if (result.is<Invalid>()) {
    cursor.create_expression_error(
        get_anchor(), "Type access did not select a semantic context."_view,
        "Publish the named context or Type on the receiver before linking this access."_view);
    return False;
  }

  if (selected && &selected->get() != &result) {
    cursor.create_expression_error(
        get_anchor(), "Type access cannot change its selected result."_view,
        "Keep one exact Type bound to this authored Token."_view);
    return False;
  }

  selected = Reference<const Abstract>(result);
  return True;
}

auto Language::Access::Type::get_documentation() const -> const Documentation& {
  return selected.visit(
      []() -> const Documentation& { return Documentation::get_empty(); },
      [](const Reference<const Abstract>& selected) -> const Documentation& {
        return selected.get().get_documentation();
      });
}

auto Language::Access::Type::get_type() const -> const Abstract& {
  return Invalid::get_invalid();
}

auto Language::Access::Type::get_result() const -> const Abstract& {
  return selected.visit(
      []() -> const Abstract& { return Invalid::get_invalid(); },
      [](const Reference<const Abstract>& selected) -> const Abstract& {
        return selected.get();
      });
}

auto Language::Access::Type::finalize(Cursor& cursor) -> void {
  receiver.finalize(cursor);
  Expression::finalize(cursor);
}

auto Language::Access::Type::lower(Llvm::Builder&) const -> Bool {
  return True;
}
