// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/ffi/cpp/domain.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/static/vector.hpp"

#include "ttx/concept/unknown.hpp"
#include "ttx/ffi/cpp/addressable.hpp"
#include "ttx/reference/model/layouts/named.hpp"
#include "ttx/reference/model/layouts/termination.hpp"

using namespace Perimortem::Core;
using namespace Ttx::Concept;
using namespace Ttx::Model;
using namespace Ttx::Model::Layouts;
using namespace Validation;

/// A language may reserve a stable Domain before its declaration has exposed
/// enough structure to use it. The same identity answers Unknown until the
/// owner completes that declaration, so callers never need a mutable stand-in.
class ResolvingDomain final : public Domain {
 public:
  ResolvingDomain(View::Bytes name, Named layout = Named())
      : name(name), layout(layout) {}

  auto get_name() const -> View::Bytes override { return name; }
  auto get_documentation() const -> const Documentation& override {
    return Documentation::get_empty();
  }
  auto resolve() const -> const Abstract& override {
    if (complete_state) {
      return *this;
    }
    return Unknown::get_unknown();
  }
  auto resolve_concept(View::Bytes) const -> const Abstract& override {
    if (complete_state) {
      return *this;
    }
    return Unknown::get_unknown();
  }
  auto get_layout() const -> const Named& override { return layout; }

  auto complete() -> void { complete_state = True; }

 private:
  View::Bytes name;
  Named layout;
  Bool complete_state = False;
};

/// A field is a real Addressable object retained by its owner's layout.
class DomainField final : public Addressable {
 public:
  DomainField(View::Bytes name, const Domain& domain)
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

class AtomicDomain final : public Domain {
 public:
  TTX_NAME("Atomic"_view);
  TTX_EMPTY_DOCUMENTATION();
};

class RecursiveDomain final : public Domain {
 public:
  RecursiveDomain()
      : field("next"_view, *this), fields{{&field}}, layout(arena, fields) {}

  TTX_NAME("Recursive"_view);
  TTX_EMPTY_DOCUMENTATION();

  auto get_layout() const -> const Named& override { return layout; }

 private:
  Perimortem::Memory::Allocator::Arena arena;
  DomainField field;
  Static::Vector<const Abstract*, 1> fields;
  Named layout;
};

static Harness TtxDomain = {
  .name = "Ttx::Model::Domain"_view,
};

PERIMORTEM_UNIT_TEST(TtxDomain, incomplete_domain) {
  ResolvingDomain reserved("Reserved"_view);
  const Domain& domain = reserved;

  EXPECT(&domain.resolve() == &Unknown::get_unknown());

  reserved.complete();

  EXPECT(&domain.resolve() == &reserved);
  EXPECT(domain.get_layout().is_empty());
}

PERIMORTEM_UNIT_TEST(TtxDomain, domain_fields) {
  Perimortem::Memory::Allocator::Arena arena;
  ResolvingDomain real("R32"_view);
  real.complete();
  DomainField x("x"_view, real);
  DomainField y("y"_view, real);
  const Static::Vector<const Abstract*, 2> fields = {{&x, &y}};
  ResolvingDomain point("Point"_view, Named(arena, fields));

  EXPECT(&point.resolve() == &Unknown::get_unknown());

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
  EXPECT(&first->get_domain().resolve() == &real);
}

PERIMORTEM_UNIT_TEST(TtxDomain, layout_termination) {
  Perimortem::Memory::Allocator::Arena arena;
  AtomicDomain atomic;
  ResolvingDomain empty("Empty"_view);
  empty.complete();
  DomainField value("value"_view, atomic);
  const Static::Vector<const Abstract*, 1> fields = {{&value}};
  ResolvingDomain aggregate("Aggregate"_view, Named(arena, fields));
  aggregate.complete();
  RecursiveDomain recursive;

  EXPECT(is_terminating(atomic));
  EXPECT_NOT(is_terminating(empty));
  EXPECT(is_terminating(aggregate));
  EXPECT_NOT(is_terminating(recursive));
}
