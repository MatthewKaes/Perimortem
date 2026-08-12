// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/type_reference.hpp"

#include "perimortem/core/static/union.hpp"

#include "perimortem/memory/dynamic/vector.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/library/language/generic.hpp"
#include "tetrodotoxin/library/language/materializations.hpp"
#include "tetrodotoxin/library/language/model/parser/layout.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "tetrodotoxin/library/language/parser/literal.hpp"
#include "tetrodotoxin/library/language/types/composite.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/model/alias.hpp"
#include "ttx/model/layouts/fluid.hpp"

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

auto Language::TypeReference::parse(
    Tetrodotoxin::Language::Monograph& source,
    Cursor& cursor) -> Core::Option<TypeReference> {
  auto library_source = source.select<Language::Monograph>();
  BAIL_IF(!library_source);

  auto transaction = cursor.branch();
  auto route = parse_route(transaction);
  BAIL_IF(!route);

  if (!transaction.matches(Code::Type::BracketStart)) {
    cursor.join(transaction);
    return *route;
  }

  auto& domain = transaction.get_arena();
  Memory::Managed::Vector<Argument> arguments(domain);
  auto closing = Model::Parser::Layout::parse_entries(
      transaction, Code::Type::BracketStart, Code::Type::BracketEnd,
      [&](Cursor& entry, Count) -> Bool {
        if (entry.matches(Code::Type::Type)) {
          auto nested = parse(source, entry);
          BAIL_IF(!nested);

          // A nested route is retained once in the same Arena as this authored
          // shape. The Layout parser owns punctuation while TypeReference keeps
          // the delayed source edge required by recursive linking.
          const TypeReference& retained =
              domain.construct<TypeReference>(*nested);
          arguments.insert(Argument(retained));
          return True;
        }

        switch (entry.current().get_code().get_type()) {
        case Code::Type::Numeric:
        case Code::Type::Hex:
        case Code::Type::Float:
        case Code::Type::String:
        case Code::Type::Bytes:
        case Code::Type::Embedded:
        case Code::Type::True:
        case Code::Type::False:
          break;
        default:
          entry.create_token_error(
              "Library Generic Layout entries require a Type reference or "
              "literal."_view);
          return False;
        }

        // Literal owns the complete concrete literal grammar and diagnostics.
        // TypeReference only distinguishes that real semantic edge from a
        // nested delayed Type route.
        auto literal = Parser::Literal::parse(domain, *library_source, entry);
        BAIL_IF(!literal);
        arguments.insert(Argument(*literal));
        return True;
      });
  BAIL_IF(!closing);

  TypeReference completed(
      route->tokens, route->names,
      Anchor::create(
          route->get_anchor().get_token(),
          Span(route->get_anchor().get_token(), *closing)),
      arguments.get_view());
  cursor.join(transaction);
  return completed;
}

auto Language::TypeReference::parse_route(Cursor& cursor)
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

auto Language::TypeReference::matches_route(const TypeReference& other) const
    -> Bool {
  if (get_size() != other.get_size()) {
    return False;
  }

  for (Count i = 0; i < get_size(); i++) {
    if (get_name(i) != other.get_name(i)) {
      return False;
    }
  }
  return True;
}

auto Language::TypeReference::get_argument_reference(Count index) const
    -> Core::Option<const TypeReference&> {
  BAIL_IF(!arguments || index >= arguments->get_size());
  const TypeReference* reference =
      arguments->get_data()[index].find<const TypeReference&>();
  BAIL_IF(reference == nullptr);
  return *reference;
}

auto Language::TypeReference::resolve_route(const Abstract& context) const
    -> const Abstract& {
  const Abstract& root = resolve_alias(context.resolve_context(get_root()));
  return resolve_type_reference(*this, root, {});
}

auto Language::TypeReference::resolve_route_from(const Abstract& root) const
    -> const Abstract& {
  return resolve_type_reference(*this, root, {});
}

auto Language::TypeReference::resolve_route_from(
    const Abstract& root,
    const Ttx::Model::Type& caller_scope) const -> const Abstract& {
  return resolve_type_reference(*this, root, caller_scope);
}

auto Language::TypeReference::resolve_selected(
    const Abstract& selected,
    const Ttx::Model::Type& caller_scope,
    Bool exported) const -> const Abstract& {
  if (!arguments) {
    return selected.visit<Ttx::Model::Type>(
        [](const Ttx::Model::Type& type) -> const Abstract& { return type; },
        [](const Abstract&) -> const Abstract& {
          return Invalid::get_invalid();
        });
  }

  return selected.visit<Generic>(
      [&](const Generic& generic) -> const Abstract& {
        auto scope = caller_scope.select<Types::Composite>();
        if (!scope) {
          return Invalid::get_invalid();
        }

        // Resolution needs one transient real Layout. Materializations copies
        // the normalized semantic key into its Arena before this local storage
        // leaves; TypeReference retains neither a cache nor a mutable link
        // buffer. Every nested route keeps the original caller authority.
        Memory::Dynamic::Vector<Reference<const Abstract>> linked(
            arguments->get_size());
        const auto* argument_data = arguments->get_data();
        for (Count i = 0; i < arguments->get_size(); i++) {
          const Argument& argument = argument_data[i];
          const TypeReference* reference =
              argument.find<const TypeReference&>();
          if (reference != nullptr) {
            const Abstract& nested =
                exported ? scope->resolve_exported_type(*reference)
                         : scope->resolve_type(*reference);
            const Abstract& resolved = resolve_alias(nested);
            if (!resolved.is<Ttx::Model::Type>()) {
              return Invalid::get_invalid();
            }
            linked.insert(resolved);
            continue;
          }

          const Abstract* literal = argument.find<const Abstract&>();
          if (literal == nullptr) {
            return Invalid::get_invalid();
          }
          linked.insert(*literal);
        }

        Ttx::Model::Layouts::Fluid layout(linked.get_view());
        const Abstract* host = &caller_scope;
        while (auto composite = host->select<Types::Composite>()) {
          host = &composite->get_host();
        }

        auto library_source = host->select<Language::Monograph>();
        if (!library_source) {
          return Invalid::get_invalid();
        }
        auto& materializations = library_source->get_materializations();
        auto materialized = materializations.materialize(generic, layout);
        return materialized.visit(
            []() -> const Abstract& { return Invalid::get_invalid(); },
            [](const Ttx::Model::Type& type) -> const Abstract& {
              return type;
            });
      },
      [](const Abstract&) -> const Abstract& {
        return Invalid::get_invalid();
      });
}

auto Language::TypeReference::resolve_type(
    const Abstract& selected,
    const Ttx::Model::Type& caller_scope) const -> const Abstract& {
  return resolve_selected(selected, caller_scope, False);
}

auto Language::TypeReference::resolve_exported_type(
    const Abstract& selected,
    const Ttx::Model::Type& caller_scope) const -> const Abstract& {
  return resolve_selected(selected, caller_scope, True);
}
