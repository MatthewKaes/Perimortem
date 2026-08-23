// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/model/type.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/static/vector.hpp"

#include "ttx/concept/invalid.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/model/addressable.hpp"
#include "ttx/model/layouts/named.hpp"
#include "ttx/model/layouts/termination.hpp"

using namespace Perimortem::Core;
using namespace Ttx::Concept;
using namespace Ttx::Model;
using namespace Ttx::Model::Layouts;
using namespace Validation;

/// Registration may reserve a stable Type object before its facts are ready.
/// Resolution remains total by returning Invalid until completion.
class ResolvingType final : public Type {
 public:
  ResolvingType(View::Bytes name, Named layout = Named())
      : name(name), layout(layout) {}

  auto get_name() const -> View::Bytes override { return name; }
  auto get_documentation() const -> const Documentation& override {
    return Documentation::get_empty();
  }
  auto resolve() const -> const Abstract& override {
    if (complete_state) {
      return *this;
    }
    return Invalid::get_invalid();
  }
  auto resolve_context(View::Bytes) const -> const Abstract& override {
    if (complete_state) {
      return *this;
    }
    return Invalid::get_invalid();
  }
  auto get_layout() const -> const Named& override { return layout; }

  auto complete() -> void { complete_state = True; }

 private:
  View::Bytes name;
  Named layout;
  Bool complete_state = False;
};

/// A field is a real Addressable object retained by its owner's layout.
class TypeField final : public Addressable {
 public:
  TypeField(View::Bytes name, const Type& type) : name(name), type(type) {}

  auto get_name() const -> View::Bytes override { return name; }
  auto get_documentation() const -> const Documentation& override {
    return Documentation::get_empty();
  }
  auto resolve_context(View::Bytes) const -> const Abstract& override {
    return Invalid::get_invalid();
  }
  auto get_type() const -> const Type& override { return type; }

 private:
  View::Bytes name;
  const Type& type;
};

class AtomicType final : public Type {
 public:
  TTX_CONTRACT(AtomicType, Type);
  TTX_NAME("Atomic"_view);
  TTX_EMPTY_DOCUMENTATION();
  TTX_INVALID_CONTEXT;
};

class RecursiveType final : public Type {
 public:
  RecursiveType()
      : field("next"_view, *this), fields{{field}}, layout(fields) {}

  TTX_CONTRACT(RecursiveType, Type);
  TTX_NAME("Recursive"_view);
  TTX_EMPTY_DOCUMENTATION();
  TTX_INVALID_CONTEXT;

  auto get_layout() const -> const Named& override { return layout; }

 private:
  TypeField field;
  Static::Vector<Reference<const Abstract>, 1> fields;
  Named layout;
};

static Harness TtxType = {
  .name = "Ttx::Model::Type"_view,
};

PERIMORTEM_UNIT_TEST(TtxType, incomplete_type) {
  ResolvingType reserved("Reserved"_view);
  const Type& type = reserved;

  EXPECT(&type.resolve() == &Invalid::get_invalid());

  reserved.complete();

  EXPECT(&type.resolve() == &reserved);
  EXPECT(type.get_layout().is_empty());
}

PERIMORTEM_UNIT_TEST(TtxType, type_fields) {
  ResolvingType real("R32"_view);
  real.complete();
  TypeField x("x"_view, real);
  TypeField y("y"_view, real);
  const Static::Vector<Reference<const Abstract>, 2> fields = {{x, y}};
  ResolvingType point("Point"_view, Named(fields));

  EXPECT(&point.resolve() == &Invalid::get_invalid());

  point.complete();

  const Addressable* first = point.get_layout().get_abstract(0).visit(
      []() { return static_cast<const Addressable*>(nullptr); },
      [](const Abstract& abstract) {
        return abstract.visit<Addressable>(
            [](const Addressable& addressable) { return &addressable; },
            [](const Abstract&) {
              return static_cast<const Addressable*>(nullptr);
            });
      });
  ASSERT(first != nullptr);
  EXPECT(first == &x);
  EXPECT(point.get_layout().get_abstract(1).visit(
      []() { return False; },
      [&y](const Abstract& selected) {
        return &selected == &y ? True : False;
      }));
  EXPECT(&first->resolve() == first);
  EXPECT(&first->get_type().resolve() == &real);
}

PERIMORTEM_UNIT_TEST(TtxType, layout_termination) {
  AtomicType atomic;
  ResolvingType empty("Empty"_view);
  empty.complete();
  TypeField value("value"_view, atomic);
  const Static::Vector<Reference<const Abstract>, 1> fields = {{value}};
  ResolvingType aggregate("Aggregate"_view, Named(fields));
  aggregate.complete();
  RecursiveType recursive;

  EXPECT(is_terminating(atomic));
  EXPECT_NOT(is_terminating(empty));
  EXPECT(is_terminating(aggregate));
  EXPECT_NOT(is_terminating(recursive));
}
