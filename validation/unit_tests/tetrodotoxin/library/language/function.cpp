// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/function.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/static/vector.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/language/monograph.hpp"
#include "tetrodotoxin/library/language/constants/unsigned.hpp"
#include "tetrodotoxin/library/language/identifier.hpp"
#include "tetrodotoxin/library/language/materializations.hpp"
#include "tetrodotoxin/library/language/parameter.hpp"
#include "tetrodotoxin/library/language/signature.hpp"
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

static_assert(
    !__is_constructible(Language::Function, const Language::Function&));
static_assert(!__is_constructible(Language::Function, Language::Function&&));
static_assert(
    !__is_constructible(Language::Parameter, const Language::Parameter&));
static_assert(!__is_constructible(Language::Parameter, Language::Parameter&&));

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

class SignatureContext : public Abstract {
 public:
  constexpr SignatureContext(View::Bytes name, const Type& boolean)
      : name(name), boolean(boolean) {}

  constexpr auto get_name() const -> View::Bytes override { return name; }
  auto get_documentation() const -> const Documentation& override {
    return Documentation::get_empty();
  }
  auto resolve_context(View::Bytes route) const -> const Abstract& override {
    if (route == "Bool"_view) {
      return boolean;
    }

    return Invalid::get_invalid();
  }

 private:
  View::Bytes name;
  const Type& boolean;
};

class SignatureTypes : public Type {
 public:
  SignatureTypes() : core("Core"_view, boolean) {}

  constexpr auto get_name() const -> View::Bytes override {
    return "SignatureTypes"_view;
  }
  auto get_documentation() const -> const Documentation& override {
    return Documentation::get_empty();
  }
  auto resolve_context(View::Bytes route) const -> const Abstract& override {
    if (route == "Bool"_view) {
      return boolean;
    }
    if (route == "Core"_view) {
      return core;
    }
    if (route == "Unsigned_64"_view) {
      return unsigned_64;
    }
    return Invalid::get_invalid();
  }

  SignatureType boolean{"Bool"_view};
  SignatureType unsigned_64{"Unsigned_64"_view};
  SignatureContext core;
};

class LateSignatureTypes : public Type {
 public:
  constexpr auto get_name() const -> View::Bytes override {
    return "LateSignatureTypes"_view;
  }
  auto get_documentation() const -> const Documentation& override {
    return Documentation::get_empty();
  }
  auto resolve_context(View::Bytes route) const -> const Abstract& override {
    if (published && route == "Late"_view) {
      return late;
    }

    return Invalid::get_invalid();
  }

  constexpr auto publish() -> void { published = True; }

  SignatureType late{"Late"_view};

 private:
  Bool published = False;
};

class FunctionParent : public Tetrodotoxin::Language::Monograph {
 public:
  FunctionParent(Allocator::Arena& domain, const Abstract& types)
      : Tetrodotoxin::Language::Monograph(domain, Documentation::get_empty()),
        types(types) {}

  constexpr auto get_name() const -> View::Bytes override {
    return "FunctionParent"_view;
  }

  auto resolve_context(View::Bytes route) const -> const Abstract& override {
    return types.resolve_context(route);
  }

 private:
  const Abstract& types;
};

static constexpr Documentations::Comment function_documentation{
  "Retains authored Function documentation."_view,
};

static auto matches_token(const Cursor& cursor, Token expected) -> Bool {
  Token current = cursor.current();
  return current.get_offset() == expected.get_offset() &&
         current.get_code() == expected.get_code();
}

static auto get_parameter(const Layout& layout, Count index)
    -> const Language::Parameter* {
  return layout.get_abstract(index).visit(
      []() -> const Language::Parameter* { return nullptr; },
      [](const Abstract& edge) -> const Language::Parameter* {
        return edge.visit<Language::Parameter>(
            [](const Language::Parameter& parameter) { return &parameter; },
            [](const Abstract&) -> const Language::Parameter* {
              return nullptr;
            });
      });
}

static auto get_identifier(const Language::Expression& expression)
    -> const Language::Identifier* {
  return expression.visit<Language::Identifier>(
      [](const Language::Identifier& identifier) { return &identifier; },
      [](const Abstract&) -> const Language::Identifier* { return nullptr; });
}

static auto fold_is_unsigned(
    const Perimortem::Utility::Result<
        Perimortem::Utility::Option<Language::Expression&>,
        Language::Expression::Error>& result,
    Unsigned_64 expected) -> Bool {
  return result.visit(
      [&](const Perimortem::Utility::Option<Language::Expression&>& selected) {
        if (!selected) {
          return False;
        }

        return selected->visit<Language::Constants::Unsigned>(
            [&](const Language::Constants::Unsigned& value) {
              return value.get_value() == expected ? True : False;
            },
            [](const Abstract&) { return False; });
      },
      [](const Language::Expression::Error&) { return False; });
}

static auto fold_reports(
    const Perimortem::Utility::Result<
        Perimortem::Utility::Option<Language::Expression&>,
        Language::Expression::Error>& result,
    Language::Expression::Error::Type expected,
    const Language::Expression& expression) -> Bool {
  return result.visit(
      [](const Perimortem::Utility::Option<Language::Expression&>&) {
        return False;
      },
      [&](const Language::Expression::Error& error) {
        return error.get_type() == expected &&
                       &error.get_expression() == &expression
                   ? True
                   : False;
      });
}

static auto rejects_completion(View::Bytes source, const SignatureTypes& types)
    -> Bool {
  Allocator::Arena arena;
  Language::Materializations materializations(arena);
  FunctionParent parent(arena, types);
  Errors errors;
  Tokenizer tokenizer(arena, source, "rejected-function.ttx"_view);
  Cursor cursor(tokenizer, errors);
  auto reserved = Language::Function::reserve(
      arena, cursor, function_documentation, parent, types, materializations);
  if (!reserved) {
    return False;
  }

  Token signature_start = cursor.current();
  Bool completed = reserved->complete(cursor);
  return !completed && !reserved->is_complete() &&
         &reserved->resolve() == &Invalid::get_invalid() &&
         matches_token(cursor, signature_start) && !errors.is_empty();
}

static Harness FunctionTests = {
  .name = "Tetrodotoxin::Library::Language::Function"_view,
};

PERIMORTEM_UNIT_TEST(FunctionTests, stable_authored_graph) {
  static constexpr View::Bytes source =
      "public func ready[.value : Bool] -> Unsigned_64 { value; return value; "
      "} "
      "private func next[] -> Bool {}"_view;
  Allocator::Arena arena;
  Language::Materializations materializations(arena);
  Errors errors;
  Tokenizer tokenizer(arena, source, "stable-function.ttx"_view);
  Cursor cursor(tokenizer, errors);
  SignatureTypes types;
  FunctionParent parent(arena, types);

  // Reservation publishes only identity and the defining lexical anchors.
  // Completion enriches that same object with signature and body facts while
  // the caller remains positioned at the next declaration.
  auto reserved = Language::Function::reserve(
      arena, cursor, function_documentation, parent, types, materializations);
  ASSERT(reserved);
  Language::Function& function = *reserved;
  const Language::Function* identity = &function;

  EXPECT(&function.resolve() == &Invalid::get_invalid());
  EXPECT_TEXT(function.get_name(), "ready"_view);
  EXPECT(&function.get_documentation() == &function_documentation);
  EXPECT(function.get_visibility() == Language::Visibility::Public);
  EXPECT(&function.get_source() == &parent);
  EXPECT(&function.get_host() == &types);
  EXPECT_TEXT(function.get_token().caculate_text(source), "func"_view);
  EXPECT_TEXT(function.get_name_token().caculate_text(source), "ready"_view);
  EXPECT_NOT(function.get_span());
  EXPECT(cursor.matches(Code::Type::BracketStart));

  ASSERT(function.complete(cursor));
  EXPECT(&function == identity);
  EXPECT(&function.resolve() == &Invalid::get_invalid());
  EXPECT(function.get_parameters().is_empty());
  EXPECT(function.get_results().is_empty());
  auto authored_signature = function.get_signature();
  ASSERT(authored_signature);
  const Language::Signature* signature_identity = &*authored_signature;
  ASSERT(authored_signature->get_anchor());
  EXPECT_TEXT(
      authored_signature->get_anchor()->get_span().caculate_text(source),
      "[.value : Bool] -> Unsigned_64"_view);
  ASSERT_EQ(authored_signature->get_parameter_size(), Count(1));
  ASSERT_EQ(authored_signature->get_result_size(), Count(1));
  ASSERT(authored_signature->get_parameter_anchor(0));
  ASSERT(authored_signature->get_result_anchor(0));
  EXPECT_TEXT(
      authored_signature->get_parameter_anchor(0)->get_span().caculate_text(
          source),
      ".value : Bool"_view);
  EXPECT_TEXT(
      authored_signature->get_result_anchor(0)->get_span().caculate_text(
          source),
      "Unsigned_64"_view);

  ASSERT(function.link_signature());
  EXPECT(&function.resolve() == identity);
  EXPECT_NOT(function.is_linked());
  ASSERT(function.link_body());
  ASSERT(function.link());
  EXPECT(&function.resolve() == identity);
  EXPECT(&*function.get_signature() == signature_identity);
  EXPECT(function.is<Language::Function>());
  EXPECT(function.is<Callable>());
  EXPECT_TEXT(
      function.get_span().caculate_text(source),
      "public func ready[.value : Bool] -> Unsigned_64 { value; return value; }"_view);
  EXPECT(cursor.matches(Code::Type::Private));
  EXPECT(errors.is_empty());

  ASSERT_EQ(function.get_parameters().get_size(), Count(1));
  const Language::Parameter* parameter =
      get_parameter(function.get_parameters(), 0);
  ASSERT(parameter);
  EXPECT_TEXT(parameter->get_name(), "value"_view);
  EXPECT(&parameter->get_type() == &types.boolean);
  EXPECT_TEXT(parameter->get_name_token().caculate_text(source), "value"_view);
  EXPECT_TEXT(
      parameter->get_span().caculate_text(source), ".value : Bool"_view);
  EXPECT_TEXT(parameter->get_type_span().caculate_text(source), "Bool"_view);

  ASSERT_EQ(function.get_results().get_size(), Count(1));
  EXPECT(function.get_results().get_abstract(0).visit(
      []() { return False; },
      [&types](const Abstract& edge) {
        return &edge == &types.unsigned_64 ? True : False;
      }));

  auto expressions = function.get_expressions();
  ASSERT_EQ(expressions.get_size(), Count(2));
  const Language::Identifier* first =
      get_identifier(expressions.get_data()[0].get());
  const Language::Identifier* returned =
      get_identifier(expressions.get_data()[1].get());
  ASSERT(first && returned);
  const auto& first_anchor = first->get_anchor();
  const auto& returned_anchor = returned->get_anchor();
  ASSERT(first_anchor);
  ASSERT(returned_anchor);
  EXPECT(first != returned);
  EXPECT_TEXT(first->get_name(), "value"_view);
  EXPECT_TEXT(first_anchor->get_span().caculate_text(source), "value"_view);
  EXPECT(
      first_anchor->get_span().get_offset() !=
      returned_anchor->get_span().get_offset());
  auto first_addressable = first->get_addressable();
  auto returned_addressable = returned->get_addressable();
  ASSERT(first_addressable && returned_addressable);
  EXPECT(&*first_addressable == parameter);
  EXPECT(&*returned_addressable == parameter);

  EXPECT_TEXT(function.get_return_token().caculate_text(source), "return"_view);
  EXPECT_TEXT(
      function.get_return_span().caculate_text(source), "return value;"_view);
  auto return_expression = function.get_return_expression();
  ASSERT(return_expression);
  EXPECT(&*return_expression == &expressions.get_data()[1].get());
}

PERIMORTEM_UNIT_TEST(FunctionTests, direct_parameter_and_bare_return) {
  static constexpr View::Bytes source =
      "private func stop Bool -> [] { return; }"_view;
  Allocator::Arena arena;
  Language::Materializations materializations(arena);
  Errors errors;
  Tokenizer tokenizer(arena, source, "bare-return.ttx"_view);
  Cursor cursor(tokenizer, errors);
  SignatureTypes types;
  FunctionParent parent(arena, types);
  auto function = Language::Function::reserve(
      arena, cursor, function_documentation, parent, types, materializations);
  ASSERT(function);
  ASSERT(function->complete(cursor));
  ASSERT(function->link());

  ASSERT_EQ(function->get_parameters().get_size(), Count(1));
  EXPECT(function->get_parameters().get_abstract(0).visit(
      []() { return False; },
      [&types](const Abstract& edge) {
        return &edge == &types.boolean ? True : False;
      }));
  auto signature = function->get_signature();
  ASSERT(signature);
  ASSERT_EQ(signature->get_parameter_size(), Count(1));
  ASSERT(signature->get_parameter_anchor(0));
  EXPECT_TEXT(
      signature->get_parameter_anchor(0)->get_span().caculate_text(source),
      "Bool"_view);
  EXPECT(signature->get_result_size() == 0);
  EXPECT(function->get_expressions().is_empty());
  EXPECT_TEXT(
      function->get_return_token().caculate_text(source), "return"_view);
  EXPECT_TEXT(
      function->get_return_span().caculate_text(source), "return;"_view);
  EXPECT_NOT(function->get_return_expression());
  EXPECT(cursor.matches(Code::Type::Terminal));
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(FunctionTests, self_is_exact_host_parameter) {
  static constexpr View::Bytes source =
      "public func inspect[self, .value : Bool] -> Bool { return value; }"_view;
  Allocator::Arena arena;
  Language::Materializations materializations(arena);
  Errors errors;
  Tokenizer tokenizer(arena, source, "self-function.ttx"_view);
  Cursor cursor(tokenizer, errors);
  SignatureTypes types;
  FunctionParent parent(arena, types);
  auto function = Language::Function::reserve(
      arena, cursor, function_documentation, parent, types, materializations);
  ASSERT(function);
  ASSERT(function->complete(cursor));

  auto signature = function->get_signature();
  ASSERT(signature);
  EXPECT_TEXT(signature->get_parameter_name(0), "self"_view);
  EXPECT_NOT(signature->get_parameter_type_access(0));
  ASSERT(signature->get_parameter_type_anchor(0));
  EXPECT_TEXT(
      signature->get_parameter_type_anchor(0)->get_span().caculate_text(source),
      "self"_view);

  ASSERT(function->link());
  const Callable& callable = *function;
  ASSERT_EQ(callable.get_parameters().get_size(), Count(2));
  const Language::Parameter* self = get_parameter(callable.get_parameters(), 0);
  const Language::Parameter* value =
      get_parameter(callable.get_parameters(), 1);
  ASSERT(self && value);
  EXPECT_TEXT(self->get_name(), "self"_view);
  EXPECT(&self->get_type() == &types);
  EXPECT_TEXT(value->get_name(), "value"_view);
  EXPECT(&value->get_type() == &types.boolean);
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(FunctionTests, authored_signature_shapes) {
  static constexpr View::Bytes source =
      "[] -> Bool "
      "[Bool, Unsigned_64] -> [.value : Bool, .ready : Bool] "
      "Core::Bool -> []"_view;
  Allocator::Arena arena;
  Errors errors;
  Tokenizer tokenizer(arena, source, "signature-shapes.ttx"_view);
  Cursor cursor(tokenizer, errors);
  SignatureTypes types;
  FunctionParent parent(arena, types);

  // Signature is the complete source owner for parameters and results.
  // Parsing retains both sides without constructing a provisional TTX Layout.
  auto first = Language::Signature::interpret(arena, cursor);
  ASSERT(first);
  EXPECT(first->get_parameter_size() == 0);
  EXPECT(first->get_result_size() == 1);
  EXPECT_NOT(first->get_result_type(0));
  ASSERT(first->link(parent, types));
  ASSERT(first->get_result_type(0));
  EXPECT(&*first->get_result_type(0) == &types.boolean);

  auto second = Language::Signature::interpret(arena, cursor);
  ASSERT(second);
  EXPECT(second->get_parameter_size() == 2);
  EXPECT(second->get_result_size() == 2);
  EXPECT_TEXT(second->get_result_name(0), "value"_view);
  EXPECT_TEXT(second->get_result_name(1), "ready"_view);
  ASSERT(second->link(parent, types));
  EXPECT(second->get_parameters().get_size() == 2);
  EXPECT(second->get_results().get_size() == 2);

  auto third = Language::Signature::interpret(arena, cursor);
  ASSERT(third);
  EXPECT(third->get_parameter_size() == 1);
  EXPECT(third->get_result_size() == 0);
  ASSERT(third->get_parameter_type_anchor(0));
  EXPECT_TEXT(
      third->get_parameter_type_anchor(0)->get_span().caculate_text(source),
      "Core::Bool"_view);
  ASSERT(third->link(parent, types));
  ASSERT(third->get_parameter_type(0));
  EXPECT(&*third->get_parameter_type(0) == &types.boolean);
  EXPECT(cursor.matches(Code::Type::Terminal));
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(FunctionTests, parameter_requires_linked_signature) {
  static constexpr View::Bytes source = "[.value : Bool] -> []"_view;
  Allocator::Arena arena;
  Errors errors;
  Tokenizer tokenizer(arena, source, "parameter-signature.ttx"_view);
  Cursor cursor(tokenizer, errors);
  SignatureTypes types;
  FunctionParent parent(arena, types);
  auto signature = Language::Signature::interpret(arena, cursor);
  ASSERT(signature);
  EXPECT_NOT(
      Language::Parameter::create_authored(
          arena, *signature, 0, types.boolean));
  ASSERT(signature->link(parent, types));
  const Language::Parameter* parameter =
      get_parameter(signature->get_parameters(), 0);
  ASSERT(parameter);
  EXPECT_TEXT(parameter->get_name(), "value"_view);
  EXPECT(&parameter->get_type() == &types.boolean);
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(FunctionTests, rejected_completion_is_atomic) {
  SignatureTypes types;
  static constexpr Static::Vector<View::Bytes, 11> rejected = {{
    "private func bad[.value : Bool, .value : Bool] -> [] {}"_view,
    "private func bad[Bool, .value : Bool] -> [] {}"_view,
    "private func bad[.Bool : Bool] -> [] {}"_view,
    "private func bad[] -> Bool;"_view,
    "private func bad[] -> Bool { { } }"_view,
    "private func bad[Bool Bool] -> [] {}"_view,
    "private func bad[] -> Bool { return true; false; }"_view,
    "private func bad[] -> Bool { return; return; }"_view,
    "private func bad[.value : Bool, self] -> [] {}"_view,
    "private func bad[self, Bool] -> [] {}"_view,
    "private func bad[] -> [self] {}"_view,
  }};

  // Each rejected transaction allocates independently and leaves its Function
  // incomplete while later cases continue against the same Type identities.
  for (Count i = 0; i < rejected.get_size(); i++) {
    EXPECT(rejects_completion(rejected[i], types));
  }
}

PERIMORTEM_UNIT_TEST(
    FunctionTests,
    incomplete_link_retains_reservation_anchor) {
  static constexpr View::Bytes source = "private func reserved[] -> [] {}"_view;
  Allocator::Arena arena;
  Language::Materializations materializations(arena);
  Errors errors;
  Tokenizer tokenizer(arena, source, "incomplete-function.ttx"_view);
  Cursor cursor(tokenizer, errors);
  SignatureTypes types;
  FunctionParent parent(arena, types);
  auto function = Language::Function::reserve(
      arena, cursor, function_documentation, parent, types, materializations);
  ASSERT(function);

  EXPECT_NOT(function->link());
  auto diagnostics = parent.get_diagnostics();
  ASSERT_EQ(diagnostics.get_size(), Count(1));
  ASSERT(diagnostics.get_data()[0].get_anchor());
  const Anchor& anchor = *diagnostics.get_data()[0].get_anchor();
  EXPECT_TEXT(anchor.get_token().caculate_text(source), "func"_view);
  EXPECT_TEXT(
      anchor.get_span().caculate_text(source), "private func reserved"_view);
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(FunctionTests, unresolved_type_waits_for_link) {
  static constexpr View::Bytes source =
      "private func late[.value : Missing] -> [] { value; }"_view;
  Allocator::Arena arena;
  Language::Materializations materializations(arena);
  Errors errors;
  Tokenizer tokenizer(arena, source, "late-type.ttx"_view);
  Cursor cursor(tokenizer, errors);
  SignatureTypes types;
  FunctionParent parent(arena, types);
  auto function = Language::Function::reserve(
      arena, cursor, function_documentation, parent, types, materializations);
  ASSERT(function);
  ASSERT(function->complete(cursor));
  EXPECT(errors.is_empty());
  EXPECT_NOT(function->link());
  EXPECT(&function->resolve() == &Invalid::get_invalid());
  auto expressions = function->get_expressions();
  ASSERT_EQ(expressions.get_size(), Count(1));
  const auto* identifier = get_identifier(expressions.get_data()[0].get());
  ASSERT(identifier);
  EXPECT_NOT(identifier->get_addressable());
  auto diagnostics = parent.get_diagnostics();
  ASSERT_EQ(diagnostics.get_size(), Count(1));
  ASSERT(diagnostics.get_data()[0].get_anchor());
  EXPECT_TEXT(
      diagnostics.get_data()[0].get_anchor()->get_span().caculate_text(source),
      "Missing"_view);
}

PERIMORTEM_UNIT_TEST(FunctionTests, late_type_enriches_authored_identities) {
  static constexpr View::Bytes source =
      "private func late[.value : Late] -> [] { value; }"_view;
  Allocator::Arena arena;
  Language::Materializations materializations(arena);
  Errors errors;
  Tokenizer tokenizer(arena, source, "late-publication.ttx"_view);
  Cursor cursor(tokenizer, errors);
  LateSignatureTypes types;
  FunctionParent parent(arena, types);
  auto function = Language::Function::reserve(
      arena, cursor, function_documentation, parent, types, materializations);
  ASSERT(function);
  ASSERT(function->complete(cursor));
  const Language::Function* function_identity = &*function;
  auto authored_signature = function->get_signature();
  ASSERT(authored_signature);
  const Language::Signature* signature_identity = &*authored_signature;
  auto expressions = function->get_expressions();
  ASSERT_EQ(expressions.get_size(), Count(1));
  const Language::Identifier* identifier =
      get_identifier(expressions.get_data()[0].get());
  ASSERT(identifier);
  const Language::Identifier* identifier_identity = identifier;

  EXPECT_NOT(authored_signature->get_parameter_type(0));
  EXPECT_NOT(identifier->get_addressable());
  EXPECT(&function->resolve() == &Invalid::get_invalid());

  types.publish();
  ASSERT(function->link());
  EXPECT(&*function == function_identity);
  EXPECT(&*function->get_signature() == signature_identity);
  auto linked_type = authored_signature->get_parameter_type(0);
  ASSERT(linked_type);
  EXPECT(&*linked_type == &types.late);
  const Language::Parameter* parameter =
      get_parameter(function->get_parameters(), 0);
  ASSERT(parameter);
  EXPECT(&parameter->get_type() == &types.late);
  EXPECT(
      get_identifier(function->get_expressions().get_data()[0].get()) ==
      identifier_identity);
  auto addressable = identifier->get_addressable();
  ASSERT(addressable);
  EXPECT(&*addressable == parameter);
  EXPECT(errors.is_empty());
  EXPECT(parent.get_diagnostics().is_empty());
}

PERIMORTEM_UNIT_TEST(FunctionTests, completion_occurs_once) {
  static constexpr View::Bytes source = "private func once[] -> Bool {}"_view;
  Allocator::Arena arena;
  Language::Materializations materializations(arena);
  Errors errors;
  Tokenizer tokenizer(arena, source, "one-completion.ttx"_view);
  Cursor cursor(tokenizer, errors);
  SignatureTypes types;
  FunctionParent parent(arena, types);
  auto reserved = Language::Function::reserve(
      arena, cursor, function_documentation, parent, types, materializations);
  ASSERT(reserved);
  ASSERT(reserved->complete(cursor));
  ASSERT(reserved->link());
  const Layout& parameters = reserved->get_parameters();
  const Layout& results = reserved->get_results();
  Span span = reserved->get_span();
  EXPECT_NOT(reserved->get_return_token());
  EXPECT_NOT(reserved->get_return_span());
  EXPECT_NOT(reserved->get_return_expression());

  Errors repeated_errors;
  Tokenizer repeated_tokenizer(arena, source, "repeated-completion.ttx"_view);
  Cursor repeated(repeated_tokenizer, repeated_errors);
  repeated.consume();
  repeated.consume();
  repeated.consume();
  EXPECT_NOT(reserved->complete(repeated));
  EXPECT(&reserved->get_parameters() == &parameters);
  EXPECT(&reserved->get_results() == &results);
  EXPECT(reserved->get_span().get_offset() == span.get_offset());
  EXPECT(reserved->get_span().get_size() == span.get_size());
  EXPECT(&reserved->resolve() == &*reserved);
  EXPECT_NOT(repeated_errors.is_empty());
}

PERIMORTEM_UNIT_TEST(
    FunctionTests,
    finalize_caches_authored_roots_without_diagnostics) {
  static constexpr View::Bytes source =
      "private func folded[] -> Unsigned_64 { 1 / 0; 6 / 2; }"_view;
  Allocator::Arena arena;
  Language::Materializations materializations(arena);
  Errors errors;
  Tokenizer tokenizer(arena, source, "function-folding.ttx"_view);
  Cursor cursor(tokenizer, errors);
  SignatureTypes types;
  FunctionParent parent(arena, types);
  auto function = Language::Function::reserve(
      arena, cursor, function_documentation, parent, types, materializations);

  ASSERT(function);
  ASSERT(function->complete(cursor));
  ASSERT(function->link());
  ASSERT_EQ(function->get_expressions().get_size(), Count(2));
  EXPECT(parent.get_diagnostics().is_empty());

  ASSERT(function->finalize());
  auto expressions = function->get_expressions();
  Language::Expression& first = expressions.get_data()[0].get();
  Language::Expression& second = expressions.get_data()[1].get();
  const auto& first_anchor = first.get_anchor();
  const auto& second_anchor = second.get_anchor();
  ASSERT(first_anchor);
  ASSERT(second_anchor);
  EXPECT_TEXT(first_anchor->get_span().caculate_text(source), "1 / 0"_view);
  EXPECT_TEXT(second_anchor->get_span().caculate_text(source), "6 / 2"_view);
  EXPECT(fold_reports(
      first.fold(), Language::Expression::Error::Type::DivisionByZero, first));
  EXPECT(fold_is_unsigned(second.fold(), 3));
  EXPECT(parent.get_diagnostics().is_empty());
  EXPECT(errors.is_empty());
}
