// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/type_reference.hpp"

#include "perimortem/core/static/union.hpp"

#include "perimortem/memory/dynamic/vector.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/library/language/expression.hpp"
#include "tetrodotoxin/library/language/generic.hpp"
#include "tetrodotoxin/library/language/model/parser/layout.hpp"
#include "tetrodotoxin/library/language/model/type.hpp"
#include "tetrodotoxin/library/language/parser/literal.hpp"
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

auto Language::TypeReference::parse(const Abstract& context, Cursor& cursor)
    -> Core::Option<TypeReference> {
  // Dispatch has already selected a Type edge, so malformed arguments reject
  // that declaration instead of making the same spelling available to a
  // competing production.
  auto& domain = cursor.get_arena();
  auto route = parse_route(cursor);
  BAIL_IF(!route);

  if (!cursor.matches(Code::Type::BracketStart)) {
    return *route;
  }

  Memory::Managed::Vector<Argument> arguments(domain);
  auto closing = Model::Parser::Layout::parse_entries(
      cursor, Code::Type::BracketStart, Code::Type::BracketEnd,
      [&](Cursor& entry, Count) -> Bool {
        if (entry.matches(Code::Type::Type)) {
          auto nested = parse(context, entry);
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
        auto literal = Parser::Literal::parse(context, entry);
        BAIL_IF(!literal);
        arguments.insert(Argument(*literal));
        return True;
      });
  BAIL_IF(!closing);

  TypeReference completed(
      route->route,
      Anchor::create(
          route->get_anchor().get_token(),
          Span(route->get_anchor().get_token(), *closing)),
      arguments.get_view());
  return completed;
}

auto Language::TypeReference::parse_route(Cursor& cursor)
    -> Core::Option<TypeReference> {
  Token first = cursor.require(
      Code::Type::Type, "Library Type reference requires one Type name."_view);
  BAIL_IF(!first);

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

    last = segment;
  }

  Count start = first.get_offset();
  Count end = Count(last.get_offset()) + Count(last.get_size());
  return TypeReference(
      cursor.get_source_text().slice(start, end - start),
      Anchor::create(first, Span(first, last)));
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

auto Language::TypeReference::matches_route(const TypeReference& other) const
    -> Bool {
  return route == other.route;
}

auto Language::TypeReference::get_argument_reference(Count index) const
    -> Core::Option<const TypeReference&> {
  BAIL_IF(!arguments || index >= arguments->get_size());
  const TypeReference* reference =
      arguments->get_data()[index].find<const TypeReference&>();
  BAIL_IF(reference == nullptr);
  return *reference;
}

static auto map_failure(
    Anchor anchor,
    const Language::Generic::Failure& failure)
    -> Language::TypeReference::Failure {
  using GenericFailure = Language::Generic::Failure;
  using ReferenceFailure = Language::TypeReference::Failure;
  switch (failure.get_type()) {
  case GenericFailure::Type::Unavailable:
    return ReferenceFailure(ReferenceFailure::Type::Unavailable, anchor);
  case GenericFailure::Type::Arity:
    return ReferenceFailure(ReferenceFailure::Type::Arity, anchor);
  case GenericFailure::Type::Parameter:
    return ReferenceFailure(
        ReferenceFailure::Type::Parameter, anchor, failure.get_argument());
  case GenericFailure::Type::Recursive:
    return ReferenceFailure(ReferenceFailure::Type::Recursive, anchor);
  case GenericFailure::Type::Formula:
    return ReferenceFailure(ReferenceFailure::Type::Formula, anchor);
  }

  return ReferenceFailure(ReferenceFailure::Type::Formula, anchor);
}

auto Language::TypeReference::resolve_with_root(
    const Abstract& context,
    Root root) const -> Resolution {
  // Lexical authority applies only to the unqualified root. Every explicit
  // suffix is an ordinary public context query on the identity just selected.
  const Abstract* selected = &context.resolve_context(get_root());
  if (root == Root::Lexical) {
    auto type = context.select<Language::Model::Type>();
    if (type) {
      selected = &type->resolve_lexical_context(get_root());
    }
  }
  selected = &resolve_alias(*selected);
  if (selected->is<Invalid>()) {
    return Failure(Failure::Type::Route, anchor, 0);
  }

  for (Count i = 1; i < get_size(); i++) {
    // TypeReference performs the explicit Alias resolution required before a
    // selected target may receive the next ordinary context query.
    selected = &resolve_alias(selected->resolve_context(get_name(i)));
    if (selected->is<Invalid>()) {
      return Failure(Failure::Type::Route, anchor, i);
    }
  }

  if (!arguments) {
    return *selected;
  }

  auto generic = selected->select<Generic>();
  if (!generic) {
    return Failure(Failure::Type::Generic, anchor);
  }

  // Resolution needs one transient real Layout. Generic copies its normalized
  // semantic key into its own Arena before this local storage leaves. Nested
  // routes use the same root policy so arguments cannot acquire extra access.
  Memory::Dynamic::Vector<Reference<const Abstract>> linked(
      arguments->get_size());
  const auto* argument_data = arguments->get_data();
  for (Count i = 0; i < arguments->get_size(); i++) {
    const Argument& argument = argument_data[i];
    const TypeReference* reference = argument.find<const TypeReference&>();
    if (reference != nullptr) {
      const Abstract* nested = nullptr;
      Core::Option<Failure> nested_failure;
      reference->resolve_with_root(context, root)
          .visit(
              [&](const Abstract& resolved) {
                nested = &resolve_alias(resolved);
              },
              [&](const Failure& failure) { nested_failure = failure; });
      if (nested_failure) {
        return *nested_failure;
      }
      if (nested == nullptr || !nested->is<Language::Model::Type>()) {
        return Failure(Failure::Type::Argument, anchor, i);
      }
      linked.insert(*nested);
      continue;
    }

    const Abstract* literal = argument.find<const Abstract&>();
    if (literal == nullptr) {
      return Failure(Failure::Type::Argument, anchor, i);
    }
    linked.insert(*literal);
  }

  Ttx::Model::Layouts::Fluid layout(linked.get_view());
  return generic->materialize(layout).visit(
      [](const Language::Model::Type& type) -> Resolution { return type; },
      [&](const Generic::Failure& failure) -> Resolution {
        // Generic owns source free formula failures. TypeReference maps the
        // parameter index back to authored syntax because it owns those
        // Anchors.
        Anchor failure_anchor = anchor;
        Count index = failure.get_argument();
        if (failure.get_type() == Generic::Failure::Type::Parameter &&
            index < arguments->get_size()) {
          const Argument& argument = arguments->get_data()[index];
          const TypeReference* reference =
              argument.find<const TypeReference&>();
          if (reference != nullptr) {
            failure_anchor = reference->get_anchor();
          } else {
            const Abstract* literal = argument.find<const Abstract&>();
            auto expression = literal == nullptr
                                  ? Core::Option<const Expression&>()
                                  : literal->select<Expression>();
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
  resolve_lexical(context).visit(
      [&](const Abstract& resolved) { selected = resolved; },
      [&](const Failure& failure) { report(cursor, failure); });
  return selected;
}

auto Language::TypeReference::report(Cursor& cursor, const Failure& failure)
    const -> void {
  switch (failure.get_type()) {
  case Failure::Type::Route: {
    auto report = cursor.create_report(failure.get_anchor());
    report << "Library route segment "_view
           << Unsigned_64(failure.get_index() + 1)
           << " did not resolve in its selected context."_view;
    report.get_hint()
        << "Publish that exact name before linking this declaration."_view;
    return;
  }
  case Failure::Type::Argument: {
    auto report = cursor.create_report(failure.get_anchor());
    report << "Library Generic argument "_view
           << Unsigned_64(failure.get_index() + 1)
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
    report << "Library Generic argument "_view
           << Unsigned_64(failure.get_index() + 1)
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
