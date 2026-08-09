// Perimortem Engine
// Copyright © Matt Kaes

#include "validation/unit_test.hpp"

#include "perimortem/core/static/vector.hpp"

#include "ttx/concept/invalid.hpp"
#include "ttx/model/addressable.hpp"
#include "ttx/model/alias.hpp"
#include "ttx/model/layouts/composite.hpp"
#include "ttx/model/layouts/fluid.hpp"
#include "ttx/model/layouts/named.hpp"
#include "ttx/model/layouts/ranged.hpp"
#include "ttx/model/type.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Utility;
using namespace Ttx::Concept;
using namespace Ttx::Model;
using namespace Ttx::Model::Layouts;
using namespace Validation;

/// A Type owns its stable shape while Layout only exposes ordered Abstracts.
class LayoutType final : public Type {
 public:
  LayoutType(View::Bytes name, Named layout = Named())
      : name(name), layout(layout) {}

  auto get_name() const -> View::Bytes override { return name; }
  auto get_documentation() const -> const Documentation& override {
    return Documentation::get_empty();
  }
  auto resolve_context(View::Bytes) const -> const Abstract& override {
    return Invalid::get_invalid();
  }
  auto get_layout() const -> const Named& override { return layout; }

 private:
  View::Bytes name;
  Named layout;
};

/// Named layouts retain fields as Addressable facts instead of copying their
/// names and Types into a parallel member model.
class LayoutField final : public Addressable {
 public:
  LayoutField(View::Bytes name, const Type& type) : name(name), type(type) {}

  auto get_name() const -> View::Bytes override { return name; }
  auto get_documentation() const -> const Documentation& override {
    return Documentation::get_empty();
  }
  auto get_type() const -> const Type& override { return type; }

 private:
  View::Bytes name;
  const Type& type;
};

/// A fixed byte sequence is a Type whose complete indexed shape is one compact
/// Ranged layout rather than N copied fields.
class ByteSequence final : public Type {
 public:
  ByteSequence(const Type& byte, Count size) : layout(byte, size) {}

  auto get_name() const -> View::Bytes override { return "ByteSequence"_view; }
  auto get_documentation() const -> const Documentation& override {
    return Documentation::get_empty();
  }
  auto resolve_context(View::Bytes) const -> const Abstract& override {
    return Invalid::get_invalid();
  }
  auto get_layout() const -> const Ranged& override { return layout; }

 private:
  Ranged layout;
};

static Harness TtxLayout = {
  .name = "Ttx::Model::Layout"_view,
};

static auto selects(
    const Perimortem::Core::Option<const Abstract&>& result,
    const Abstract& expected) -> Bool {
  return result.visit(
      []() { return False; },
      [&expected](const Abstract& selected) {
        return &selected == &expected ? True : False;
      });
}

static auto selects(
    const Result<const Abstract&, Layout::Errors>& result,
    const Abstract& expected) -> Bool {
  return result.visit(
      [&](const Abstract& selected) { return &selected == &expected; },
      [](Layout::Errors) { return false; });
}

static auto is_none(const Perimortem::Core::Option<const Abstract&>& result)
    -> Bool {
  return result.visit(
      []() { return True; }, [](const Abstract&) { return False; });
}

static auto reports(
    const Result<const Abstract&, Layout::Errors>& result,
    Layout::Errors expected) -> Bool {
  return result.visit(
      [](const Abstract&) { return false; },
      [&](Layout::Errors error) { return error == expected; });
}

PERIMORTEM_UNIT_TEST(TtxLayout, fluid_order) {
  LayoutType real("Real_32"_view);
  LayoutType bits("Unsigned_32"_view);
  const Static::Vector<Reference<const Abstract>, 2> values = {{real, bits}};
  Fluid layout(values);

  EXPECT_EQ(layout.get_size(), Count(2));
  EXPECT(selects(layout.get_abstract(0), real));
  EXPECT(selects(layout.get_abstract(1), bits));
  EXPECT(is_none(layout.get_abstract(2)));
}

PERIMORTEM_UNIT_TEST(TtxLayout, named_fields) {
  LayoutType real("Real_32"_view);
  LayoutType bits("Unsigned_32"_view);
  LayoutField x("x"_view, real);
  LayoutField y("y"_view, bits);
  const Static::Vector<Reference<const Abstract>, 2> fields = {{x, y}};
  Named layout(fields);

  EXPECT_EQ(layout.get_size(), Count(2));
  EXPECT(selects(layout.get_abstract(0), x));
  EXPECT(selects(layout.get_abstract(1), y));
  EXPECT_TEXT(x.get_name(), "x"_view);
  EXPECT(&x.resolve() == &x);
  EXPECT(&x.get_type().resolve() == &real);
  EXPECT(&y.get_type().resolve() == &bits);
  EXPECT(is_none(layout.get_abstract(2)));
}

PERIMORTEM_UNIT_TEST(TtxLayout, ranged_materialization) {
  LayoutType byte("Unsigned_8"_view);
  ByteSequence bytes(byte, 16);
  Ranged same(byte, 16);
  Ranged shorter(byte, 15);

  const Layout& layout = bytes.get_layout();
  EXPECT_EQ(layout.get_size(), Count(16));
  EXPECT(selects(layout.get_abstract(0), byte));
  EXPECT(selects(layout.get_abstract(10), byte));
  EXPECT(is_none(layout.get_abstract(16)));
  EXPECT(layout.fits(same));
  EXPECT_NOT(layout.fits(shorter));
  EXPECT(selects(layout.get_fitted(same, 10), byte));
  EXPECT(
      reports(layout.get_fitted(same, 16), Layout::Errors::IndexOutOfBounds));
  EXPECT(reports(layout.get_fitted(shorter, 0), Layout::Errors::SizeMismatch));
  EXPECT(reports(
      layout.get_fitted_at(same, 0, 16), Layout::Errors::IndexOutOfBounds));
  EXPECT(reports(
      layout.get_fitted_at(shorter, 0, 0), Layout::Errors::SizeMismatch));
}

PERIMORTEM_UNIT_TEST(TtxLayout, composite_components) {
  LayoutType byte("Unsigned_8"_view);
  Alias first("first"_view, byte);
  Alias second("second"_view, byte);
  Alias third("third"_view, byte);
  Alias fourth("fourth"_view, byte);
  const Static::Vector<Reference<const Abstract>, 4> values = {{
    first,
    second,
    third,
    fourth,
  }};
  Ranged prefix(byte, 32);
  Fluid suffix(values);
  Composite composite(prefix, suffix);
  Ranged target(byte, 36);
  Ranged shorter(byte, 35);

  EXPECT_EQ(composite.get_size(), Count(36));
  EXPECT(selects(composite.get_abstract(31), byte));
  EXPECT(selects(composite.get_abstract(32), first));
  EXPECT(selects(composite.get_abstract(35), fourth));
  EXPECT(is_none(composite.get_abstract(36)));
  EXPECT(composite.fits(target));
  EXPECT_NOT(composite.fits(shorter));
  EXPECT(selects(composite.get_fitted(target, 31), byte));
  EXPECT(selects(composite.get_fitted(target, 32), first));
  EXPECT(selects(composite.get_fitted(target, 35), fourth));
  EXPECT(reports(
      composite.get_fitted(target, 36), Layout::Errors::IndexOutOfBounds));
  EXPECT(
      reports(composite.get_fitted(shorter, 0), Layout::Errors::SizeMismatch));
}

PERIMORTEM_UNIT_TEST(TtxLayout, composite_fitting) {
  LayoutType byte("Unsigned_8"_view);
  LayoutType real("Real_32"_view);
  LayoutType bits("Unsigned_32"_view);
  LayoutField x("x"_view, real);
  LayoutField y("y"_view, bits);
  Alias named_x("x"_view, real);
  Alias named_y("y"_view, bits);
  const Static::Vector<Reference<const Abstract>, 2> fields = {{x, y}};
  const Static::Vector<Reference<const Abstract>, 2> values = {
    {named_y, named_x}};
  Ranged source_prefix(byte, 2);
  Ranged target_prefix(byte, 2);
  Named source_suffix(values);
  Named target_suffix(fields);
  Composite source(source_prefix, source_suffix);
  Composite target(target_prefix, target_suffix);

  EXPECT(source.fits(target));
  EXPECT(selects(source.get_fitted(target, 2), named_x));
  EXPECT(selects(source.get_fitted(target, 3), named_y));
}

PERIMORTEM_UNIT_TEST(TtxLayout, fitting_contracts) {
  LayoutType real("Real_32"_view);
  LayoutType bits("Unsigned_32"_view);
  LayoutField x("x"_view, real);
  LayoutField y("y"_view, bits);
  Alias named_x("x"_view, real);
  Alias named_y("y"_view, bits);
  const Static::Vector<Reference<const Abstract>, 2> fields = {{x, y}};
  const Static::Vector<Reference<const Abstract>, 2> positional = {
    {real, bits}};
  const Static::Vector<Reference<const Abstract>, 2> reordered = {
    {named_y, named_x}};
  Named target(fields);
  Fluid fluid(positional);
  Named named(reordered);

  EXPECT(fluid.fits(target));
  EXPECT(named.fits(target));
  EXPECT(target.fits(target));
  EXPECT(selects(fluid.get_fitted(target, 0), real));
  EXPECT(selects(named.get_fitted(target, 0), named_x));
  EXPECT(selects(named.get_fitted(target, 1), named_y));
  EXPECT(selects(target.get_fitted(target, 1), y));
  EXPECT(
      reports(fluid.get_fitted(target, 2), Layout::Errors::IndexOutOfBounds));
}

PERIMORTEM_UNIT_TEST(TtxLayout, addressable_alias_fitting) {
  LayoutType real("Real_32"_view);
  LayoutField x("x"_view, real);
  Alias alias("x"_view, x);
  const Static::Vector<Reference<const Abstract>, 1> aliases = {{alias}};
  const Static::Vector<Reference<const Abstract>, 1> fields = {{x}};
  Fluid fluid_source(aliases);
  Fluid fluid_target(fields);
  Named named_source(aliases);
  Named named_target(fields);
  Ranged ranged_source(alias, 2);
  Ranged ranged_target(x, 2);

  EXPECT(fluid_source.fits(fluid_target));
  EXPECT(named_source.fits(named_target));
  EXPECT(ranged_source.fits(ranged_target));
  EXPECT(selects(named_source.get_fitted(named_target, 0), alias));
}

PERIMORTEM_UNIT_TEST(TtxLayout, named_ambiguity) {
  LayoutType real("Real_32"_view);
  LayoutType bits("Unsigned_32"_view);
  LayoutField x("x"_view, real);
  LayoutField y("y"_view, bits);
  Alias first("x"_view, real);
  Alias duplicate("x"_view, bits);
  Alias unnamed({}, real);
  const Static::Vector<Reference<const Abstract>, 2> fields = {{x, y}};
  const Static::Vector<Reference<const Abstract>, 2> values = {
    {first, duplicate}};
  const Static::Vector<Reference<const Abstract>, 2> empty_names = {
    {first, unnamed}};
  Named target(fields);
  Named named(values);
  Named nameless(empty_names);

  EXPECT_NOT(named.fits(target));
  EXPECT(reports(named.get_fitted(target, 0), Layout::Errors::IncompatibleFit));
  EXPECT_NOT(nameless.fits(target));
  EXPECT(
      reports(nameless.get_fitted(target, 0), Layout::Errors::IncompatibleFit));
}

PERIMORTEM_UNIT_TEST(TtxLayout, named_shape) {
  LayoutType real("Real_32"_view);
  LayoutField first_x("x"_view, real);
  LayoutField second_x("x"_view, real);
  const Static::Vector<Reference<const Abstract>, 1> first_fields = {{first_x}};
  const Static::Vector<Reference<const Abstract>, 1> same_fields = {{first_x}};
  const Static::Vector<Reference<const Abstract>, 1> other_fields = {
    {second_x}};
  Named first(first_fields);
  Named same(same_fields);
  Named other(other_fields);

  EXPECT(first.fits(same));
  EXPECT(first.fits(other));
  EXPECT(selects(first.get_fitted(other, 0), first_x));
  EXPECT(selects(first.get_fitted_at(other, 0, 0), first_x));
  EXPECT(
      reports(first.get_fitted_at(same, 1, 0), Layout::Errors::SizeMismatch));
}
