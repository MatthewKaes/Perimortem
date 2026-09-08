// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/environment/toolchain.hpp"

#include "validation/unit_test.hpp"

#include <cstdlib>
#include <unistd.h>

#include "perimortem/core/null_terminated.hpp"

#include "perimortem/system/file.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Validation;

class TestDialect : public Language::Dialect {
 public:
  TestDialect(View::Bytes name) : Language::Dialect(name) {}

  auto interpret(Cursor&, const Documentation&, const Anchor&, Abstract&)
      -> Option<Language::Monograph&> override {
    return {};
  }
};

class IndependentDialect : public TestDialect {
 public:
  using TestDialect::TestDialect;
};

class LowerDialect : public TestDialect {
 public:
  using TestDialect::TestDialect;
};

class MiddleDialect : public TestDialect {
 public:
  MiddleDialect(View::Bytes name, LowerDialect& lower)
      : TestDialect(name), lower(lower) {}

  auto get_lower() const -> const LowerDialect& { return lower; }

 private:
  LowerDialect& lower;
};

class LeftDialect : public TestDialect {
 public:
  LeftDialect(View::Bytes name, LowerDialect& lower)
      : TestDialect(name), lower(lower) {}

  auto get_lower() const -> const LowerDialect& { return lower; }

 private:
  LowerDialect& lower;
};

class RightDialect : public TestDialect {
 public:
  RightDialect(View::Bytes name, LowerDialect& lower)
      : TestDialect(name), lower(lower) {}

  auto get_lower() const -> const LowerDialect& { return lower; }

 private:
  LowerDialect& lower;
};

class TopDialect : public TestDialect {
 public:
  TopDialect(View::Bytes name, LeftDialect& left, RightDialect& right)
      : TestDialect(name), left(left), right(right) {}

  auto get_left() const -> const LeftDialect& { return left; }
  auto get_right() const -> const RightDialect& { return right; }

 private:
  LeftDialect& left;
  RightDialect& right;
};

class RootDialect : public TestDialect {
 public:
  RootDialect(View::Bytes name) : TestDialect(name) {}

  RootDialect(View::Bytes name, Language::Dialect&) : TestDialect(name) {}
};

class ChildDialect : public TestDialect {
 public:
  ChildDialect(View::Bytes name, RootDialect& root)
      : TestDialect(name), root(root) {}

  auto get_root() const -> const RootDialect& { return root; }

 private:
  RootDialect& root;
};

class SourceFile {
 public:
  explicit SourceFile(View::Bytes text) {
    const int descriptor = mkstemp(path);
    if (descriptor < 0) {
      return;
    }
    close(descriptor);
    ready = Perimortem::System::File::write(text, get_path());
  }
  ~SourceFile() { unlink(path); }
  auto get_path() const -> View::Bytes { return NullTerminated::to_view(path); }
  Bool ready = False;

 private:
  char path[64] = "/tmp/ttx-toolchain-XXXXXX";
};

class ProcessedSource : public Language::Monograph {
 public:
  ProcessedSource(
      Allocator::Arena& arena,
      Language::Dialect& dialect,
      const Documentation& documentation,
      Abstract& context,
      View::Bytes body)
      : Monograph(arena, dialect, documentation, context), body(body) {}
  TTX_NAME("Processed"_view);
  auto link(Cursor&) -> Bool override {
    linked = True;
    return True;
  }
  View::Bytes body;
  Bool linked = False;
};

class ProcessingDialect : public Language::Dialect {
 public:
  explicit ProcessingDialect(View::Bytes name) : Dialect(name) {}
  auto interpret(
      Cursor& cursor,
      const Documentation& documentation,
      const Anchor&,
      Abstract& context) -> Option<Language::Monograph&> override {
    source = cursor.get_arena().construct<ProcessedSource>(
        cursor.get_arena(), *this, documentation, context,
        cursor.get_source_text().slice(cursor.current().get_offset()));
    return *source;
  }
  Option<ProcessedSource&> source;
};

static Harness EnvironmentToolchain = {
  .name = "Tetrodotoxin::Environment::Toolchain"_view,
};

PERIMORTEM_UNIT_TEST(EnvironmentToolchain, installs_dialects) {
  Environment::Toolchain toolchain;

  auto independent = toolchain.install<IndependentDialect>("Independent"_view);
  auto lower = toolchain.install<LowerDialect>("Lower"_view);
  ASSERT(independent);
  ASSERT(lower);
  auto middle = toolchain.install<MiddleDialect>("Middle"_view, *lower);

  ASSERT(middle);
  EXPECT(&middle->get_lower() == &*lower);

  auto left = toolchain.install<LeftDialect>("Left"_view, *lower);
  auto right = toolchain.install<RightDialect>("Right"_view, *lower);
  ASSERT(left);
  ASSERT(right);
  auto top = toolchain.install<TopDialect>("Top"_view, *left, *right);

  ASSERT(top);
  EXPECT(&top->get_left() == &*left);
  EXPECT(&top->get_right() == &*right);
  EXPECT(&top->get_left().get_lower() == &*lower);
  EXPECT(&top->get_right().get_lower() == &*lower);

  auto root = toolchain.install<RootDialect>("Root"_view);
  ASSERT(root);
  auto child = toolchain.install<ChildDialect>("Child"_view, *root);
  ASSERT(child);

  EXPECT_NOT(toolchain.install<IndependentDialect>("Root"_view));
  EXPECT(toolchain.install<RootDialect>("SecondRoot"_view));
  EXPECT(toolchain.install<RootDialect>("ContextRoot"_view, *child));
  EXPECT(&child->get_root() == &*root);
}

PERIMORTEM_UNIT_TEST(EnvironmentToolchain, rejects_dependencies) {
  Environment::Toolchain local;
  Environment::Toolchain foreign;
  RootDialect missing("Missing"_view);

  auto missing_result = local.install<ChildDialect>("Missing"_view, missing);
  auto foreign_root = foreign.install<RootDialect>("ForeignRoot"_view);
  ASSERT(foreign_root);
  auto foreign_result =
      local.install<ChildDialect>("Foreign"_view, *foreign_root);

  EXPECT_NOT(missing_result);
  EXPECT_NOT(foreign_result);
}

PERIMORTEM_UNIT_TEST(EnvironmentToolchain, processes_source_body) {
  Environment::Toolchain toolchain;
  auto dialect = toolchain.install<ProcessingDialect>("Probe"_view);
  ASSERT(dialect);
  {
    SourceFile file(
        "// Source documentation\ndialect : Probe;\nprivate Input : alias = source(\"unused.ttx\");\n"_view);
    ASSERT(file.ready);
    auto processed = toolchain.process(file.get_path());
    ASSERT(processed);
    ASSERT(dialect->source);
    EXPECT(&*processed == &*dialect->source);
  }
  // Removing the physical source does not invalidate the interpreted root.
  // Its body also proves that the engine stopped parsing after the header.
  EXPECT_TEXT(
      dialect->source->body,
      "private Input : alias = source(\"unused.ttx\");\n"_view);
  EXPECT_NOT(dialect->source->linked);
}

PERIMORTEM_UNIT_TEST(EnvironmentToolchain, rejects_invalid_source) {
  Environment::Toolchain toolchain;
  ASSERT(toolchain.install<TestDialect>("Reject"_view));
  SourceFile unknown("// Source\ndialect : Unknown;\n"_view);
  SourceFile malformed("// Source\ndialect Reject;\n"_view);
  SourceFile rejected("// Source\ndialect : Reject;\n"_view);
  ASSERT(unknown.ready && malformed.ready && rejected.ready);
  EXPECT_NOT(toolchain.process(unknown.get_path()));
  EXPECT_NOT(toolchain.process(malformed.get_path()));
  EXPECT_NOT(toolchain.process(rejected.get_path()));
  EXPECT_NOT(toolchain.process("/dev/null/missing.ttx"_view));
}
