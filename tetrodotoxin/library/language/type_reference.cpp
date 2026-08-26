// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/type_reference.hpp"

#include "perimortem/core/static/union.hpp"

#include "perimortem/memory/dynamic/vector.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/library/language/constants/false.hpp"
#include "tetrodotoxin/library/language/constants/signed.hpp"
#include "tetrodotoxin/library/language/constants/true.hpp"
#include "tetrodotoxin/library/language/constants/unsigned.hpp"
#include "tetrodotoxin/library/language/expression.hpp"
#include "tetrodotoxin/library/language/generic.hpp"
#include "tetrodotoxin/library/language/model/type.hpp"
#include "tetrodotoxin/library/language/model/types/flag.hpp"
#include "tetrodotoxin/library/language/model/types/signed.hpp"
#include "tetrodotoxin/library/language/model/types/unsigned.hpp"
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

auto Language::TypeReference::get_size() const -> Count {
  if (route.is_empty()) {
    return 0;
  }

  Count segments = 1;
  for (Count index = 0; index + 1 < route.get_size(); index++) {
    if (route[index] == ':' && route[index + 1] == ':') {
      segments++;
      index++;
    }
  }
  return segments;
}

auto Language::TypeReference::get_name(Count requested) const
    -> Core::View::Bytes {
  Count segment = 0;
  Count start = 0;
  for (Count index = 0; index <= route.get_size(); index++) {
    Bool end = index == route.get_size();
    Bool separator = !end && index + 1 < route.get_size() &&
                     route[index] == ':' && route[index + 1] == ':';
    if (!end && !separator) {
      continue;
    }
    if (segment == requested) {
      return route.slice(start, index - start);
    }
    if (end) {
      return {};
    }
    segment++;
    index++;
    start = index + 1;
  }

  return {};
}

auto Language::TypeReference::get_token(Count requested) const -> Token {
  if (requested + 1 == get_size() && terminal) {
    return terminal;
  }
  Core::View::Bytes name = get_name(requested);
  Token first = anchor.get_token();
  if (!first || name.is_empty()) {
    return {};
  }

  Count offset = Count(name.get_data() - route.get_data());
  return Token(
      U16(Count(first.get_offset()) + offset), first.get_line(),
      U16(Count(first.get_column()) + offset), U8(name.get_size()),
      first.get_code());
}

auto Language::TypeReference::matches_route(const TypeReference& other) const
    -> Bool {
  return route == other.route;
}

auto Language::TypeReference::get_argument_reference(Count index) const
    -> Core::Option<const TypeReference&> {
  BAIL_IF(!arguments || index >= arguments->get_size());
  const TypeReference* reference =
      arguments->get_data()[index].find<const TypeReference&>();
  BAIL_IF(!reference);
  return *reference;
}

auto Language::TypeReference::get_argument(Count index) const
    -> Core::Option<const Argument&> {
  BAIL_IF(!arguments || index >= arguments->get_size());
  return arguments->get_data()[index];
}

static auto map_failure(
    Anchor anchor,
    const Language::Generic::Failure& failure)
    -> Language::TypeReference::Failure {
  switch (failure.get_type()) {
  case Language::Generic::Failure::Type::Unavailable:
    return Language::TypeReference::Failure(
        Language::TypeReference::Failure::Type::Unavailable, anchor);
  case Language::Generic::Failure::Type::Arity:
    return Language::TypeReference::Failure(
        Language::TypeReference::Failure::Type::Arity, anchor);
  case Language::Generic::Failure::Type::Parameter:
    return Language::TypeReference::Failure(
        Language::TypeReference::Failure::Type::Parameter, anchor,
        failure.get_argument());
  case Language::Generic::Failure::Type::Recursive:
    return Language::TypeReference::Failure(
        Language::TypeReference::Failure::Type::Recursive, anchor);
  case Language::Generic::Failure::Type::Formula:
    return Language::TypeReference::Failure(
        Language::TypeReference::Failure::Type::Formula, anchor);
  }

  return Language::TypeReference::Failure(
      Language::TypeReference::Failure::Type::Formula, anchor);
}

auto Language::TypeReference::resolve_with_root(
    const Abstract& context,
    Root root,
    Core::Option<Cursor&> cursor) const -> Resolution {
  // The declaration context gives the root name its lexical authority. Each
  // explicit suffix then asks the identity selected by the preceding segment.
  const Abstract* selected = &context.resolve_context(get_root());
  if (root == Root::Lexical) {
    auto type = context.select<Language::Model::Type>();
    if (type) {
      selected = &type->resolve_lexical_context(get_root());
    }
  }
  if (selected->is<Invalid>()) {
    return Failure(Failure::Type::Route, anchor, 0);
  }
  if (cursor && get_size() > 1) {
    Token token = get_token(0);
    cursor->get_associations().create(
        Anchor::create(token, Span(token)), *selected);
  }

  for (Count i = 1; i < get_size(); i++) {
    // Alias resolution reveals the identity that can answer the next ordinary
    // context query. Keeping that step visible also preserves Alias opacity for
    // every other consumer.
    const Abstract& route_context = resolve_alias(*selected);
    if (route_context.is<Invalid>()) {
      return Failure(Failure::Type::Route, anchor, i - 1);
    }

    selected = &route_context.resolve_context(get_name(i));
    if (selected->is<Invalid>()) {
      return Failure(Failure::Type::Route, anchor, i);
    }
    if (cursor && i + 1 < get_size()) {
      Token token = get_token(i);
      cursor->get_associations().create(
          Anchor::create(token, Span(token)), *selected);
    }
  }

  if (!arguments) {
    const Abstract& direct = resolve_alias(*selected);
    const Abstract& resolved = direct;
    if (resolved.is<Invalid>()) {
      return Failure(Failure::Type::Route, anchor, get_size() - 1);
    }

    if (cursor) {
      Token token = get_token(get_size() - 1);
      cursor->get_associations().create(
          Anchor::create(token, Span(token)), *selected);
    }
    return resolved;
  }

  const Abstract& resolved = resolve_alias(*selected);
  if (resolved.is<Invalid>()) {
    return Failure(Failure::Type::Route, anchor, get_size() - 1);
  }
  auto generic = resolved.select<Generic>();
  if (!generic) {
    return Failure(Failure::Type::Generic, anchor);
  }
  if (cursor) {
    // The authored name still denotes the Generic even though applying its
    // arguments returns a materialized Type. Recording the terminal Token lets
    // editor tooling show that distinction with the same identity selected by
    // resolution.
    Token token = get_token(get_size() - 1);
    cursor->get_associations().create(
        Anchor::create(token, Span(token)), *generic);
  }

  // Resolution assembles one temporary Layout from the real argument
  // identities. Generic copies its normalized key into its own Arena before
  // this storage leaves, and nested routes follow the same root access policy.
  Memory::Dynamic::Vector<Reference<const Abstract>> linked(
      arguments->get_size());
  const auto* argument_data = arguments->get_data();
  for (Count i = 0; i < arguments->get_size(); i++) {
    const Argument& argument = argument_data[i];
    const TypeReference* reference = argument.find<const TypeReference&>();
    if (reference) {
      Core::Option<const Abstract&> nested;
      Core::Option<Failure> nested_failure;
      reference->resolve_with_root(context, root, cursor)
          .visit(
              [&](const Abstract& resolved) {
                nested = resolve_alias(resolved);
              },
              [&](const Failure& failure) { nested_failure = failure; });
      if (nested_failure) {
        return *nested_failure;
      }
      if (!nested || !nested->is<Ttx::Model::Type>()) {
        return Failure(Failure::Type::Argument, anchor, i);
      }
      linked.insert(*nested);
      continue;
    }

    const Abstract* literal = argument.find<const Abstract&>();
    if (!literal) {
      return Failure(Failure::Type::Argument, anchor, i);
    }
    linked.insert(*literal);
  }

  Ttx::Model::Layouts::Fluid layout(linked.get_view());
  return generic->materialize(layout).visit(
      [&](const Language::Model::Type& type) -> Resolution {
        if (cursor) {
          cursor->get_associations().create(anchor, type);
        }
        return type;
      },
      [&](const Generic::Failure& failure) -> Resolution {
        // Generic knows which formula parameter failed, while TypeReference
        // knows where that argument was written. Joining those facts gives the
        // diagnostic the right authored Anchor.
        Anchor failure_anchor = anchor;
        Count index = failure.get_argument();
        if (failure.get_type() == Generic::Failure::Type::Parameter &&
            index < arguments->get_size()) {
          const Argument& argument = arguments->get_data()[index];
          const TypeReference* reference =
              argument.find<const TypeReference&>();
          if (reference) {
            failure_anchor = reference->get_anchor();
          } else {
            const Abstract* literal = argument.find<const Abstract&>();
            auto expression = literal ? literal->select<Expression>()
                                      : Core::Option<const Expression&>();
            if (expression && expression->get_anchor()) {
              failure_anchor = *expression->get_anchor();
            }
          }
        }
        return map_failure(failure_anchor, failure);
      });
}

auto Language::TypeReference::resolve(const Abstract& context) const
    -> Resolution {
  return resolve_with_root(context, Root::Context);
}

auto Language::TypeReference::resolve_lexical(const Abstract& context) const
    -> Resolution {
  return resolve_with_root(context, Root::Lexical);
}

auto Language::TypeReference::resolve_authored(
    Cursor& cursor,
    const Abstract& context) const -> Core::Option<const Abstract&> {
  Core::Option<const Abstract&> selected;
  resolve_with_root(context, Root::Lexical, cursor)
      .visit(
          [&](const Abstract& resolved) { selected = resolved; },
          [&](const Failure& failure) { report(cursor, failure); });
  return selected;
}

auto Language::TypeReference::report(Cursor& cursor, const Failure& failure)
    const -> void {
  switch (failure.get_type()) {
  case Failure::Type::Route: {
    auto report = cursor.create_report(failure.get_anchor());
    report << "Library route segment "_view << U64(failure.get_index() + 1)
           << " did not resolve in its selected context."_view;
    report.get_hint()
        << "Publish that exact name before linking this declaration."_view;
    return;
  }
  case Failure::Type::Argument: {
    auto report = cursor.create_report(failure.get_anchor());
    report << "Library Generic argument "_view << U64(failure.get_index() + 1)
           << " did not resolve to one Library Type."_view;
    report.get_hint()
        << "Use a Type route or one literal accepted by this Generic."_view;
    return;
  }
  case Failure::Type::Generic:
    cursor.create_expression_error(
        failure.get_anchor(),
        "Library Type arguments require a Generic at the route terminal."_view,
        "Remove the arguments or select one named Generic."_view);
    return;
  case Failure::Type::Unavailable:
    cursor.create_expression_error(
        failure.get_anchor(),
        "Library Generic is not ready for materialization."_view,
        "Complete the selected Generic before applying arguments."_view);
    return;
  case Failure::Type::Arity:
    cursor.create_expression_error(
        failure.get_anchor(),
        "Library Generic application has the wrong number of arguments."_view,
        "Supply exactly the parameters declared by the selected Generic."_view);
    return;
  case Failure::Type::Parameter: {
    auto report = cursor.create_report(failure.get_anchor());
    report << "Library Generic argument "_view << U64(failure.get_index() + 1)
           << " does not satisfy its parameter category."_view;
    report.get_hint()
        << "Use the Type or scalar Constant category required at this position."_view;
    return;
  }
  case Failure::Type::Recursive:
    cursor.create_expression_error(
        failure.get_anchor(),
        "Library Generic application is recursively self dependent."_view,
        "Break the materialization cycle with one already completed Type."_view);
    return;
  case Failure::Type::Formula:
    cursor.create_expression_error(
        failure.get_anchor(),
        "Library Generic rejected this argument combination."_view,
        "Use values admitted by the selected Generic formula."_view);
    return;
  }
}
