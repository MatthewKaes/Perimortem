// Perimortem Engine
// Copyright © Matt Kaes

#include "validation/unit_test.hpp"

#include "perimortem/core/algorithm/search.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/dynamic/bytes.hpp"

#include "tetrodotoxin/linker/linker.hpp"
#include "tetrodotoxin/puffer/resolution/resolver.hpp"
#include "tetrodotoxin/puffer/terminal/plan.hpp"
#include "tetrodotoxin/toolchain.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Puffer;
using namespace Tetrodotoxin::Puffer::Resolution;
using namespace Validation;

static Harness TerminalPlan = {
  .name = "Tetrodotoxin::Terminal::Plan"_view,
};

PERIMORTEM_UNIT_TEST(TerminalPlan, package_library) {
  Tetrodotoxin::Toolchain toolchain = Tetrodotoxin::Toolchain::standard();
  Resolver resolver(toolchain);
  Resolver::Context logic_source_context;

  ASSERT(resolver.load_source(
      logic_source_context, "unit/logic.ttx"_view,
      "dialect : Library;\n"
      "public Console : foreign {\n"
      "  expose func print[.data : View[Bytes]] -> [];\n"
      "}\n"
      "public func hello[.data : View[Bytes]] -> [] {\n"
      "  Console->print(data);\n"
      "  return;\n"
      "}\n"_view));
  EXPECT_NOT(logic_source_context.has_errors());
  Resolver::Context package_source_context;

  Source::Record* package = resolver.load_source(
      package_source_context, "unit/package.ttx"_view,
      "dialect : Package;\n"
      "import Logic : Library = \"logic.ttx\";\n"
      "@package_name = Test::Terminal;\n"_view);
  ASSERT(package != nullptr);
  EXPECT_NOT(package_source_context.has_errors());

  Allocator::Arena arena;
  Terminal::Plan plan(arena);
  plan.add_package(resolver, *package);
  ASSERT(plan.lower());

  Tetrodotoxin::Linker::Linker linker;
  linker.add(plan.get_library());
  linker.add(plan.get_shader());
  Dynamic::Bytes archive = linker.build_library("terminal.o"_view);
  EXPECT(
      Algorithm::search(archive.get_view(), "TTX_logic_hello"_view) !=
      Count(-1));

  Dynamic::Bytes header;
  linker.append_header(header, plan.get_library());
  EXPECT(
      Algorithm::search(header.get_view(), "TTX_logic_hello"_view) !=
      Count(-1));

  EXPECT(
      Algorithm::search(
          plan.get_puffer_buffer(),
          "function: Library->hello params=1 result=0 blocks=1"_view) !=
      Count(-1));
}
