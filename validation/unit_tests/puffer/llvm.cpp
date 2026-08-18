// Perimortem Engine
// Copyright © Matt Kaes

#include "validation/process/child.hpp"
#include "validation/unit_test.hpp"

#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/algorithm/search.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "perimortem/system/file.hpp"

#include "tetrodotoxin/environment/workspace.hpp"
#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "tetrodotoxin/library/llvm/compiler.hpp"
#include "ttx/lexical/errors.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::System;
using namespace Tetrodotoxin;
using namespace Ttx::Lexical;
using namespace Validation;

static Harness LlvmTests = {
  .name = "Tetrodotoxin::Library::Llvm"_view,
};

extern "C" auto run_foreign_integration() -> int;
extern "C" auto run_runtime_integration() -> int;

static auto compile_source(
    Allocator::Arena& products,
    Errors& errors,
    View::Bytes path,
    View::Bytes source,
    Library::Llvm::Debug::Level debug) -> Perimortem::Utility::
    Result<Library::Llvm::Products, Library::Llvm::Failure> {
  Environment::Workspace workspace;
  auto dialect = workspace.install_dialect<Library::Dialect>("Library"_view);
  if (!dialect) {
    return Library::Llvm::Failure::ToolchainFailed;
  }

  auto interpreted =
      workspace.interpret_source(errors, "LlvmTest"_view, path, source);
  if (!interpreted || !interpreted->is<Library::Language::Monograph>()) {
    return Library::Llvm::Failure::SourceRejected;
  }

  Library::Llvm::Request request(
      static_cast<const Library::Language::Monograph&>(*interpreted), errors,
      path, source, Library::Llvm::Target::X86_64SysV, debug);
  Library::Llvm::Compiler compiler;
  return compiler.compile(products, request);
}

static auto contains_error(const Errors& errors, View::Bytes expected) -> Bool {
  Allocator::Arena rendered;
  for (Count index = 0; index < errors.get_size(); index++) {
    if (Algorithm::search(errors.render_message(rendered, index), expected) !=
        Count(-1)) {
      return True;
    }
  }

  return False;
}

PERIMORTEM_UNIT_TEST(LlvmTests, foreign_integration) {
  EXPECT_EQ(run_foreign_integration(), 0);
}

PERIMORTEM_UNIT_TEST(LlvmTests, runtime_integration) {
  EXPECT_EQ(run_runtime_integration(), 0);
}

PERIMORTEM_UNIT_TEST(LlvmTests, standalone_runtime_integration) {
  Process::Request request = {
    .executable = ".bin/bin/validation/runtime_link_integration"_view,
  };
  Process::Observation observation = Process::run(request);

  EXPECT(observation.launched);
  EXPECT_NOT(observation.timed_out);
  EXPECT_EQ(observation.exit_status, 0);
  EXPECT(observation.standard_output.is_empty());
  EXPECT(observation.standard_error.is_empty());
  EXPECT(observation.runner_error.is_empty());
}

PERIMORTEM_UNIT_TEST(LlvmTests, deterministic_debug_products) {
  auto source = File::read("validation/data/ttx/llvm/runtime.ttx"_view);
  ASSERT(source);
  Allocator::Arena first_domain;
  Allocator::Arena second_domain;
  Allocator::Arena relocated_domain;
  Errors first_errors;
  Errors second_errors;
  Errors relocated_errors;
  Option<Library::Llvm::Products> first_products;
  Option<Library::Llvm::Products> second_products;
  Option<Library::Llvm::Products> relocated_products;
  compile_source(
      first_domain, first_errors, "validation/data/ttx/llvm/runtime.ttx"_view,
      *source, Library::Llvm::Debug::Level::Full)
      .visit(
          [&](const Library::Llvm::Products& products) {
            first_products = Option<Library::Llvm::Products>(products);
          },
          [&](Library::Llvm::Failure) {});
  compile_source(
      second_domain, second_errors, "validation/data/ttx/llvm/runtime.ttx"_view,
      *source, Library::Llvm::Debug::Level::Full)
      .visit(
          [&](const Library::Llvm::Products& products) {
            second_products = Option<Library::Llvm::Products>(products);
          },
          [&](Library::Llvm::Failure) {});
  compile_source(
      relocated_domain, relocated_errors,
      "validation/data/ttx/llvm/relocated_runtime.ttx"_view, *source,
      Library::Llvm::Debug::Level::Full)
      .visit(
          [&](const Library::Llvm::Products& products) {
            relocated_products = Option<Library::Llvm::Products>(products);
          },
          [&](Library::Llvm::Failure) {});

  ASSERT(first_products && second_products && relocated_products);
  EXPECT(first_errors.is_empty());
  EXPECT(second_errors.is_empty());
  EXPECT(relocated_errors.is_empty());
  EXPECT(first_products->get_llvm_ir() == second_products->get_llvm_ir());
  EXPECT(first_products->get_object() == second_products->get_object());
  EXPECT(first_products->get_header() == second_products->get_header());
  EXPECT(
      Algorithm::search(
          first_products->get_llvm_ir(),
          "validation/data/ttx/llvm/runtime.ttx"_view) != Count(-1));
  EXPECT(
      Algorithm::search(
          first_products->get_llvm_ir(),
          "!DIBasicType(name: \"Unsigned_64\", size: 64, encoding: "
          "DW_ATE_unsigned)"_view) != Count(-1));
  EXPECT(
      Algorithm::search(
          first_products->get_llvm_ir(),
          "!DICompositeType(tag: DW_TAG_structure_type, name: \"Pair\""_view) !=
      Count(-1));
  EXPECT(
      Algorithm::search(
          first_products->get_llvm_ir(),
          "!DICompositeType(tag: DW_TAG_enumeration_type, name: \"Mode\""_view) !=
      Count(-1));
  EXPECT(
      Algorithm::search(
          first_products->get_llvm_ir(),
          "!DIEnumerator(name: \"idle\", value: 0, isUnsigned: true)"_view) !=
      Count(-1));
  EXPECT(
      Algorithm::search(
          first_products->get_llvm_ir(),
          "!DILocalVariable(name: \"pair\""_view) != Count(-1));
  EXPECT(
      Algorithm::search(
          first_products->get_llvm_ir(),
          "!DICompositeType(tag: DW_TAG_structure_type, name: \"source\""_view) !=
      Count(-1));
  EXPECT(
      Algorithm::search(
          first_products->get_llvm_ir(),
          "!DIDerivedType(tag: DW_TAG_variable, name: \"object_static\""_view) !=
      Count(-1));
  EXPECT(
      Algorithm::search(
          first_products->get_llvm_ir(),
          "@TTX_ADDR_object_5fstatic = internal global"_view) != Count(-1));
  EXPECT(
      Algorithm::search(
          first_products->get_llvm_ir(), "DW_TAG_unspecified_type"_view) ==
      Count(-1));
  EXPECT(
      Algorithm::search(
          first_products->get_llvm_ir(), "checksumkind: CSK_SHA256"_view) !=
      Count(-1));
  EXPECT(
      Algorithm::search(
          relocated_products->get_llvm_ir(),
          "validation/data/ttx/llvm/relocated_runtime.ttx"_view) != Count(-1));
  EXPECT(
      Algorithm::search(
          first_products->get_llvm_ir(), "@TTX_FUNC_Pair__sum_self"_view) !=
      Count(-1));
  EXPECT(
      Algorithm::search(
          first_products->get_llvm_ir(), "@TTX_ADDR_dynamic"_view) !=
      Count(-1));
  EXPECT(
      Algorithm::search(
          first_products->get_llvm_ir(),
          "define internal %ttx.struct.Large "
          "@TTX_FUNC_LargeOps__create_static()"_view) != Count(-1));
  EXPECT(
      Algorithm::search(
          first_products->get_llvm_ir(),
          "define void @llvm_large(ptr noalias sret(%ttx.struct.Large)"_view) !=
      Count(-1));
  EXPECT(
      Algorithm::search(
          first_products->get_header(), "TTX_FUNC_small_static"_view) !=
      Count(-1));
  EXPECT(
      Algorithm::search(
          first_products->get_header(),
          "typedef struct ttx_Option_5bUnsigned_5f64_5d {\n"
          "  uint64_t value;\n"
          "  bool set;\n"_view) != Count(-1));
  EXPECT(
      Algorithm::search(
          first_products->get_header(),
          "typedef struct ttx_View_5bUnsigned_5f64_5d {\n"
          "  const uint64_t *data;\n"_view) != Count(-1));
  EXPECT(
      Algorithm::search(first_products->get_llvm_ir(), "__ttx_fn_"_view) ==
      Count(-1));
  EXPECT(first_products->get_object() != relocated_products->get_object());
}

PERIMORTEM_UNIT_TEST(LlvmTests, scoped_static_debug_metadata) {
  static constexpr View::Bytes source =
      "// Scoped Static debug acceptance.\n"
      "dialect : Library;\n"
      "public Holder : struct {\n"
      "  public cached : Unsigned_64 = 7;\n"
      "}\n"_view;
  Allocator::Arena domain;
  Errors errors;
  Option<Library::Llvm::Products> products;
  compile_source(
      domain, errors, "scoped_static.ttx"_view, source,
      Library::Llvm::Debug::Level::Full)
      .visit(
          [&](const Library::Llvm::Products& selected) {
            products = Option<Library::Llvm::Products>(selected);
          },
          [&](Library::Llvm::Failure) {});

  ASSERT(products);
  EXPECT(errors.is_empty());
  EXPECT(
      Algorithm::search(
          products->get_llvm_ir(),
          "@TTX_ADDR_Holder__cached = internal global"_view) != Count(-1));
  EXPECT(
      Algorithm::search(
          products->get_llvm_ir(), "%ttx.struct.Holder = type"_view) ==
      Count(-1));
  EXPECT(
      Algorithm::search(
          products->get_llvm_ir(),
          "!DICompositeType(tag: DW_TAG_structure_type, name: \"Holder\""_view) !=
      Count(-1));
  EXPECT(
      Algorithm::search(
          products->get_llvm_ir(),
          "!DIDerivedType(tag: DW_TAG_variable, name: \"cached\""_view) !=
      Count(-1));
  EXPECT(
      Algorithm::search(products->get_llvm_ir(), "DIFlagStaticMember"_view) !=
      Count(-1));
}

PERIMORTEM_UNIT_TEST(LlvmTests, backend_rejections_report_source) {
  struct Rejection {
    View::Bytes source;
    View::Bytes message;
  };
  static constexpr Static::Vector<Rejection, 4> rejections = {{
    Rejection{
      "// Missing ABI.\ndialect : Library;\n@symbol(\"entry\")\npublic "
      "entry : func = [] -> [] { return; }\n"_view,
      "symbol override requires `@abi(\"C\")`"_view,
    },
    {
      "// Invalid symbol.\ndialect : Library;\n@abi(\"C\")\n"
      "@symbol(\"bad-symbol\")\npublic entry : func = [] -> [] { return; }\n"_view,
      "not a valid C identifier"_view,
    },
    {
      "// Duplicate symbol.\ndialect : Library;\n@abi(\"C\")\n"
      "@symbol(\"shared\")\npublic first : func = [] -> [] { return; }\n"
      "@abi(\"C\")\n@symbol(\"shared\")\npublic second : func = [] -> [] "
      "{ return; }\n"_view,
      "collides with another emitted declaration"_view,
    },
    {
      "// Recursive inline carrier.\ndialect : Library;\n"
      "public Recursive : struct { public state next : Option[Recursive]; }\n"
      "@abi(\"C\")\npublic entry : func = [] -> [] { return; }\n"_view,
      "recursively contains itself"_view,
    },
  }};

  for (const Rejection& rejection : rejections.get_view()) {
    Allocator::Arena products;
    Errors errors;
    Bool rejected = False;
    compile_source(
        products, errors, "backend_rejection.ttx"_view, rejection.source,
        Library::Llvm::Debug::Level::None)
        .visit(
            [&](const Library::Llvm::Products&) {},
            [&](Library::Llvm::Failure failure) {
              rejected = failure == Library::Llvm::Failure::SourceRejected;
            });
    EXPECT(rejected);
    EXPECT(contains_error(errors, rejection.message));
  }
}
