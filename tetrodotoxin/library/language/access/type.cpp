// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/access/type.hpp"

#include "tetrodotoxin/library/language/types/composite.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/model/alias.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Utility;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin::Library;

static auto resolve_type_route(
    const Language::Access::Type& access,
    const Abstract& root,
    Option<const Ttx::Model::Type&> caller_scope) -> const Abstract& {
  Reference<const Abstract> selected(Ttx::Model::Alias::get_represented(root));
  View::Bytes route = access.get_route();
  Count segment_start = access.get_root().get_size();

  // Alias contributes only its local name, so traversal follows that exact
  // edge without asking a terminal Type whether its owner has completed it.
  // The original caller remains unchanged across that redirection: reachability
  // through an Alias never transfers the target owner's private authority.
  while (segment_start < route.get_size()) {
    segment_start += 2;
    Count segment_end = segment_start;
    while (segment_end < route.get_size() &&
           !(segment_end + 1 < route.get_size() && route[segment_end] == ':' &&
             route[segment_end + 1] == ':')) {
      segment_end++;
    }

    View::Bytes segment =
        route.slice(segment_start, segment_end - segment_start);
    const Abstract& next = selected.get().visit<Language::Types::Composite>(
        [&](const Language::Types::Composite& composite) -> const Abstract& {
          return caller_scope.visit(
              [&]() -> const Abstract& {
                return composite.resolve_context(segment);
              },
              [&](const Ttx::Model::Type& caller) -> const Abstract& {
                // An explicit segment remains local to the selected Composite.
                // Caller scope filters visibility but cannot turn a miss into
                // lookup through the selected Type's enclosing host.
                return composite.resolve_type(segment, caller);
              });
        },
        [&](const Abstract& context) -> const Abstract& {
          return context.resolve_context(segment);
        });
    selected =
        Reference<const Abstract>(Ttx::Model::Alias::get_represented(next));
    if (selected.get().is<Invalid>()) {
      return selected.get();
    }

    segment_start = segment_end;
  }

  return selected.get();
}

auto Language::Access::Type::parse(Cursor& cursor) -> Option<Type> {
  Token first = cursor.require(
      Code::Type::Type, "Library Type access requires one Type name."_view);
  if (!first) {
    return {};
  }

  // The spelling remains one borrowed source range, while the parser proves
  // every boundary so traversal can split it without retaining token state.
  Token last = first;
  while (cursor.matches(Code::Type::TypeAccessOp)) {
    Token separator = cursor.current();
    Count previous_end = Count(last.get_offset()) + Count(last.get_size());
    if (separator.get_offset() != previous_end) {
      cursor.create_expression_error(
          Span(first, separator),
          "Library Type access cannot contain whitespace around `::`."_view);
      return {};
    }

    cursor.consume();
    Token segment = cursor.require(
        Code::Type::Type,
        "Library Type access requires a Type after `::`."_view);
    if (!segment) {
      return {};
    }

    Count separator_end =
        Count(separator.get_offset()) + Count(separator.get_size());
    if (segment.get_offset() != separator_end) {
      cursor.create_expression_error(
          Span(first, segment),
          "Library Type access cannot contain whitespace around `::`."_view);
      return {};
    }

    last = segment;
  }

  Count route_start = first.get_offset();
  Count route_end = Count(last.get_offset()) + Count(last.get_size());
  View::Bytes route =
      cursor.get_source_text().slice(route_start, route_end - route_start);
  return Type(
      route, first.get_size(), Anchor::create(first, Span(first, last)));
}

auto Language::Access::Type::resolve(const Abstract& context) const
    -> const Abstract& {
  const Abstract& exact_context = Ttx::Model::Alias::get_represented(context);
  return resolve_from(exact_context.resolve_context(get_root()));
}

auto Language::Access::Type::resolve_from(const Abstract& root) const
    -> const Abstract& {
  return resolve_type_route(*this, root, {});
}

auto Language::Access::Type::resolve_from(
    const Abstract& root,
    const Ttx::Model::Type& caller_scope) const -> const Abstract& {
  return resolve_type_route(*this, root, caller_scope);
}
