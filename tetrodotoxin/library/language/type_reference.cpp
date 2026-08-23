// Tetrodotoxin
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

enum class PersistedArgument : U8 {
  Reference,
  Unsigned,
  Signed,
  False,
  True,
};

static auto write_argument(
    Archive::Writer& writer,
    const Language::TypeReference::Argument& argument) -> Bool {
  return argument.visit(
      []() -> Bool { return False; },
      [&](const Language::TypeReference& reference) -> Bool {
        writer.write(U8(PersistedArgument::Reference));
        return reference.persist(writer);
      },
      [&](const Abstract& selected) -> Bool {
        auto unsigned_value = selected.select<Language::Constants::Unsigned>();
        if (unsigned_value) {
          writer.write(U8(PersistedArgument::Unsigned));
          writer.write(unsigned_value->get_value());
          return True;
        }

        auto signed_value = selected.select<Language::Constants::Signed>();
        if (signed_value) {
          writer.write(U8(PersistedArgument::Signed));
          writer.write(signed_value->get_value());
          return True;
        }

        if (selected.is<Language::Constants::False>()) {
          writer.write(U8(PersistedArgument::False));
          return True;
        }
        if (selected.is<Language::Constants::True>()) {
          writer.write(U8(PersistedArgument::True));
          return True;
        }
        return False;
      });
}

static auto resolve_root_type(const Abstract& context, Core::View::Bytes name)
    -> Core::Option<const Language::Model::Type&> {
  return context.resolve_context(name)
      .resolve()
      .select<Language::Model::Type>();
}

static auto read_argument(
    Archive::Reader& reader,
    Memory::Allocator::Arena& arena,
    const Abstract& context)
    -> Core::Option<Language::TypeReference::Argument> {
  auto kind = reader.read_u8();
  BAIL_IF(!kind);

  switch (PersistedArgument(*kind)) {
  case PersistedArgument::Reference: {
    auto reference = Language::TypeReference::restore(reader, arena, context);
    BAIL_IF(!reference);
    const auto& retained = arena.construct<Language::TypeReference>(*reference);
    return Language::TypeReference::Argument(retained);
  }
  case PersistedArgument::Unsigned: {
    auto value = reader.read_u64();
    auto type = resolve_root_type(context, "U64"_view);
    auto selected =
        type ? type->select<Language::Model::Types::Unsigned>()
             : Core::Option<const Language::Model::Types::Unsigned&>();
    BAIL_IF(!value || !selected);
    const auto& constant = Language::Constants::Unsigned::create_synthetic(
        arena, *selected, *value);
    return Language::TypeReference::Argument(
        static_cast<const Abstract&>(constant));
  }
  case PersistedArgument::Signed: {
    auto value = reader.read_s64();
    auto type = resolve_root_type(context, "S64"_view);
    auto selected = type
                        ? type->select<Language::Model::Types::Signed>()
                        : Core::Option<const Language::Model::Types::Signed&>();
    BAIL_IF(!value || !selected);
    const auto& constant =
        Language::Constants::Signed::create_synthetic(arena, *selected, *value);
    return Language::TypeReference::Argument(
        static_cast<const Abstract&>(constant));
  }
  case PersistedArgument::False:
  case PersistedArgument::True: {
    auto type = resolve_root_type(context, "Bool"_view);
    auto selected = type ? type->select<Language::Model::Types::Flag>()
                         : Core::Option<const Language::Model::Types::Flag&>();
    BAIL_IF(!selected);
    const Abstract& constant =
        PersistedArgument(*kind) == PersistedArgument::True
            ? static_cast<const Abstract&>(
                  Language::Constants::True::create_synthetic(arena, *selected))
            : static_cast<const Abstract&>(
                  Language::Constants::False::create_synthetic(
                      arena, *selected));
    return Language::TypeReference::Argument(constant);
  }
  }

  return {};
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
  BAIL_IF(!reference);
  return *reference;
}

auto Language::TypeReference::get_argument(Count index) const
    -> Core::Option<const Argument&> {
  BAIL_IF(!arguments || index >= arguments->get_size());
  return arguments->get_data()[index];
}

auto Language::TypeReference::persist(Archive::Writer& writer) const -> Bool {
  auto record = writer.begin(Archive::Tag::TypeReference);
  BAIL_IF(!writer.write(route) || get_argument_size() > U32(-1));

  writer.write(U32(get_argument_size()));
  for (Count index = 0; index < get_argument_size(); index++) {
    auto argument = get_argument(index);
    BAIL_IF(!argument || !write_argument(writer, *argument));
  }
  return writer.finish(record);
}

auto Language::TypeReference::restore(
    Archive::Reader& reader,
    Memory::Allocator::Arena& arena,
    const Abstract& context) -> Core::Option<TypeReference> {
  auto record = reader.read_record();
  BAIL_IF(
      !record || record->get_tag() != U16(Archive::Tag::TypeReference) ||
      record->is_optional());

  Archive::Reader contents(record->get_payload());
  auto route = contents.read_bytes();
  auto count = contents.read_u32();
  BAIL_IF(!route || route->is_empty() || !count);

  Memory::Managed::Vector<Argument> restored(arena);
  for (Count index = 0; index < *count; index++) {
    auto argument = read_argument(contents, arena, context);
    BAIL_IF(!argument);
    restored.insert(*argument);
  }
  BAIL_IF(!contents.is_complete());

  Core::Option<Core::View::Vector<Argument>> selected_arguments;
  if (*count != 0) {
    selected_arguments = restored.get_view();
  }
  return TypeReference(
      arena.proxy(*route), Anchor::create(Span()), Token(), selected_arguments);
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
  }

  if (!arguments) {
    const Abstract& resolved = resolve_alias(*selected);
    if (resolved.is<Invalid>()) {
      return Failure(Failure::Type::Route, anchor, get_size() - 1);
    }

    if (cursor) {
      cursor->get_associations().create(anchor, *selected);
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
    cursor->get_associations().create(
        Anchor::create(terminal, Span(terminal)), *generic);
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
      if (!nested || !nested->is<Language::Model::Type>()) {
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
