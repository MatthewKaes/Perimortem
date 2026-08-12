// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/access/index.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/language/monograph.hpp"
#include "tetrodotoxin/library/language/materializations.hpp"
#include "tetrodotoxin/library/language/parser/expression.hpp"
#include "tetrodotoxin/library/language/types/access.hpp"
#include "tetrodotoxin/library/language/types/bool.hpp"
#include "tetrodotoxin/library/language/types/unsigned_8.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/lexical/errors.hpp"
#include "ttx/lexical/tokenizer.hpp"
#include "ttx/model/addressable.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Ttx::Model;
using namespace Validation;

static Harness LibraryIndex = {
  .name = "Tetrodotoxin::Library::Language::Access::Index"_view,
};

class IndexMonograph : public Language::Monograph {
 public:
  IndexMonograph(Allocator::Arena& domain)
      : Language::Monograph(domain, Documentation::get_empty()) {}

  auto get_name() const -> View::Bytes override { return "Index source"_view; }
  auto resolve_context(View::Bytes) const -> const Abstract& override {
    return Invalid::get_invalid();
  }
};

class IndexBinding : public Addressable {
 public:
  IndexBinding(View::Bytes name, const Ttx::Model::Type& type)
      : name(name), type(type) {}

  auto get_name() const -> View::Bytes override { return name; }
  auto get_documentation() const -> const Documentation& override {
    return Documentation::get_empty();
  }
  auto get_type() const -> const Ttx::Model::Type& override { return type; }

 private:
  View::Bytes name;
  const Ttx::Model::Type& type;
};

class IndexContext : public Abstract {
 public:
  explicit IndexContext(const Addressable& binding) : binding(binding) {}

  auto get_name() const -> View::Bytes override { return "Index context"_view; }
  auto get_documentation() const -> const Documentation& override {
    return Documentation::get_empty();
  }
  auto resolve_context(View::Bytes name) const -> const Abstract& override {
    if (name == binding.get_name()) {
      return binding;
    }

    return Invalid::get_invalid();
  }

 private:
  const Addressable& binding;
};

static auto parse_index(
    Allocator::Arena& domain,
    Library::Language::Materializations& materializations,
    const Abstract& context,
    View::Bytes source,
    Errors& errors) -> Option<Library::Language::Access::Index&> {
  Tokenizer tokenizer(domain, source, "index.ttx"_view);
  Cursor cursor(tokenizer, errors);
  auto parsed = Library::Language::Parser::Expression::parse(
      domain, materializations, cursor, context);
  if (!parsed || !cursor.matches(Code::Type::Terminal)) {
    return {};
  }

  return parsed->select<Library::Language::Access::Index>();
}

static auto rejects_grammar(
    Allocator::Arena& domain,
    Library::Language::Materializations& materializations,
    const Abstract& context,
    View::Bytes source) -> Bool {
  Errors errors;
  Tokenizer tokenizer(domain, source, "invalid-index.ttx"_view);
  Cursor cursor(tokenizer, errors);
  Token opening = cursor.current();
  auto parsed = Library::Language::Parser::Expression::parse(
      domain, materializations, cursor, context);
  Token ending = cursor.current();
  return !parsed && !errors.is_empty() &&
         opening.get_code() == ending.get_code() &&
         opening.get_offset() == ending.get_offset();
}

PERIMORTEM_UNIT_TEST(LibraryIndex, exact_edges_and_delayed_link) {
  Allocator::Arena domain;
  Library::Language::Materializations materializations(domain);
  Library::Language::Types::Unsigned_8 element;
  Library::Language::Types::Access access("Access[Unsigned_8]"_view, element);
  IndexBinding binding("storage"_view, access);
  IndexContext context(binding);
  IndexMonograph monograph(domain);
  Errors unsigned_errors;
  auto unsigned_index = parse_index(
      domain, materializations, context, "storage[1]"_view, unsigned_errors);
  ASSERT(unsigned_index);

  EXPECT(&unsigned_index->get_element_type() == &Invalid::get_invalid());
  ASSERT_EQ(unsigned_index->get_inputs().get_size(), Count(2));
  EXPECT(
      &*unsigned_index->get_inputs().get_abstract(0) ==
      &unsigned_index->get_receiver());
  EXPECT(
      &*unsigned_index->get_inputs().get_abstract(1) ==
      &unsigned_index->get_index());
  ASSERT(unsigned_index->link(monograph, context, materializations));
  ASSERT(unsigned_index->link(monograph, context, materializations));
  EXPECT(&unsigned_index->get_element_type() == &element);
  EXPECT(&unsigned_index->get_type() == &Invalid::get_invalid());
  EXPECT_NOT(unsigned_index->fits(element));
  EXPECT(unsigned_errors.is_empty());
  EXPECT(monograph.get_diagnostics().is_empty());

  Errors signed_errors;
  auto signed_index = parse_index(
      domain, materializations, context, "storage[-1]"_view, signed_errors);
  ASSERT(signed_index);
  EXPECT(signed_index->link(monograph, context, materializations));
  EXPECT(&signed_index->get_element_type() == &element);
  EXPECT(signed_errors.is_empty());
}

PERIMORTEM_UNIT_TEST(LibraryIndex, invalid_domains_and_value_use_are_rejected) {
  Allocator::Arena domain;
  Library::Language::Materializations materializations(domain);
  Library::Language::Types::Unsigned_8 element;
  Library::Language::Types::Access access("Access[Unsigned_8]"_view, element);
  Library::Language::Types::Boolean value_type;
  IndexBinding storage("storage"_view, access);
  IndexBinding value("value"_view, value_type);
  IndexContext storage_context(storage);
  IndexContext value_context(value);

  IndexMonograph receiver_error(domain);
  Errors receiver_parse_errors;
  auto invalid_receiver = parse_index(
      domain, materializations, value_context, "value[1]"_view,
      receiver_parse_errors);
  ASSERT(invalid_receiver);
  EXPECT_NOT(
      invalid_receiver->link(receiver_error, value_context, materializations));
  EXPECT_NOT(receiver_error.get_diagnostics().is_empty());

  IndexMonograph index_error(domain);
  Errors index_parse_errors;
  auto invalid_index = parse_index(
      domain, materializations, storage_context, "storage[true]"_view,
      index_parse_errors);
  ASSERT(invalid_index);
  EXPECT_NOT(
      invalid_index->link(index_error, storage_context, materializations));
  EXPECT_NOT(index_error.get_diagnostics().is_empty());

  IndexMonograph value_error(domain);
  Errors value_parse_errors;
  Tokenizer tokenizer(domain, "storage[1] + 1"_view, "index-as-value.ttx"_view);
  Cursor cursor(tokenizer, value_parse_errors);
  auto value_expression = Library::Language::Parser::Expression::parse(
      domain, materializations, cursor, storage_context);
  ASSERT(value_expression && cursor.matches(Code::Type::Terminal));
  EXPECT_NOT(
      value_expression->link(value_error, storage_context, materializations));
  EXPECT_NOT(value_error.get_diagnostics().is_empty());
}

PERIMORTEM_UNIT_TEST(LibraryIndex, malformed_postfix_is_atomic) {
  Allocator::Arena domain;
  Library::Language::Materializations materializations(domain);
  Library::Language::Types::Unsigned_8 element;
  Library::Language::Types::Access access("Access[Unsigned_8]"_view, element);
  IndexBinding storage("storage"_view, access);
  IndexContext context(storage);

  EXPECT(rejects_grammar(domain, materializations, context, "storage[]"_view));
  EXPECT(rejects_grammar(domain, materializations, context, "storage[1"_view));
  EXPECT(
      rejects_grammar(domain, materializations, context, "storage[1, 2]"_view));
}
