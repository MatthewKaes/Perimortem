// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "validation/process/child.hpp"
#include "validation/unit_test.hpp"

#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/algorithm/search.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "perimortem/system/file.hpp"

#include "tetrodotoxin/environment/workspace.hpp"
#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "tetrodotoxin/terminal/abi/c/header.hpp"
#include "tetrodotoxin/terminal/abi/compiler.hpp"
#include "tetrodotoxin/terminal/llvm/compiler.hpp"
#include "ttx/lexical/errors.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::System;
using namespace Tetrodotoxin;
using namespace Tetrodotoxin::Terminal;
using namespace Ttx::Lexical;
using namespace Validation;

static Harness LlvmTests = {
  .name = "Tetrodotoxin::Terminal::Llvm"_view,
};

extern "C" auto run_foreign_integration() -> int;
extern "C" auto run_runtime_integration() -> int;

static auto compile_source(
    Allocator::Arena& products,
    Errors& errors,
    View::Bytes path,
    View::Bytes source,
    Llvm::Module::Debug::Level debug,
    View::Bytes* header = nullptr)
    -> Perimortem::Utility::Result<Llvm::Products, Llvm::Failure> {
  Environment::Toolchain toolchain;
  auto dialect = toolchain.install<Library::Dialect>("Library"_view);
  if (!dialect) {
    return Llvm::Failure::ToolchainFailed;
  }
  Environment::Workspace workspace(toolchain);

  auto interpreted =
      workspace.interpret_source(errors, "LlvmTest"_view, path, source);
  if (!interpreted || !interpreted->is<Library::Language::Monograph>()) {
    return Llvm::Failure::SourceRejected;
  }

  const auto& monograph =
      static_cast<const Library::Language::Monograph&>(*interpreted);
  Terminal::Abi::Unit unit("LlvmTest"_view);
  Terminal::Abi::Compiler interface_compiler;
  auto native_interface = interface_compiler.compile(
      products, monograph, unit, errors, path, source);
  if (!native_interface) {
    return Llvm::Failure::SourceRejected;
  }

  Llvm::Request request(
      monograph, errors, path, source, Llvm::Target::X86_64SysV, debug, unit,
      *native_interface);
  Llvm::Compiler compiler;
  auto result = compiler.compile(products, request);
  if (header) {
    result.visit(
        [&](const Llvm::Products& compiled) {
          auto identified = Terminal::Abi::C::Header::identify(
              products, native_interface->get_c_header(), unit.get_package(),
              compiled.get_abi_fingerprint());
          if (identified) {
            *header = identified->get_view();
          }
        },
        [](Llvm::Failure) {});
  }
  return result;
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

PERIMORTEM_UNIT_TEST(LlvmTests, standalone_runtime) {
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

PERIMORTEM_UNIT_TEST(LlvmTests, system_abi) {
  Process::Request request = {
    .executable = ".bin/bin/validation/system_abi_integration"_view,
    .standard_input = "Hello System\n"_view,
  };
  Process::Observation observation = Process::run(request);

  EXPECT(observation.launched);
  EXPECT_NOT(observation.timed_out);
  EXPECT_EQ(observation.exit_status, 0);
  EXPECT_TEXT(observation.standard_input, "Hello System\n"_view);
  EXPECT_TEXT(observation.standard_output, "Hello System\n"_view);
  EXPECT(observation.standard_error.is_empty());
  EXPECT(observation.runner_error.is_empty());
}

PERIMORTEM_UNIT_TEST(LlvmTests, native_package) {
  Process::Request request = {
    .executable = ".bin/bin/validation/package_native_integration"_view,
  };
  Process::Observation observation = Process::run(request);

  EXPECT(observation.launched);
  EXPECT_NOT(observation.timed_out);
  EXPECT_EQ(observation.exit_status, 0);
  EXPECT(observation.standard_output.is_empty());
  EXPECT(observation.standard_error.is_empty());
  EXPECT(observation.runner_error.is_empty());
}

PERIMORTEM_UNIT_TEST(LlvmTests, echo_package) {
  Process::Request request = {
    .executable = ".bin/bin/apps/ttx/echo/echo"_view,
    .standard_input = "hello\n\nquit\n"_view,
  };
  Process::Observation observation = Process::run(request);

  EXPECT(observation.launched);
  EXPECT_NOT(observation.timed_out);
  EXPECT_EQ(observation.exit_status, 0);
  EXPECT_TEXT(observation.standard_input, "hello\n\nquit\n"_view);
  EXPECT_TEXT(observation.standard_output, "Echo: hello\nEcho: \n"_view);
  EXPECT(observation.standard_error.is_empty());
  EXPECT(observation.runner_error.is_empty());

  request.standard_input = "exit\n"_view;
  observation = Process::run(request);
  EXPECT(observation.launched);
  EXPECT_NOT(observation.timed_out);
  EXPECT_EQ(observation.exit_status, 0);
  EXPECT(observation.standard_output.is_empty());
  EXPECT(observation.standard_error.is_empty());
  EXPECT(observation.runner_error.is_empty());

  request.standard_input = {};
  observation = Process::run(request);
  EXPECT(observation.launched);
  EXPECT_NOT(observation.timed_out);
  EXPECT_EQ(observation.exit_status, 0);
  EXPECT(observation.standard_output.is_empty());
  EXPECT(observation.standard_error.is_empty());
  EXPECT(observation.runner_error.is_empty());
}

PERIMORTEM_UNIT_TEST(LlvmTests, debug_products) {
  auto source = File::read("validation/data/ttx/llvm/runtime.ttx"_view);
  ASSERT(source);
  Allocator::Arena first_domain;
  Allocator::Arena second_domain;
  Allocator::Arena relocated_domain;
  Errors first_errors;
  Errors second_errors;
  Errors relocated_errors;
  Option<Llvm::Products> first_products;
  Option<Llvm::Products> second_products;
  Option<Llvm::Products> relocated_products;
  View::Bytes first_header;
  View::Bytes second_header;
  compile_source(
      first_domain, first_errors, "validation/data/ttx/llvm/runtime.ttx"_view,
      *source, Llvm::Module::Debug::Level::Full, &first_header)
      .visit(
          [&](const Llvm::Products& products) {
            first_products = Option<Llvm::Products>(products);
          },
          [&](Llvm::Failure) {});
  compile_source(
      second_domain, second_errors, "validation/data/ttx/llvm/runtime.ttx"_view,
      *source, Llvm::Module::Debug::Level::Full, &second_header)
      .visit(
          [&](const Llvm::Products& products) {
            second_products = Option<Llvm::Products>(products);
          },
          [&](Llvm::Failure) {});
  compile_source(
      relocated_domain, relocated_errors,
      "validation/data/ttx/llvm/relocated_runtime.ttx"_view, *source,
      Llvm::Module::Debug::Level::Full)
      .visit(
          [&](const Llvm::Products& products) {
            relocated_products = Option<Llvm::Products>(products);
          },
          [&](Llvm::Failure) {});

  ASSERT(first_products && second_products && relocated_products);
  EXPECT(first_errors.is_empty());
  EXPECT(second_errors.is_empty());
  EXPECT(relocated_errors.is_empty());
  EXPECT(first_products->get_llvm_ir() == second_products->get_llvm_ir());
  EXPECT(first_products->get_object() == second_products->get_object());
  EXPECT(first_header == second_header);
  EXPECT(
      Algorithm::search(
          first_products->get_llvm_ir(),
          "validation/data/ttx/llvm/runtime.ttx"_view) != Count(-1));
  EXPECT(
      Algorithm::search(
          first_products->get_llvm_ir(),
          "!DIBasicType(name: \"U64\", size: 64, encoding: "
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
          "!DILocalVariable(name: \"pair_value\""_view) != Count(-1));
  EXPECT(
      Algorithm::search(
          first_products->get_llvm_ir(),
          "!DILocalVariable(name: \"frozen_dense\""_view) != Count(-1));
  EXPECT(
      Algorithm::search(
          first_products->get_llvm_ir(),
          "store [4 x i64] [i64 5, i64 6, i64 7, i64 8], ptr %const.debug"_view) !=
      Count(-1));
  EXPECT(
      Algorithm::search(
          first_products->get_llvm_ir(),
          "#dbg_declare(ptr %const.debug"_view) != Count(-1));
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
          "@__ttx_object_descriptor_Counter = internal constant { i64, i64, "
          "ptr } { i64 8, i64 8, ptr @__ttx_object_finalize_Counter }"_view) !=
      Count(-1));
  EXPECT(
      Algorithm::search(
          first_products->get_llvm_ir(),
          "@perimortem_core_object_allocate(ptr "
          "@__ttx_object_descriptor_Counter)"_view) != Count(-1));
  EXPECT(
      Algorithm::search(
          first_products->get_llvm_ir(),
          "@perimortem_core_object_retain(ptr null)"_view) == Count(-1));
  EXPECT(
      Algorithm::search(
          first_products->get_llvm_ir(),
          "@perimortem_core_object_release(ptr null)"_view) == Count(-1));
  EXPECT(
      Algorithm::search(
          first_products->get_llvm_ir(),
          "define void @llvm_large(ptr noalias sret(%ttx.struct.Large)"_view) !=
      Count(-1));
  EXPECT(
      Algorithm::search(first_header, "TTX_FUNC_small_static"_view) !=
      Count(-1));
  EXPECT(
      Algorithm::search(
          first_header,
          "typedef struct ttx_llvmtest_Option_5bU64_5d {\n"
          "  uint64_t value;\n"
          "  bool set;\n"_view) != Count(-1));
  EXPECT(
      Algorithm::search(
          first_header,
          "typedef struct ttx_llvmtest_View_5bU64_5d {\n"
          "  const uint64_t *data;\n"_view) != Count(-1));
  EXPECT(
      Algorithm::search(first_header, "uint64_t llvm_bytes_api(void);"_view) !=
      Count(-1));
  EXPECT(
      Algorithm::search(
          first_header, "#define TTX_ABI_FINGERPRINT_llvmtest \""_view) !=
      Count(-1));
  EXPECT(
      Algorithm::search(
          first_products->get_llvm_ir(),
          "%ttx.struct.Dynamic__Bytes = type { ptr, i64 }"_view) != Count(-1));
  EXPECT(
      Algorithm::search(first_products->get_llvm_ir(), "__ttx_fn_"_view) ==
      Count(-1));
  EXPECT(first_products->get_object() != relocated_products->get_object());
}

PERIMORTEM_UNIT_TEST(LlvmTests, static_debug_scope) {
  static constexpr View::Bytes source =
      "// Scoped Static debug acceptance.\n"
      "dialect : Library;\n"
      "public Holder : struct {\n"
      "  public cached : U64 = 7;\n"
      "}\n"_view;
  Allocator::Arena domain;
  Errors errors;
  Option<Llvm::Products> products;
  compile_source(
      domain, errors, "scoped_static.ttx"_view, source,
      Llvm::Module::Debug::Level::Full)
      .visit(
          [&](const Llvm::Products& selected) {
            products = Option<Llvm::Products>(selected);
          },
          [&](Llvm::Failure) {});

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

PERIMORTEM_UNIT_TEST(LlvmTests, source_diagnostics) {
  struct Rejection {
    View::Bytes source;
    View::Bytes message;
  };
  static constexpr Static::Vector<Rejection, 5> rejections = {{
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
    {
      "// Object buffer owning element.\ndialect : Library;\n"
      "public Node : object { public state value : U64; }\n"
      "@abi(\"C\")\npublic entry : func = [.value : Object[Node]] -> [] "
      "{ return; }\n"_view,
      "requires one value-only element Type"_view,
    },
  }};

  for (const Rejection& rejection : rejections.get_view()) {
    Allocator::Arena products;
    Errors errors;
    Bool rejected = False;
    compile_source(
        products, errors, "backend_rejection.ttx"_view, rejection.source,
        Llvm::Module::Debug::Level::None)
        .visit(
            [&](const Llvm::Products&) {},
            [&](Llvm::Failure failure) {
              rejected = failure == Llvm::Failure::SourceRejected;
            });
    EXPECT(rejected);
    EXPECT(contains_error(errors, rejection.message));
  }
}
