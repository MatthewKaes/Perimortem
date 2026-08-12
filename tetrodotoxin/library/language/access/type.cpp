// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/access/type.hpp"

#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/language/types/composite.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/model/alias.hpp"

using namespace Perimortem;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin::Library;

static auto resolve_alias(const Abstract& binding) -> const Abstract& {
  return binding.visit<Ttx::Model::Alias>(
      [](const Ttx::Model::Alias& alias) -> const Abstract& {
        return alias.resolve();
      },
      [](const Abstract& direct) -> const Abstract& { return direct; });
}

auto Language::Access::Type::parse(
    Memory::Allocator::Arena& domain,
    Language::Monograph&,
    Cursor& cursor,
    Expression& receiver) -> Core::Option<Expression&> {
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

  Core::View::Bytes name =
      domain.proxy(type.caculate_text(cursor.get_source_text()));
  Anchor anchor = Anchor::create(type, receiver_anchor->get_span(), Span(type));
  Type& access = Expression::create_authored<Type>(
      domain, anchor, [&](Core::Option<Anchor> source) -> Type {
        return Type(receiver, type, name, source);
      });
  return access;
}

auto Language::Access::Type::link(
    Tetrodotoxin::Language::Monograph& source,
    const Abstract& lexical_context,
    Core::Option<const Ttx::Model::Type&> access_scope) -> Bool {
  BAIL_IF(!receiver.link(source, lexical_context, access_scope));

  const Abstract& receiver_result = receiver.get_result();
  auto receiver_type = receiver_result.select<Ttx::Model::Type>();
  if (!receiver_type) {
    source.report(
        get_anchor(), "Type access receiver did not produce a Type."_view,
        "Use `::` only after an Expression whose result is a semantic Type."_view);
    return False;
  }

  const Abstract& candidate = receiver_type->visit<Language::Types::Composite>(
      [&](const Language::Types::Composite& composite) -> const Abstract& {
        return access_scope.visit(
            [&]() -> const Abstract& {
              return composite.resolve_context(name);
            },
            [&](const Ttx::Model::Type& caller) -> const Abstract& {
              return composite.resolve_type(name, caller);
            });
      },
      [&](const Abstract& type) -> const Abstract& {
        return type.resolve_context(name);
      });
  const Abstract& resolved = resolve_alias(candidate);
  auto result = resolved.select<Ttx::Model::Type>();
  if (!result) {
    source.report(
        get_anchor(), "Type access did not select one semantic Type."_view,
        "Publish the named Type on the receiver before linking this access."_view);
    return False;
  }

  if (selected && &selected->get() != &*result) {
    source.report(
        get_anchor(), "Type access cannot change its selected result."_view,
        "Keep one exact Type bound to this authored Token."_view);
    return False;
  }

  selected = Reference<const Ttx::Model::Type>(*result);
  return True;
}

auto Language::Access::Type::get_documentation() const -> const Documentation& {
  return selected.visit(
      []() -> const Documentation& { return Documentation::get_empty(); },
      [](const Reference<const Ttx::Model::Type>& type)
          -> const Documentation& { return type.get().get_documentation(); });
}

auto Language::Access::Type::get_type() const -> const Abstract& {
  return selected.visit(
      []() -> const Abstract& { return Invalid::get_invalid(); },
      [](const Reference<const Ttx::Model::Type>&) -> const Abstract& {
        return Dialect::get_descriptor();
      });
}

auto Language::Access::Type::get_result() const -> const Abstract& {
  return selected.visit(
      []() -> const Abstract& { return Invalid::get_invalid(); },
      [](const Reference<const Ttx::Model::Type>& type) -> const Abstract& {
        return type.get();
      });
}

auto Language::Access::Type::finalize() -> void {
  receiver.finalize();
  Expression::finalize();
}
