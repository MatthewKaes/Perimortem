// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/type_reference.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/library/language/types/composite.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/concept/reference.hpp"
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

static auto resolve_type_reference(
    const Language::TypeReference& reference,
    const Abstract& root,
    Core::Option<const Ttx::Model::Type&> caller_scope) -> const Abstract& {
  Reference<const Abstract> selected(resolve_alias(root));
  for (Count i = 1; i < reference.get_size(); i++) {
    if (selected.get().is<Invalid>()) {
      return selected.get();
    }

    Core::View::Bytes name = reference.get_name(i);
    const Abstract& next = selected.get().visit<Language::Types::Composite>(
        [&](const Language::Types::Composite& composite) -> const Abstract& {
          return caller_scope.visit(
              [&]() -> const Abstract& {
                return composite.resolve_context(name);
              },
              [&](const Ttx::Model::Type& caller) -> const Abstract& {
                return composite.resolve_type(name, caller);
              });
        },
        [&](const Abstract& context) -> const Abstract& {
          return context.resolve_context(name);
        });

    // Alias is deliberately opaque to TypeReference. Resolution is its only
    // semantic operation, and therefore the only way a qualified edge can
    // reveal the identity that owns the following segment.
    selected = Reference<const Abstract>(resolve_alias(next));
  }

  return selected.get();
}

auto Language::TypeReference::parse(Cursor& cursor)
    -> Core::Option<TypeReference> {
  auto& domain = cursor.get_arena();
  Memory::Managed::Vector<Token> tokens(domain);
  Memory::Managed::Vector<Core::View::Bytes> names(domain);

  Token first = cursor.require(
      Code::Type::Type, "Library Type reference requires one Type name."_view);
  BAIL_IF(!first);

  auto retain = [&](Token token) {
    tokens.insert(token);
    names.insert(domain.proxy(token.caculate_text(cursor.get_source_text())));
  };
  retain(first);

  Token last = first;
  while (cursor.matches(Code::Type::TypeAccessOp)) {
    Token separator = cursor.current();
    Count previous_end = Count(last.get_offset()) + Count(last.get_size());
    if (separator.get_offset() != previous_end) {
      cursor.create_expression_error(
          Span(first, separator),
          "Library Type references cannot contain whitespace around `::`."_view);
      return {};
    }

    cursor.consume();
    Token segment = cursor.require(
        Code::Type::Type,
        "Library Type reference requires a Type after `::`."_view);
    BAIL_IF(!segment);

    Count separator_end =
        Count(separator.get_offset()) + Count(separator.get_size());
    if (segment.get_offset() != separator_end) {
      cursor.create_expression_error(
          Span(first, segment),
          "Library Type references cannot contain whitespace around `::`."_view);
      return {};
    }

    retain(segment);
    last = segment;
  }

  return TypeReference(
      tokens.get_view(), names.get_view(),
      Anchor::create(first, Span(first, last)));
}

auto Language::TypeReference::resolve(const Abstract& context) const
    -> const Abstract& {
  const Abstract& root = resolve_alias(context.resolve_context(get_root()));
  return resolve_type_reference(*this, root, {});
}

auto Language::TypeReference::resolve_from(const Abstract& root) const
    -> const Abstract& {
  return resolve_type_reference(*this, root, {});
}

auto Language::TypeReference::resolve_from(
    const Abstract& root,
    const Ttx::Model::Type& caller_scope) const -> const Abstract& {
  return resolve_type_reference(*this, root, caller_scope);
}
