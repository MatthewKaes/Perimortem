// Perimortem Engine
// Copyright © Matt Kaes

#include "ttx/model/generic.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/bytes.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "ttx/concept/alias.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/model/type.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Ttx::Concept;
using namespace Ttx::Model;
using namespace Ttx::Model::Layouts;
using namespace Validation;

/// A concrete Type has stable object identity even when another context uses
/// the same local name.
class GenericType final : public Type {
 public:
  GenericType(View::Bytes name, const Invalid& invalid)
      : name(name), invalid(invalid) {}

  auto get_name() const -> View::Bytes override { return name; }
  auto resolve_context(View::Bytes) const -> const Abstract& override {
    return invalid;
  }
  auto get_layout() const -> const Structured& override { return layout; }

 private:
  View::Bytes name;
  const Invalid& invalid;
  Structured layout;
};

/// Materialized fields remain real Addressable edges in Structured layouts.
class GenericField final : public Addressable {
 public:
  GenericField(View::Bytes name, const Type& type) : name(name), type(type) {}

  auto get_name() const -> View::Bytes override { return name; }
  auto resolve() const -> const Abstract& override { return type.resolve(); }
  auto resolve_context(View::Bytes route) const -> const Abstract& override {
    return type.resolve().resolve_context(route);
  }

 private:
  View::Bytes name;
  const Type& type;
};

/// A successful formula produces an ordinary Type, not a Generic subtype.
class MaterializedVector final : public Type {
 public:
  MaterializedVector(
      View::Bytes name,
      const Invalid& invalid,
      Structured layout)
      : name(name), invalid(invalid), layout(layout) {}

  auto get_name() const -> View::Bytes override { return name; }
  auto resolve_context(View::Bytes) const -> const Abstract& override {
    return invalid;
  }
  auto get_layout() const -> const Structured& override { return layout; }

 private:
  View::Bytes name;
  const Invalid& invalid;
  Structured layout;
};

/// A formula owns argument validation, materialization, and its local cache.
/// The cache is keyed by resolved arguments rather than names or routes.
class VecFormula final : public Generic {
  /// Cache entries borrow arena-owned arguments and materialized Types for the
  /// lifetime of the formula's owning compilation boundary.
  class CacheEntry {
   public:
    CacheEntry(View::Vector<Argument> arguments, const Abstract& materialized)
        : arguments(arguments), materialized(materialized) {}

    constexpr auto get_arguments() const -> View::Vector<Argument> {
      return arguments;
    }
    constexpr auto get_materialized() const -> const Abstract& {
      return materialized;
    }

   private:
    View::Vector<Argument> arguments;
    const Abstract& materialized;
  };

 public:
  VecFormula(Allocator::Arena& arena, const Invalid& invalid)
      : arena(arena), invalid(invalid), cache(arena) {}

  auto get_name() const -> View::Bytes override { return "Vec"_view; }
  auto resolve_context(View::Bytes) const -> const Abstract& override {
    return invalid;
  }

  auto materialize(View::Vector<Argument> arguments) const
      -> const Abstract& override {
    if (arguments.get_size() != 2) {
      return invalid;
    }

    const Bool valid_element = arguments[0].get_value().visit(
        []() -> Bool { return False; },
        [](const Reference<Abstract>& element) -> Bool {
          return element.get().is<Type>();
        },
        [](auto) -> Bool { return False; });
    const Bool valid_extent = arguments[1].get_value().visit(
        []() -> Bool { return False; },
        [](Bits_64 extent) -> Bool { return extent == 3; },
        [](auto) -> Bool { return False; });
    if (!valid_element || !valid_extent) {
      return invalid;
    }

    for (Count i = 0; i < cache.get_size(); i++) {
      if (cache[i].get_arguments() == arguments) {
        return cache[i].get_materialized();
      }
    }

    const Reference<Abstract> element = arguments[0].get_value().visit(
        [this]() -> Reference<Abstract> { return invalid; },
        [](const Reference<Abstract>& value) -> Reference<Abstract> {
          return value;
        },
        [this](auto) -> Reference<Abstract> { return invalid; });
    const Type& element_type = element.get().as<Type>();
    Managed::Bytes name(arena, "Vec["_view);
    name.concat(element_type.get_name());
    name.concat(",3]"_view);

    constexpr View::Bytes component_names[] = {
      "x"_view,
      "y"_view,
      "z"_view,
    };
    Managed::Vector<Reference<Addressable>> fields(arena);
    for (Count i = 0; i < 3; i++) {
      const GenericField& field =
          arena.construct<GenericField>(component_names[i], element_type);
      fields.insert(Reference<Addressable>(field));
    }

    const MaterializedVector& result = arena.construct<MaterializedVector>(
        name.get_view(), invalid, Structured(fields.get_view()));
    Managed::Vector<Argument> stored_arguments(arena);
    for (Count i = 0; i < arguments.get_size(); i++) {
      stored_arguments.insert(arguments[i]);
    }

    cache.insert(CacheEntry(stored_arguments.get_view(), result));
    return result;
  }

  constexpr auto get_cache_size() const -> Count { return cache.get_size(); }

 private:
  Allocator::Arena& arena;
  const Invalid& invalid;
  mutable Managed::Vector<CacheEntry> cache;
};

/// The selected scope owns formula lookup and keeps missing names distinct
/// from failures reported by a formula that was found.
class GenericScope final : public Type {
 public:
  GenericScope(
      const Generic& formula,
      const Type& concrete,
      const Invalid& invalid)
      : formula(formula), concrete(concrete), invalid(invalid) {}

  auto get_name() const -> View::Bytes override { return "Package"_view; }
  auto resolve_context(View::Bytes route) const -> const Abstract& override {
    if (route == formula.get_name()) {
      return formula;
    }
    if (route == concrete.get_name()) {
      return concrete;
    }
    return invalid;
  }
  auto get_layout() const -> const Structured& override { return layout; }

 private:
  const Generic& formula;
  const Type& concrete;
  const Invalid& invalid;
  Structured layout;
};

/// Argument normalization follows ordinary Abstract identity redirection. A
/// future type-producing expression can use this path after evaluation chooses
/// its concrete Type without making every Expression resolve to its result.
class TypeRedirect final : public Abstract {
 public:
  TypeRedirect(View::Bytes name, const Abstract& type)
      : name(name), type(type) {}

  auto get_name() const -> View::Bytes override { return name; }
  auto resolve() const -> const Abstract& override { return type.resolve(); }
  auto resolve_context(View::Bytes route) const -> const Abstract& override {
    return type.resolve().resolve_context(route);
  }

 private:
  View::Bytes name;
  const Abstract& type;
};

static Harness TtxGeneric = {
  .name = "TTX::Generic"_view,
};

PERIMORTEM_UNIT_TEST(TtxGeneric, resolved_cache_key) {
  Allocator::Arena arena;
  Invalid invalid;
  GenericType unsigned_64("Unsigned_64"_view, invalid);
  Alias count("Count"_view, unsigned_64);
  TypeRedirect expression("computed_type"_view, count);
  VecFormula vec(arena, invalid);
  GenericScope scope(vec, unsigned_64, invalid);
  const Argument alias_arguments[] = {count, Bits_64(3)};
  const Argument direct_arguments[] = {unsigned_64, Bits_64(3)};
  const Argument expression_arguments[] = {expression, Bits_64(3)};

  const Abstract& selected = scope.resolve_context("Vec"_view);
  const Abstract& from_alias =
      selected.as<Generic>().materialize(alias_arguments);
  const Abstract& from_type =
      selected.as<Generic>().materialize(direct_arguments);
  const Abstract& from_expression =
      selected.as<Generic>().materialize(expression_arguments);

  EXPECT(selected.is<Generic>());
  EXPECT(alias_arguments[0] == direct_arguments[0]);
  EXPECT(expression_arguments[0] == direct_arguments[0]);
  EXPECT(&from_alias == &from_type);
  EXPECT(&from_alias == &from_expression);
  EXPECT(from_alias.is<Type>());
  EXPECT_EQ(from_alias.as<Type>().get_layout().get_size(), Count(3));
  EXPECT(
      &from_alias.as<Type>().get_layout().get_abstract(0).resolve() ==
      &unsigned_64);
  EXPECT_EQ(vec.get_cache_size(), Count(1));
}

PERIMORTEM_UNIT_TEST(TtxGeneric, name_is_not_identity) {
  Allocator::Arena arena;
  Invalid invalid;
  GenericType graphics_image("Image"_view, invalid);
  GenericType runtime_image("Image"_view, invalid);
  VecFormula vec(arena, invalid);
  const Argument graphics_arguments[] = {graphics_image, Bits_64(3)};
  const Argument runtime_arguments[] = {runtime_image, Bits_64(3)};

  const Abstract& graphics = vec.materialize(graphics_arguments);
  const Abstract& runtime = vec.materialize(runtime_arguments);

  EXPECT(&graphics != &runtime);
  EXPECT_TEXT(graphics.get_name(), runtime.get_name());
  EXPECT_EQ(vec.get_cache_size(), Count(2));
}

PERIMORTEM_UNIT_TEST(TtxGeneric, failure_boundaries) {
  Allocator::Arena arena;
  Invalid invalid;
  GenericType unsigned_64("Unsigned_64"_view, invalid);
  VecFormula vec(arena, invalid);
  GenericScope scope(vec, unsigned_64, invalid);
  const Argument rejected_arguments[] = {unsigned_64, True};
  const Argument empty_arguments[] = {
    Argument::Value(),
    Bits_64(3),
  };

  EXPECT(scope.resolve_context("Missing"_view).is<Invalid>());
  EXPECT_NOT(scope.resolve_context("Unsigned_64"_view).is<Generic>());
  EXPECT(&vec.materialize(rejected_arguments) == &invalid);
  EXPECT(&vec.materialize(empty_arguments) == &invalid);
  EXPECT_EQ(vec.get_cache_size(), Count(0));
}
