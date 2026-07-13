// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/isa/base/implementation.hpp"

#include "validation/unit_test.hpp"

#include "tetrodotoxin/compiler/execution/body.hpp"
#include "tetrodotoxin/isa/base/context.hpp"
#include "tetrodotoxin/isa/shader/block.hpp"
#include "ttx/type.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin;
using namespace Validation;

static Harness TtxImplementation = {
  .name = "Tetrodotoxin::Impl"_view,
};

PERIMORTEM_UNIT_TEST(TtxImplementation, missing) {
  Isa::Base::Implementation implementation;
  Ttx::Function function("main"_view, Ttx::Layout(), Ttx::Layout());

  EXPECT_NOT(implementation.has(function));
  EXPECT(implementation.find<Compiler::Execution::Body>(function) == nullptr);
}

PERIMORTEM_UNIT_TEST(TtxImplementation, define) {
  Isa::Base::Implementation implementation;
  Compiler::Execution::Body body({}, {}, {}, {});
  Ttx::Function function("main"_view, Ttx::Layout(), Ttx::Layout());

  EXPECT(implementation.define(function, body));
  EXPECT(implementation.has(function));
  const Compiler::Execution::Body* found =
      implementation.find<Compiler::Execution::Body>(function);
  ASSERT(found != nullptr);
  EXPECT(found == &body);
  EXPECT(implementation.find<Isa::Shader::Block>(function) == nullptr);
}

PERIMORTEM_UNIT_TEST(TtxImplementation, identity) {
  Isa::Base::Implementation implementation;
  Compiler::Execution::Body first({}, {}, {}, {});
  Compiler::Execution::Body second({}, {}, {}, {});
  Ttx::Function first_function("run"_view, Ttx::Layout(), Ttx::Layout());
  Ttx::Function second_function("run"_view, Ttx::Layout(), Ttx::Layout());

  EXPECT(implementation.define(first_function, first));
  EXPECT(implementation.define(second_function, second));
  const Compiler::Execution::Body* first_body =
      implementation.find<Compiler::Execution::Body>(first_function);
  const Compiler::Execution::Body* second_body =
      implementation.find<Compiler::Execution::Body>(second_function);
  ASSERT(first_body != nullptr);
  ASSERT(second_body != nullptr);
  EXPECT(first_body == &first);
  EXPECT(second_body == &second);
}

PERIMORTEM_UNIT_TEST(TtxImplementation, linkage) {
  Isa::Base::Implementation implementation;
  Ttx::Function function("print"_view, Ttx::Layout(), Ttx::Layout());
  Static::Vector<Ttx::Function, 1> functions = {{function}};
  Ttx::Type foreign("Console"_view, Ttx::Layout(), {}, functions);
  const Ttx::Function& published = foreign.get_functions()[0];

  EXPECT(implementation.define(
      Tetrodotoxin::Compiler::Linkage(foreign, published, "print"_view)));
  const auto* linkage = implementation.find_linkage(published);
  ASSERT(linkage != nullptr);
  EXPECT(&linkage->get_owner() == &foreign);
  EXPECT_TEXT(linkage->get_symbol(), "print"_view);
  EXPECT_NOT(implementation.define(
      Tetrodotoxin::Compiler::Linkage(foreign, published, "other"_view)));
}

PERIMORTEM_UNIT_TEST(TtxImplementation, imported_call) {
  Perimortem::Memory::Allocator::Arena arena;
  Isa::Base::Implementation producer;
  Isa::Base::Implementation consumer;
  Isa::Base::Context context(arena, &consumer);
  Ttx::Function function("print"_view, Ttx::Layout(), Ttx::Layout());
  Static::Vector<Ttx::Function, 1> functions = {{function}};
  Ttx::Type foreign("Console"_view, Ttx::Layout(), {}, functions);
  const Ttx::Function& published = foreign.get_functions()[0];

  EXPECT(producer.define(
      Tetrodotoxin::Compiler::Linkage(foreign, published, "print"_view)));
  EXPECT(context.find_linkage(published) == nullptr);
  context.import_implementation(producer);
  const auto* linkage = context.find_linkage(published);
  ASSERT(linkage != nullptr);
  EXPECT_TEXT(linkage->get_symbol(), "print"_view);
}

PERIMORTEM_UNIT_TEST(TtxImplementation, member_facts) {
  Isa::Base::Implementation implementation;
  Ttx::Type type("Bits_8"_view);
  Ttx::Member member("red"_view, type);
  Ttx::Attribute source_attribute("range"_view, "byte"_view);
  Static::Vector<Ttx::Attribute, 1> attributes = {{
    source_attribute,
  }};
  Isa::Base::Expression::Value initializer =
      Isa::Base::Expression::Value::numeric("1"_view);

  EXPECT(implementation.define(
      member,
      Isa::Base::Definition(
          Ttx::Lexical::Class::Type::Expose, attributes, &initializer)));
  const Isa::Base::Definition* facts = implementation.find(member);
  ASSERT(facts != nullptr);
  EXPECT(facts->get_modifier() == Ttx::Lexical::Class::Type::Expose);
  ASSERT(facts->get_initializer() != nullptr);
  EXPECT(
      facts->get_initializer()->get_kind() ==
      Isa::Base::Expression::Value::Kind::Numeric);
  EXPECT_TEXT(facts->get_initializer()->get_value(), "1"_view);
  ASSERT_EQ(facts->get_attributes().get_size(), Count(1));
  EXPECT_TEXT(facts->get_attributes()[0].get_key(), "range"_view);
}
