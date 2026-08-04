// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/function.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/static/vector.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/library/language/parser/layout.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/lexical/errors.hpp"
#include "ttx/lexical/tokenizer.hpp"
#include "ttx/model/alias.hpp"
#include "ttx/model/documentations/comment.hpp"
#include "ttx/model/type.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Library;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Ttx::Model;
using namespace Validation;

class SignatureType : public Type {
 public:
  constexpr SignatureType(View::Bytes name) : name(name) {}

  constexpr auto get_name() const -> View::Bytes override { return name; }
  auto get_documentation() const -> const Documentation& override {
    return Documentation::get_empty();
  }
  auto resolve_context(View::Bytes) const -> const Abstract& override {
    return Invalid::get_invalid();
  }

 private:
  View::Bytes name;
};

class SignatureTypes : public Abstract {
 public:
  constexpr auto get_name() const -> View::Bytes override {
    return "SignatureTypes"_view;
  }
  auto get_documentation() const -> const Documentation& override {
    return Documentation::get_empty();
  }
  auto resolve_context(View::Bytes route) const -> const Abstract& override {
    if (route == "Bool"_view || route == "Core::Bool"_view) {
      return boolean;
    }
    if (route == "Unsigned_64"_view) {
      return unsigned_64;
    }
    return Invalid::get_invalid();
  }

  SignatureType boolean{"Bool"_view};
  SignatureType unsigned_64{"Unsigned_64"_view};
};

static constexpr Documentations::Comment function_documentation{
  "Retains authored Function documentation."_view,
};

static auto matches_token(const Cursor& cursor, Token expected) -> Bool {
  Token current = cursor.current();
  return current.get_offset() == expected.get_offset() &&
         current.get_code() == expected.get_code();
}

static auto rejects_completion(View::Bytes source, const SignatureTypes& types)
    -> Bool {
  Allocator::Arena arena;
  Errors errors;
  Tokenizer tokenizer(arena, source, "rejected-function.ttx"_view);
  Cursor cursor(tokenizer, errors);
  auto reserved =
      Language::Function::reserve(arena, cursor, function_documentation);
  if (!reserved) {
    return False;
  }

  Token signature_start = cursor.current();
  Bool completed = reserved->complete(cursor, types);
  return !completed && !reserved->is_complete() &&
         &reserved->resolve() == &Invalid::get_invalid() &&
         matches_token(cursor, signature_start) && !errors.is_empty();
}

static Harness FunctionTests = {
  .name = "Tetrodotoxin::Library::Language::Function"_view,
};

PERIMORTEM_UNIT_TEST(FunctionTests, stable_completion) {
  static constexpr View::Bytes source =
      "public func ready[.value : Bool] -> Unsigned_64 { { } } "
      "private func next[] -> Bool {}"_view;
  Allocator::Arena arena;
  Errors errors;
  Tokenizer tokenizer(arena, source, "stable-function.ttx"_view);
  Cursor cursor(tokenizer, errors);
  SignatureTypes types;

  // Reservation exposes the stable identity before the signature can resolve.
  // Completion must enrich that same object and leave the caller exactly at
  // the next declaration after the nested body.
  auto reserved =
      Language::Function::reserve(arena, cursor, function_documentation);
  ASSERT(reserved);
  Language::Function& function = *reserved;
  const Language::Function* identity = &function;

  EXPECT(&function.resolve() == &Invalid::get_invalid());
  EXPECT_TEXT(function.get_name(), "ready"_view);
  EXPECT(&function.get_documentation() == &function_documentation);
  EXPECT(function.get_visibility() == Language::Visibility::Public);
  EXPECT(cursor.matches(Code::Type::LayoutStart));

  ASSERT(function.complete(cursor, types));
  EXPECT(&function == identity);
  EXPECT(&function.resolve() == identity);
  EXPECT(function.is<Language::Function>());
  EXPECT(function.is<Language::Callables::Static>());
  EXPECT(cursor.matches(Code::Type::Private));
  EXPECT(errors.is_empty());

  ASSERT_EQ(function.get_parameters().get_size(), Count(1));
  EXPECT(function.get_parameters().get_abstract(0).visit(
      []() { return False; },
      [&types](const Abstract& edge) {
        return edge.is<Alias>() && edge.get_name() == "value"_view &&
                       &edge.resolve() == &types.boolean
                   ? True
                   : False;
      }));
  ASSERT_EQ(function.get_results().get_size(), Count(1));
  EXPECT(function.get_results().get_abstract(0).visit(
      []() { return False; },
      [&types](const Abstract& edge) {
        return &edge == &types.unsigned_64 ? True : False;
      }));
}

PERIMORTEM_UNIT_TEST(FunctionTests, real_layout_shapes) {
  static constexpr View::Bytes source =
      "[] Bool [Bool, Unsigned_64] "
      "[.value : Bool, .ready : Bool] Core::Bool"_view;
  Allocator::Arena arena;
  Errors errors;
  Tokenizer tokenizer(arena, source, "layout-shapes.ttx"_view);
  Cursor cursor(tokenizer, errors);
  SignatureTypes types;

  // One Cursor parses every accepted shape in sequence so each successful
  // Layout must consume exactly its own grammar and preserve the next one.
  auto empty = Language::Parser::Layout::parse(arena, cursor, types);
  auto direct = Language::Parser::Layout::parse(arena, cursor, types);
  auto unnamed = Language::Parser::Layout::parse(arena, cursor, types);
  auto named = Language::Parser::Layout::parse(arena, cursor, types);
  auto qualified = Language::Parser::Layout::parse(arena, cursor, types);
  ASSERT(empty && direct && unnamed && named && qualified);

  EXPECT(empty->is_empty());
  EXPECT(direct->get_abstract(0).visit(
      []() { return False; },
      [&types](const Abstract& edge) {
        return &edge == &types.boolean ? True : False;
      }));
  ASSERT_EQ(unnamed->get_size(), Count(2));
  EXPECT(unnamed->get_abstract(0).visit(
      []() { return False; },
      [&types](const Abstract& edge) {
        return &edge == &types.boolean ? True : False;
      }));
  EXPECT(unnamed->get_abstract(1).visit(
      []() { return False; },
      [&types](const Abstract& edge) {
        return &edge == &types.unsigned_64 ? True : False;
      }));
  ASSERT_EQ(named->get_size(), Count(2));
  EXPECT(named->get_abstract(0).visit(
      []() { return False; },
      [&types](const Abstract& edge) {
        return edge.is<Alias>() && edge.get_name() == "value"_view &&
                       &edge.resolve() == &types.boolean
                   ? True
                   : False;
      }));
  EXPECT(named->get_abstract(1).visit(
      []() { return False; },
      [&types](const Abstract& edge) {
        return edge.is<Alias>() && edge.get_name() == "ready"_view &&
                       &edge.resolve() == &types.boolean
                   ? True
                   : False;
      }));
  EXPECT(qualified->get_abstract(0).visit(
      []() { return False; },
      [&types](const Abstract& edge) {
        return &edge == &types.boolean ? True : False;
      }));
  EXPECT(cursor.matches(Code::Type::Terminal));
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(FunctionTests, rejected_completion_is_atomic) {
  SignatureTypes types;
  static constexpr Static::Vector<View::Bytes, 6> rejected = {{
    "private func bad[] -> Missing {}"_view,
    "private func bad[.value : Bool, .value : Bool] -> [] {}"_view,
    "private func bad[Bool, .value : Bool] -> [] {}"_view,
    "private func bad[] -> Bool;"_view,
    "private func bad[] -> Bool { { }"_view,
    "private func bad[Bool Bool] -> [] {}"_view,
  }};

  // Every rejected transaction allocates independently and must leave its
  // Function incomplete while later cases continue against the same Types.
  for (Count i = 0; i < rejected.get_size(); i++) {
    EXPECT(rejects_completion(rejected[i], types));
  }
}

PERIMORTEM_UNIT_TEST(FunctionTests, completion_occurs_once) {
  static constexpr View::Bytes source = "private func once[] -> Bool {}"_view;
  Allocator::Arena arena;
  Errors errors;
  Tokenizer tokenizer(arena, source, "one-completion.ttx"_view);
  Cursor cursor(tokenizer, errors);
  SignatureTypes types;
  auto reserved =
      Language::Function::reserve(arena, cursor, function_documentation);
  ASSERT(reserved);
  ASSERT(reserved->complete(cursor, types));
  const Layout& parameters = reserved->get_parameters();
  const Layout& results = reserved->get_results();

  Errors repeated_errors;
  Tokenizer repeated_tokenizer(arena, source, "repeated-completion.ttx"_view);
  Cursor repeated(repeated_tokenizer, repeated_errors);
  repeated.consume();
  repeated.consume();
  repeated.consume();
  EXPECT_NOT(reserved->complete(repeated, types));
  EXPECT(&reserved->get_parameters() == &parameters);
  EXPECT(&reserved->get_results() == &results);
  EXPECT(&reserved->resolve() == &*reserved);
  EXPECT_NOT(repeated_errors.is_empty());
}
