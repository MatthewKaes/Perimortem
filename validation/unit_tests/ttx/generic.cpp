// Perimortem Engine
// Copyright © Matt Kaes

#include "ttx/model/generic.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/static/vector.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/bytes.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "ttx/concept/invalid.hpp"
#include "ttx/model/alias.hpp"
#include "ttx/model/constants/unsigned.hpp"
#include "ttx/model/documentations/comment.hpp"
#include "ttx/model/layouts/fluid.hpp"
#include "ttx/model/layouts/ranged.hpp"
#include "ttx/model/type.hpp"
#include "ttx/model/types/unsigned_64.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Ttx::Concept;
using namespace Ttx::Model;
using namespace Ttx::Model::Documentations;
using namespace Ttx::Model::Layouts;
using namespace Validation;

/// A concrete Type has stable object identity even when another context uses
/// the same local name.
class GenericType final : public Type {
 public:
  GenericType(View::Bytes name) : name(name) {}

  auto get_name() const -> View::Bytes override { return name; }
  auto get_documentation() const -> const Documentation& override {
    return Documentation::get_empty();
  }
  auto resolve_context(View::Bytes) const -> const Abstract& override {
    return Invalid::get_invalid();
  }
  auto get_layout() const -> const Structured& override { return layout; }

 private:
  View::Bytes name;
  inline static const Structured layout;
};

/// Formula scalars are real Constants. Two separately materialized constants
/// can therefore fold without introducing an inline scalar representation.
class GenericUnsigned final : public Constants::Unsigned {
 public:
  GenericUnsigned(
      View::Bytes name,
      const Types::Unsigned_64& type,
      Unsigned_64 value)
      : name(name), type(type), value(value) {}

  auto get_name() const -> View::Bytes override { return name; }
  auto get_type() const -> const Abstract& override { return type; }
  auto get_value() const -> Unsigned_64 override { return value; }

 private:
  View::Bytes name;
  const Types::Unsigned_64& type;
  Unsigned_64 value;
};

/// A successful formula produces an ordinary Type, not a Generic subtype.
class MaterializedVector final : public Type {
 public:
  MaterializedVector(View::Bytes name, Ranged layout)
      : name(name), layout(layout) {}

  auto get_name() const -> View::Bytes override { return name; }
  auto get_documentation() const -> const Documentation& override {
    return Documentation::get_empty();
  }
  auto resolve_context(View::Bytes) const -> const Abstract& override {
    return Invalid::get_invalid();
  }
  auto get_layout() const -> const Ranged& override { return layout; }

 private:
  View::Bytes name;
  Ranged layout;
};

/// A formula owns argument validation, materialization, and its local cache.
/// The cache is keyed by resolved arguments rather than names or routes.
class VecFormula final : public Generic {
  /// Cache entries borrow arena-owned arguments and materialized Types for the
  /// lifetime of the formula's owning compilation boundary.
  class CacheEntry {
   public:
    CacheEntry(
        View::Vector<Reference<Abstract>> arguments,
        const Abstract& materialized)
        : arguments(arguments), materialized(materialized) {}

    constexpr auto get_arguments() const -> Fluid { return Fluid(arguments); }
    constexpr auto get_materialized() const -> const Abstract& {
      return materialized;
    }

   private:
    View::Vector<Reference<Abstract>> arguments;
    const Abstract& materialized;
  };

 public:
  VecFormula(Allocator::Arena& arena) : arena(arena), cache(arena) {}

  auto get_name() const -> View::Bytes override { return "Vec"_view; }
  auto get_documentation() const -> const Documentation& override {
    return documentation;
  }
  auto resolve_context(View::Bytes) const -> const Abstract& override {
    return Invalid::get_invalid();
  }

  auto materialize(const Layout& arguments) const -> const Abstract& override {
    if (arguments.get_size() != 2) {
      return Invalid::get_invalid();
    }

    const Abstract& element = arguments.get_abstract(0).resolve();
    const Abstract& extent = arguments.get_abstract(1).resolve();
    const Bool valid_element = element.is<Type>();
    const Bool valid_extent =
        extent.is<Constants::Unsigned>() &&
        extent.assume<Constants::Unsigned>().get_value() == Unsigned_64(3);
    if (!valid_element || !valid_extent) {
      return Invalid::get_invalid();
    }

    for (Count i = 0; i < cache.get_size(); i++) {
      if (arguments_equal(cache[i].get_arguments(), arguments)) {
        return cache[i].get_materialized();
      }
    }

    const Type& element_type = element.assume<Type>();
    Managed::Bytes name(arena, "Vec["_view);
    name.concat(element_type.get_name());
    name.concat(",3]"_view);

    const MaterializedVector& result = arena.construct<MaterializedVector>(
        name.get_view(), Ranged(element_type, 3));
    Managed::Vector<Reference<Abstract>> stored_arguments(arena);
    for (Count i = 0; i < arguments.get_size(); i++) {
      stored_arguments.insert(arguments.get_abstract(i).resolve());
    }

    cache.insert(CacheEntry(stored_arguments.get_view(), result));
    return result;
  }

  constexpr auto get_cache_size() const -> Count { return cache.get_size(); }

 private:
  Allocator::Arena& arena;
  mutable Managed::Vector<CacheEntry> cache;
  static constexpr Comment documentation{
    "Creates a fixed-size vector Type."_view,
  };
};

/// The selected scope owns formula lookup and keeps missing names distinct
/// from failures reported by a formula that was found.
class GenericScope final : public Type {
 public:
  GenericScope(const Generic& formula, const Type& concrete)
      : formula(formula), concrete(concrete) {}

  auto get_name() const -> View::Bytes override { return "Package"_view; }
  auto get_documentation() const -> const Documentation& override {
    return Documentation::get_empty();
  }
  auto resolve_context(View::Bytes route) const -> const Abstract& override {
    if (route == formula.get_name()) {
      return formula;
    }
    if (route == concrete.get_name()) {
      return concrete;
    }
    return Invalid::get_invalid();
  }
  auto get_layout() const -> const Structured& override { return layout; }

 private:
  const Generic& formula;
  const Type& concrete;
  inline static const Structured layout;
};

/// Generic cache equivalence follows ordinary Abstract identity redirection. A
/// future type-producing expression can use this path after evaluation chooses
/// its concrete Type without making every Expression resolve to its result.
class TypeRedirect final : public Abstract {
 public:
  TypeRedirect(View::Bytes name, const Abstract& type)
      : name(name), type(type) {}

  auto get_name() const -> View::Bytes override { return name; }
  auto get_documentation() const -> const Documentation& override {
    return Documentation::get_empty();
  }
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
  GenericType unsigned_64("Unsigned_64"_view);
  Types::Unsigned_64 extent_type;
  GenericUnsigned alias_extent("three"_view, extent_type, Unsigned_64(3));
  GenericUnsigned direct_extent("also_three"_view, extent_type, Unsigned_64(3));
  Alias count("Count"_view, unsigned_64);
  TypeRedirect expression("computed_type"_view, count);
  VecFormula vec(arena);
  GenericScope scope(vec, unsigned_64);
  const Static::Vector<Reference<Abstract>, 2> alias_abstracts = {{
    count,
    alias_extent,
  }};
  const Static::Vector<Reference<Abstract>, 2> direct_abstracts = {{
    unsigned_64,
    direct_extent,
  }};
  const Static::Vector<Reference<Abstract>, 2> expression_abstracts = {{
    expression,
    direct_extent,
  }};
  Fluid alias_arguments(alias_abstracts);
  Fluid direct_arguments(direct_abstracts);
  Fluid expression_arguments(expression_abstracts);

  const Abstract& selected = scope.resolve_context("Vec"_view);
  const Abstract& from_alias =
      selected.assume<Generic>().materialize(alias_arguments);
  const Abstract& from_type =
      selected.assume<Generic>().materialize(direct_arguments);
  const Abstract& from_expression =
      selected.assume<Generic>().materialize(expression_arguments);

  EXPECT(selected.is<Generic>());
  EXPECT(&from_alias == &from_type);
  EXPECT(&from_alias == &from_expression);
  EXPECT(from_alias.is<Type>());
  EXPECT_EQ(from_alias.assume<Type>().get_layout().get_size(), Count(3));
  EXPECT(
      &from_alias.assume<Type>().get_layout().get_abstract(0).resolve() ==
      &unsigned_64);
  EXPECT_EQ(vec.get_cache_size(), Count(1));
}

PERIMORTEM_UNIT_TEST(TtxGeneric, name_is_not_identity) {
  Allocator::Arena arena;
  GenericType graphics_image("Image"_view);
  GenericType runtime_image("Image"_view);
  Types::Unsigned_64 extent_type;
  GenericUnsigned extent("three"_view, extent_type, Unsigned_64(3));
  VecFormula vec(arena);
  const Static::Vector<Reference<Abstract>, 2> graphics_abstracts = {{
    graphics_image,
    extent,
  }};
  const Static::Vector<Reference<Abstract>, 2> runtime_abstracts = {{
    runtime_image,
    extent,
  }};
  Fluid graphics_arguments(graphics_abstracts);
  Fluid runtime_arguments(runtime_abstracts);

  const Abstract& graphics = vec.materialize(graphics_arguments);
  const Abstract& runtime = vec.materialize(runtime_arguments);

  EXPECT(&graphics != &runtime);
  EXPECT_TEXT(graphics.get_name(), runtime.get_name());
  EXPECT_EQ(vec.get_cache_size(), Count(2));
}

PERIMORTEM_UNIT_TEST(TtxGeneric, failure_boundaries) {
  Allocator::Arena arena;
  GenericType unsigned_64("Unsigned_64"_view);
  VecFormula vec(arena);
  GenericScope scope(vec, unsigned_64);
  const Static::Vector<Reference<Abstract>, 2> rejected_abstracts = {{
    unsigned_64,
    unsigned_64,
  }};
  Fluid rejected_arguments(rejected_abstracts);
  Fluid empty_arguments;

  EXPECT(scope.resolve_context("Missing"_view).is<Invalid>());
  EXPECT_NOT(scope.resolve_context("Unsigned_64"_view).is<Generic>());
  EXPECT(&vec.materialize(rejected_arguments) == &Invalid::get_invalid());
  EXPECT(&vec.materialize(empty_arguments) == &Invalid::get_invalid());
  EXPECT_EQ(vec.get_cache_size(), Count(0));
}
