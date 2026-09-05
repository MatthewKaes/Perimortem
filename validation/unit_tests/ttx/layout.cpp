// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "validation/unit_test.hpp"

#include "perimortem/core/static/vector.hpp"

#include "tetrodotoxin/language/binding.hpp"
#include "ttx/ffi/cpp/domain.hpp"
#include "ttx/ffi/cpp/addressable.hpp"
#include "ttx/reference/model/layouts/composite.hpp"
#include "ttx/reference/model/layouts/fluid.hpp"
#include "ttx/reference/model/layouts/named.hpp"
#include "ttx/reference/model/layouts/ranged.hpp"
#include "ttx/reference/model/layouts/value.hpp"
#include "ttx/concept/unknown.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Utility;
using namespace Ttx::Concept;
using namespace Ttx::Model;
using namespace Ttx::Model::Layouts;
using namespace Validation;
using Tetrodotoxin::Language::Binding;

/// These test Domains expose stable shapes without lending the Layout any of
/// the declaration machinery that produced them.
class LayoutDomain final : public Domain {
 public:
  LayoutDomain(View::Bytes name, Named layout = Named())
      : name(name), layout(layout) {}

  auto get_name() const -> View::Bytes override { return name; }
  auto get_documentation() const -> const Documentation& override {
    return Documentation::get_empty();
  }
  auto resolve_concept(View::Bytes) const -> const Abstract& override {
    return Unknown::get_unknown();
  }
  auto get_layout() const -> const Named& override { return layout; }

 private:
  View::Bytes name;
  Named layout;
};

/// Named layouts retain fields as Addressable identities instead of copying
/// their names and Domains into a parallel member model.
class LayoutField final : public Addressable {
 public:
  LayoutField(View::Bytes name, const Domain& domain)
      : name(name), domain(domain) {}

  auto get_name() const -> View::Bytes override { return name; }
  auto get_documentation() const -> const Documentation& override {
    return Documentation::get_empty();
  }
  auto resolve_concept(View::Bytes) const -> const Abstract& override {
    return Unknown::get_unknown();
  }
  auto get_domain() const -> const Domain& override { return domain; }

 private:
  View::Bytes name;
  const Domain& domain;
};

/// A fixed byte sequence can expose one repeated Domain and an extent instead
/// of manufacturing a separate semantic identity for every byte position.
class ByteSequence final : public Domain {
 public:
  ByteSequence(const Domain& byte, Count size) : layout(byte, size) {}

  auto get_name() const -> View::Bytes override { return "ByteSequence"_view; }
  auto get_documentation() const -> const Documentation& override {
    return Documentation::get_empty();
  }
  auto resolve_concept(View::Bytes) const -> const Abstract& override {
    return Unknown::get_unknown();
  }
  auto get_layout() const -> const Ranged& override { return layout; }

 private:
  Ranged layout;
};

static Harness TtxLayout = {
  .name = "Ttx::Model::Layout"_view,
};

class AtomicDomain : public Domain {
 public:
  auto get_name() const -> View::Bytes override { return "Atomic"_view; }
  auto get_documentation() const -> const Documentation& override {
    return Documentation::get_empty();
  }
  auto resolve_concept(View::Bytes) const -> const Abstract& override {
    return Unknown::get_unknown();
  }
};

/// Layout fitting may compare a reserved Domain before its owner has completed
/// the declaration. Its exact identity remains meaningful even while resolve()
/// reports Unknown.
class StagedDomain final : public AtomicDomain {
 public:
  auto resolve() const -> const Abstract& override {
    return Unknown::get_unknown();
  }
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

PERIMORTEM_UNIT_TEST(TtxLayout, value_terminal) {
  AtomicDomain atomic;
  const Layout& layout = atomic.get_layout();

  EXPECT_EQ(layout.get_size(), Count(1));
  EXPECT(selects(layout.get_abstract(0), atomic));
  EXPECT(layout.fits(layout));
  EXPECT(selects(layout.get_fitted(layout, 0), atomic));
}

PERIMORTEM_UNIT_TEST(TtxLayout, fluid_order) {
  LayoutDomain real("R32"_view);
  LayoutDomain bits("U32"_view);
  const Static::Vector<const Abstract*, 2> values = {{&real, &bits}};
  Fluid layout(values);

  EXPECT_EQ(layout.get_size(), Count(2));
  EXPECT(selects(layout.get_abstract(0), real));
  EXPECT(selects(layout.get_abstract(1), bits));
  EXPECT(is_none(layout.get_abstract(2)));
}

PERIMORTEM_UNIT_TEST(TtxLayout, staged_identity) {
  StagedDomain first;
  StagedDomain second;
  LayoutField first_field("first"_view, first);
  LayoutField second_field("second"_view, second);
  const Static::Vector<const Abstract*, 1> first_type = {{&first}};
  const Static::Vector<const Abstract*, 1> second_type = {{&second}};
  const Static::Vector<const Abstract*, 1> first_addressable = {{&first_field}};
  const Static::Vector<const Abstract*, 1> second_addressable = {
    {&second_field}};
  Fluid fluid_first(first_type);
  Fluid fluid_same(first_type);
  Fluid fluid_second(second_type);
  Fluid field_first(first_addressable);
  Fluid field_second(second_addressable);
  Ranged ranged_first(first, 2);
  Ranged ranged_same(first, 2);
  Ranged ranged_second(second, 2);

  EXPECT(fluid_first.fits(fluid_same));
  EXPECT_NOT(fluid_first.fits(fluid_second));
  EXPECT_NOT(field_first.fits(field_second));
  EXPECT(ranged_first.fits(ranged_same));
  EXPECT_NOT(ranged_first.fits(ranged_second));
}

PERIMORTEM_UNIT_TEST(TtxLayout, named_fields) {
  Perimortem::Memory::Allocator::Arena arena;
  LayoutDomain real("R32"_view);
  LayoutDomain bits("U32"_view);
  LayoutField x("x"_view, real);
  LayoutField y("y"_view, bits);
  const Static::Vector<const Abstract*, 2> fields = {{&x, &y}};
  Named layout(arena, fields);

  EXPECT_EQ(layout.get_size(), Count(2));
  EXPECT(selects(layout.get_abstract(0), x));
  EXPECT(selects(layout.get_abstract(1), y));
  EXPECT_TEXT(x.get_name(), "x"_view);
  EXPECT(&x.resolve() == &x);
  EXPECT(&x.get_domain().resolve() == &real);
  EXPECT(&y.get_domain().resolve() == &bits);
  EXPECT(is_none(layout.get_abstract(2)));
}

PERIMORTEM_UNIT_TEST(TtxLayout, ranged_layout) {
  LayoutDomain byte("U8"_view);
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
  LayoutDomain byte("U8"_view);
  Binding first("first"_view, byte);
  Binding second("second"_view, byte);
  Binding third("third"_view, byte);
  Binding fourth("fourth"_view, byte);
  const Static::Vector<const Abstract*, 4> values = {{
    &first,
    &second,
    &third,
    &fourth,
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
  Perimortem::Memory::Allocator::Arena arena;
  LayoutDomain byte("U8"_view);
  LayoutDomain real("R32"_view);
  LayoutDomain bits("U32"_view);
  LayoutField x("x"_view, real);
  LayoutField y("y"_view, bits);
  Binding named_x("x"_view, real);
  Binding named_y("y"_view, bits);
  const Static::Vector<const Abstract*, 2> fields = {{&x, &y}};
  const Static::Vector<const Abstract*, 2> values = {{&named_y, &named_x}};
  Ranged source_prefix(byte, 2);
  Ranged target_prefix(byte, 2);
  Named source_suffix(arena, values);
  Named target_suffix(arena, fields);
  Composite source(source_prefix, source_suffix);
  Composite target(target_prefix, target_suffix);

  EXPECT(source.fits(target));
  EXPECT(selects(source.get_fitted(target, 2), named_x));
  EXPECT(selects(source.get_fitted(target, 3), named_y));
}

PERIMORTEM_UNIT_TEST(TtxLayout, fitting_contracts) {
  Perimortem::Memory::Allocator::Arena arena;
  LayoutDomain real("R32"_view);
  LayoutDomain bits("U32"_view);
  LayoutField x("x"_view, real);
  LayoutField y("y"_view, bits);
  Binding named_x("x"_view, real);
  Binding named_y("y"_view, bits);
  const Static::Vector<const Abstract*, 2> fields = {{&x, &y}};
  const Static::Vector<const Abstract*, 2> positional = {{&real, &bits}};
  const Static::Vector<const Abstract*, 2> reordered = {{&named_y, &named_x}};
  const Static::Vector<const Abstract*, 2> lexical_values = {{&bits, &real}};
  const Static::Vector<View::Bytes, 2> lexical_names = {{"y"_view, "x"_view}};
  Named target(arena, fields);
  Fluid fluid(positional);
  Named named(arena, reordered);
  Fluid lexical_flow(lexical_values);
  Named lexical(arena, lexical_flow, lexical_names);

  EXPECT(fluid.fits(target));
  EXPECT(named.fits(target));
  EXPECT(lexical.fits(target));
  EXPECT(target.fits(target));
  EXPECT(selects(fluid.get_fitted(target, 0), real));
  EXPECT(selects(named.get_fitted(target, 0), named_x));
  EXPECT(selects(named.get_fitted(target, 1), named_y));
  EXPECT(selects(lexical.get_fitted(target, 0), real));
  EXPECT(selects(lexical.get_fitted(target, 1), bits));
  EXPECT(selects(target.get_fitted(target, 1), y));
  EXPECT(
      reports(fluid.get_fitted(target, 2), Layout::Errors::IndexOutOfBounds));
}

PERIMORTEM_UNIT_TEST(TtxLayout, alias_fitting) {
  Perimortem::Memory::Allocator::Arena arena;
  LayoutDomain real("R32"_view);
  LayoutField x("x"_view, real);
  Binding alias("x"_view, x);
  const Static::Vector<const Abstract*, 1> aliases = {{&alias}};
  const Static::Vector<const Abstract*, 1> fields = {{&x}};
  Fluid fluid_source(aliases);
  Fluid fluid_target(fields);
  Named named_source(arena, aliases);
  Named named_target(arena, fields);
  Ranged ranged_source(alias, 2);
  Ranged ranged_target(x, 2);

  EXPECT(fluid_source.fits(fluid_target));
  EXPECT(named_source.fits(named_target));
  EXPECT(ranged_source.fits(ranged_target));
  EXPECT(selects(named_source.get_fitted(named_target, 0), alias));
}

PERIMORTEM_UNIT_TEST(TtxLayout, named_ambiguity) {
  Perimortem::Memory::Allocator::Arena arena;
  LayoutDomain real("R32"_view);
  LayoutDomain bits("U32"_view);
  LayoutField x("x"_view, real);
  LayoutField y("y"_view, bits);
  Binding first("x"_view, real);
  Binding duplicate("x"_view, bits);
  Binding unnamed({}, real);
  const Static::Vector<const Abstract*, 2> fields = {{&x, &y}};
  const Static::Vector<const Abstract*, 2> values = {{&first, &duplicate}};
  const Static::Vector<const Abstract*, 2> empty_names = {{&first, &unnamed}};
  Named target(arena, fields);
  Named named(arena, values);
  Named nameless(arena, empty_names);

  EXPECT_NOT(named.fits(target));
  EXPECT(reports(named.get_fitted(target, 0), Layout::Errors::IncompatibleFit));
  EXPECT_NOT(nameless.fits(target));
  EXPECT(
      reports(nameless.get_fitted(target, 0), Layout::Errors::IncompatibleFit));
}

PERIMORTEM_UNIT_TEST(TtxLayout, named_shape) {
  Perimortem::Memory::Allocator::Arena arena;
  LayoutDomain real("R32"_view);
  LayoutField first_x("x"_view, real);
  LayoutField second_x("x"_view, real);
  const Static::Vector<const Abstract*, 1> first_fields = {{&first_x}};
  const Static::Vector<const Abstract*, 1> same_fields = {{&first_x}};
  const Static::Vector<const Abstract*, 1> other_fields = {{&second_x}};
  Named first(arena, first_fields);
  Named same(arena, same_fields);
  Named other(arena, other_fields);

  EXPECT(first.fits(same));
  EXPECT(first.fits(other));
  EXPECT(selects(first.get_fitted(other, 0), first_x));
  EXPECT(selects(first.get_fitted_at(other, 0, 0), first_x));
  EXPECT(
      reports(first.get_fitted_at(same, 1, 0), Layout::Errors::SizeMismatch));
}
