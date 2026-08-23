// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/access/type.hpp"

#include "tetrodotoxin/library/language/expressions/identifier.hpp"
#include "ttx/concept/invalid.hpp"

using namespace Perimortem;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin::Library;

static auto resolve_receiver(const Language::Expression& receiver)
    -> const Abstract& {
  const Abstract& completed = receiver.get_result();
  if (!completed.is<Invalid>()) {
    return completed;
  }

  auto identifier = receiver.select<Language::Expressions::Identifier>();
  if (identifier) {
    return identifier->resolve_authored();
  }

  auto access = receiver.select<Language::Access::Type>();
  return access ? access->resolve_authored() : Invalid::get_invalid();
}

static auto select_type_access(const Abstract& receiver, Core::View::Bytes name)
    -> const Abstract& {
  const Abstract& binding = receiver.resolve_context(name).resolve();
  // Package Source Aliases retain their Monograph as promised. A source may
  // publish one matching root Type under that authored route. Expression Type
  // access selects that Type while declaration and using queries still observe
  // the real Monograph binding.
  const Abstract& nested = binding.resolve_context(name).resolve();
  return nested.is<Language::Model::Type>() ? nested : binding;
}

auto Language::Access::Type::create_authored(
    Memory::Allocator::Arena& domain,
    Expression& receiver,
    Token token,
    Core::View::Bytes name,
    Anchor anchor) -> Type& {
  return Expression::create_authored<Type>(
      domain, anchor, [&](Core::Option<Anchor> source) -> Type {
        return Type(receiver, token, name, source);
      });
}

auto Language::Access::Type::link(
    Ttx::Lexical::Cursor& cursor,
    const Abstract& lexical_context,
    Core::Option<const Abstract&> access_scope) -> Bool {
  BAIL_IF(!receiver.link(cursor, lexical_context, access_scope));

  const Abstract& receiver_result = receiver.get_result();
  const Abstract& result = select_type_access(receiver_result, name);
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

auto Language::Access::Type::resolve_authored() const -> const Abstract& {
  if (selected) {
    return selected->get();
  }

  const Abstract& receiver_result = resolve_receiver(receiver);
  return receiver_result.is<Invalid>()
             ? static_cast<const Abstract&>(Invalid::get_invalid())
             : select_type_access(receiver_result, name);
}

auto Language::Access::Type::finalize(Cursor& cursor) -> void {
  receiver.finalize(cursor);
  Expression::finalize(cursor);
}
