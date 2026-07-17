// Perimortem Engine
// Copyright © Matt Kaes

#include "validation/unit_test.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "ttx/concept/alias.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/model/binding.hpp"
#include "ttx/model/constants/flag.hpp"
#include "ttx/model/constants/unsigned.hpp"
#include "ttx/model/packs/named.hpp"
#include "ttx/model/packs/positional.hpp"
#include "ttx/model/projection.hpp"
#include "ttx/model/types/boolean.hpp"
#include "ttx/model/types/unsigned_64.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Ttx::Concept;
using namespace Ttx::Model;
using namespace Ttx::Model::Layouts;
using namespace Validation;

/// A flow Type supplies the stable Structured shape used by projection,
/// swizzle, and slice construction without adding expression behavior to Type.
class FlowType final : public Type {
 public:
  FlowType(View::Bytes name, Structured layout = Structured())
      : name(name), layout(layout) {}

  auto get_name() const -> View::Bytes override { return name; }
  auto get_documentation() const -> const Documentation& override {
    return Comment::get_empty();
  }
  auto resolve_context(View::Bytes) const -> const Abstract& override {
    return Invalid::get_invalid();
  }
  auto get_layout() const -> const Structured& override { return layout; }

 private:
  View::Bytes name;
  Structured layout;
};

/// A flow field remains the real Addressable selected by every Projection.
class FlowField final : public Addressable {
 public:
  FlowField(View::Bytes name, const Abstract& type) : name(name), type(type) {}

  auto get_name() const -> View::Bytes override { return name; }
  auto get_documentation() const -> const Documentation& override {
    return Comment::get_empty();
  }
  auto resolve() const -> const Abstract& override { return type.resolve(); }
  auto resolve_context(View::Bytes route) const -> const Abstract& override {
    return type.resolve().resolve_context(route);
  }

 private:
  View::Bytes name;
  const Abstract& type;
};

/// A leaf expression proves that ordinary typed values remain one Pack entry
/// even when their Type has a non-empty Structured Layout.
class FlowExpression final : public Expression {
 public:
  FlowExpression(View::Bytes name, const Abstract& type)
      : name(name), type(type) {}

  auto get_name() const -> View::Bytes override { return name; }
  auto get_documentation() const -> const Documentation& override {
    return Comment::get_empty();
  }
  auto get_type() const -> const Abstract& override { return type; }
  auto get_inputs() const -> const Layout& override { return inputs; }

 private:
  View::Bytes name;
  const Abstract& type;
  inline static const Fluid inputs;
};

/// Constant fixtures make fixed slice arguments semantic values rather than
/// raw indexes smuggled directly into the Pack constructor.
template <typename Contract>
class FlowConstant final : public Contract {
 public:
  using Value = typename Contract::Value;

  FlowConstant(View::Bytes name, const Type& type, Value value)
      : name(name), type(type), value(value) {}

  auto get_name() const -> View::Bytes override { return name; }
  auto get_type() const -> const Abstract& override { return type; }
  auto get_value() const -> Value override { return value; }

 private:
  View::Bytes name;
  const Type& type;
  Value value;
};

/// This test owner models the future ISA boundary. It retains source context,
/// validates selection, allocates stable Projection identities, and returns
/// Invalid instead of placing failure state inside Pack.
class FlowOwner final {
 public:
  FlowOwner(Allocator::Arena& arena) : arena(arena) {}

  auto group(View::Vector<Reference<Abstract>> values) const
      -> const Packs::Positional& {
    Managed::Vector<Reference<Abstract>> flattened(arena);
    for (Count i = 0; i < values.get_size(); i++) {
      append(flattened, values[i].get());
    }
    return arena.construct<Packs::Positional>(flattened.get_view());
  }

  auto swizzle(const Expression& receiver, View::Vector<View::Bytes> names)
      const -> const Abstract& {
    const Abstract& receiver_type = receiver.get_type().resolve();
    if (!receiver_type.is<Type>()) {
      return Invalid::get_invalid();
    }

    const Structured& fields = receiver_type.as<Type>().get_layout();
    Managed::Vector<Reference<Abstract>> projections(arena);
    for (Count i = 0; i < names.get_size(); i++) {
      Count selected = fields.get_size();
      Count matches = 0;
      for (Count field_index = 0; field_index < fields.get_size();
           field_index++) {
        if (fields.get_abstract(field_index).get_name() == names[i]) {
          selected = field_index;
          matches++;
        }
      }
      if (matches != 1) {
        return Invalid::get_invalid();
      }

      const Addressable& field =
          fields.get_abstract(selected).as<Addressable>();
      const Projection& projection =
          arena.construct<Projection>(receiver, field);
      projections.insert(Reference<Abstract>(projection));
    }

    return arena.construct<Packs::Positional>(projections.get_view());
  }

  auto slice(
      const Expression& receiver,
      const Constant& start,
      const Constant& count) const -> const Abstract& {
    if (!start.is<Constants::Unsigned>() || !count.is<Constants::Unsigned>()) {
      return Invalid::get_invalid();
    }

    const Abstract& receiver_type = receiver.get_type().resolve();
    if (!receiver_type.is<Type>()) {
      return Invalid::get_invalid();
    }

    const Structured& fields = receiver_type.as<Type>().get_layout();
    Unsigned_64 first = start.as<Constants::Unsigned>().get_value();
    Unsigned_64 size = count.as<Constants::Unsigned>().get_value();
    if (first > fields.get_size() || size > fields.get_size() - first) {
      return Invalid::get_invalid();
    }

    Managed::Vector<Reference<Abstract>> projections(arena);
    for (Count i = 0; i < size; i++) {
      const Addressable& field =
          fields.get_abstract(Count(first + i)).as<Addressable>();
      const Projection& projection =
          arena.construct<Projection>(receiver, field);
      projections.insert(Reference<Abstract>(projection));
    }
    return arena.construct<Packs::Positional>(projections.get_view());
  }

 private:
  auto append(
      Managed::Vector<Reference<Abstract>>& flattened,
      const Abstract& source) const -> void {
    const Abstract& represented = source.resolve();
    if (!represented.is<Packs::Positional>()) {
      flattened.insert(Reference<Abstract>(source));
      return;
    }

    const Layout& nested = represented.as<Pack>().get_layout();
    for (Count i = 0; i < nested.get_size(); i++) {
      append(flattened, nested.get_abstract(i));
    }
  }

  Allocator::Arena& arena;
};

static Harness TtxFlow = {
  .name = "TTX::Flow"_view,
};

PERIMORTEM_UNIT_TEST(TtxFlow, projection) {
  FlowType real("Real"_view);
  FlowField r("r"_view, real);
  const Reference<Addressable> fields[] = {r};
  FlowType color("Color"_view, Structured(fields));
  FlowExpression receiver("color"_view, color);
  Projection projection(receiver, r);
  FlowField missing("missing"_view, Invalid::get_invalid());
  Projection unresolved(receiver, missing);

  EXPECT(projection.is<Expression>());
  EXPECT(projection.is<Projection>());
  EXPECT_NOT(projection.is<Pack>());
  EXPECT(&projection.resolve() == &projection);
  EXPECT_TEXT(projection.get_name(), "r"_view);
  EXPECT(&projection.get_type() == &real);
  EXPECT(&projection.get_receiver() == &receiver);
  EXPECT(&projection.get_addressable() == &r);
  const Layout& inputs = projection.get_inputs();
  EXPECT_EQ(inputs.get_size(), Count(1));
  EXPECT(&inputs.get_abstract(0) == &receiver);
  EXPECT(inputs.get_abstract(1).is<Invalid>());
  EXPECT(projection.fits(real));
  EXPECT(unresolved.get_type().is<Invalid>());
  EXPECT_NOT(unresolved.fits(real));
}

PERIMORTEM_UNIT_TEST(TtxFlow, binding) {
  FlowType real("Real"_view);
  FlowExpression source("source"_view, real);
  Binding binding("x"_view, source);

  EXPECT(binding.is<Expression>());
  EXPECT(binding.is<Binding>());
  EXPECT(&binding.resolve() == &binding);
  EXPECT_TEXT(binding.get_name(), "x"_view);
  EXPECT(&binding.get_type() == &real);
  EXPECT(&binding.get_expression() == &source);
  const Layout& inputs = binding.get_inputs();
  EXPECT_EQ(inputs.get_size(), Count(1));
  EXPECT(&inputs.get_abstract(0) == &source);
  EXPECT(inputs.get_abstract(1).is<Invalid>());
  EXPECT(binding.fits(real));
}

PERIMORTEM_UNIT_TEST(TtxFlow, pack_flatten) {
  Allocator::Arena arena;
  FlowOwner owner(arena);
  FlowType real("Real"_view);
  FlowExpression a("a"_view, real);
  FlowExpression b("b"_view, real);
  FlowExpression c("c"_view, real);
  const Reference<Abstract> inner_values[] = {a, b};
  const Packs::Positional& inner = owner.group(inner_values);
  Alias pair("pair"_view, inner);
  const Reference<Abstract> outer_values[] = {pair, c};
  const Packs::Positional& outer = owner.group(outer_values);
  FlowField first_slot("first"_view, real);
  FlowField second_slot("second"_view, real);
  FlowField third_slot("third"_view, real);
  const Reference<Addressable> outer_fields[] = {
    first_slot,
    second_slot,
    third_slot,
  };
  Structured outer_target(outer_fields);

  FlowField x("x"_view, real);
  FlowField y("y"_view, real);
  const Reference<Addressable> fields[] = {x, y};
  FlowType vector("Vector"_view, Structured(fields));
  FlowExpression typed("typed"_view, vector);
  const Reference<Abstract> typed_values[] = {outer, typed};
  const Packs::Positional& with_typed = owner.group(typed_values);

  EXPECT(inner.is<Pack>());
  EXPECT(inner.is<Packs::Positional>());
  EXPECT_NOT(inner.is<Packs::Named>());
  EXPECT_EQ(outer.get_layout().get_size(), Count(3));
  EXPECT(&outer.get_layout().get_abstract(0) == &a);
  EXPECT(&outer.get_layout().get_abstract(1) == &b);
  EXPECT(&outer.get_layout().get_abstract(2) == &c);
  EXPECT(outer.get_layout().get_abstract(3).is<Invalid>());
  EXPECT(outer.get_layout().fits(outer_target));
  EXPECT_EQ(with_typed.get_layout().get_size(), Count(4));
  EXPECT(&with_typed.get_layout().get_abstract(3) == &typed);
  EXPECT(with_typed.get_name().is_empty());
  EXPECT(with_typed.resolve_context("x"_view).is<Invalid>());
}

PERIMORTEM_UNIT_TEST(TtxFlow, named_pack) {
  FlowType real("Real"_view);
  FlowExpression first("first"_view, real);
  FlowExpression second("second"_view, real);
  Binding x("x"_view, first);
  Binding y("y"_view, second);
  const Reference<Abstract> values[] = {y, x};
  Packs::Named named(values);

  FlowField x_field("x"_view, real);
  FlowField y_field("y"_view, real);
  const Reference<Addressable> fields[] = {x_field, y_field};
  Structured target(fields);

  Binding duplicate("x"_view, second);
  const Reference<Abstract> duplicates[] = {x, duplicate};
  Packs::Named ambiguous(duplicates);
  Binding unnamed({}, second);
  const Reference<Abstract> empty_names[] = {x, unnamed};
  Packs::Named nameless(empty_names);
  const Reference<Abstract> intrinsic_values[] = {first, second};
  Packs::Named intrinsic(intrinsic_values);
  FlowField first_field("first"_view, real);
  FlowField second_field("second"_view, real);
  const Reference<Addressable> intrinsic_fields[] = {
    first_field,
    second_field,
  };
  Structured intrinsic_target(intrinsic_fields);

  EXPECT(named.is<Pack>());
  EXPECT(named.is<Packs::Named>());
  EXPECT_NOT(named.is<Packs::Positional>());
  EXPECT_EQ(named.get_layout().get_size(), Count(2));
  EXPECT(&named.get_layout().get_abstract(0) == &y);
  EXPECT(named.get_layout().fits(target));
  EXPECT(&named.get_layout().get_fitted(target, 0) == &x);
  EXPECT(&named.get_layout().get_fitted(target, 1) == &y);
  EXPECT_NOT(ambiguous.get_layout().fits(target));
  EXPECT_NOT(nameless.get_layout().fits(target));
  EXPECT(intrinsic.get_layout().fits(intrinsic_target));
}

PERIMORTEM_UNIT_TEST(TtxFlow, swizzle) {
  Allocator::Arena arena;
  FlowType real("Real"_view);
  FlowField r("r"_view, real);
  FlowField g("g"_view, real);
  FlowField b("b"_view, real);
  const Reference<Addressable> fields[] = {r, g, b};
  FlowType color("Color"_view, Structured(fields));
  FlowExpression receiver("color"_view, color);
  FlowExpression unresolved("unresolved"_view, Invalid::get_invalid());
  FlowOwner owner(arena);
  const View::Bytes names[] = {"g"_view, "r"_view, "g"_view};
  const View::Bytes missing[] = {"missing"_view};

  const Abstract& selected = owner.swizzle(receiver, names);
  const Layout& layout = selected.as<Pack>().get_layout();

  EXPECT(selected.is<Packs::Positional>());
  EXPECT_EQ(layout.get_size(), Count(3));
  EXPECT_TEXT(layout.get_abstract(0).get_name(), "g"_view);
  EXPECT_TEXT(layout.get_abstract(1).get_name(), "r"_view);
  EXPECT_TEXT(layout.get_abstract(2).get_name(), "g"_view);
  EXPECT(&layout.get_abstract(0).as<Projection>().get_receiver() == &receiver);
  EXPECT(owner.swizzle(receiver, missing).is<Invalid>());
  EXPECT(owner.swizzle(unresolved, names).is<Invalid>());
}

PERIMORTEM_UNIT_TEST(TtxFlow, fixed_slice) {
  Allocator::Arena arena;
  FlowType real("Real"_view);
  FlowField r("r"_view, real);
  FlowField g("g"_view, real);
  FlowField b("b"_view, real);
  const Reference<Addressable> fields[] = {r, g, b};
  FlowType color("Color"_view, Structured(fields));
  FlowExpression receiver("color"_view, color);
  Types::Unsigned_64 unsigned_64;
  Types::Boolean flag_type;
  FlowConstant<Constants::Unsigned> start(
      "start"_view, unsigned_64, Unsigned_64(1));
  FlowConstant<Constants::Unsigned> count(
      "count"_view, unsigned_64, Unsigned_64(2));
  FlowConstant<Constants::Unsigned> empty(
      "empty"_view, unsigned_64, Unsigned_64(0));
  FlowConstant<Constants::Unsigned> too_many(
      "too_many"_view, unsigned_64, Unsigned_64(3));
  FlowConstant<Constants::Flag> flag("flag"_view, flag_type, True);
  FlowOwner owner(arena);

  const Abstract& selected = owner.slice(receiver, start, count);
  const Layout& layout = selected.as<Pack>().get_layout();

  EXPECT(selected.is<Packs::Positional>());
  EXPECT_EQ(layout.get_size(), Count(2));
  EXPECT_TEXT(layout.get_abstract(0).get_name(), "g"_view);
  EXPECT_TEXT(layout.get_abstract(1).get_name(), "b"_view);
  EXPECT(
      owner.slice(receiver, start, empty).as<Pack>().get_layout().is_empty());
  EXPECT(owner.slice(receiver, start, too_many).is<Invalid>());
  EXPECT(owner.slice(receiver, flag, count).is<Invalid>());
}
