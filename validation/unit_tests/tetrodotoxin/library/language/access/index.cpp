// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/access/index.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
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

static auto create_monograph(
    Allocator::Arena& domain,
    Library::Dialect& dialect,
    Abstract& context) -> Option<Library::Language::Monograph&> {
  Errors errors;
  Tokenizer tokenizer(domain, ""_view, "index-source.ttx"_view);
  Cursor cursor(tokenizer, errors);
  auto interpreted = dialect.interpret(
      domain, cursor, Documentation::get_empty(), Anchor::create(Span()),
      context);
  if (!interpreted || !errors.is_empty() ||
      !interpreted->is<Library::Language::Monograph>()) {
    return {};
  }

  return static_cast<Library::Language::Monograph&>(*interpreted);
}

static auto parse_index(
    Allocator::Arena& domain,
    Library::Language::Monograph& monograph,
    View::Bytes source,
    Errors& errors) -> Option<Library::Language::Access::Index&> {
  Tokenizer tokenizer(domain, source, "index.ttx"_view);
  Cursor cursor(tokenizer, errors);
  auto parsed =
      Library::Language::Parser::Expression::parse(domain, monograph, cursor);
  auto index = parsed.visit(
      []() -> Option<Library::Language::Access::Index&> { return {}; },
      [](Library::Language::Model::Pack& selected) {
        return selected.select<Library::Language::Access::Index>();
      });
  if (!index || !cursor.matches(Code::Type::Terminal)) {
    return {};
  }

  return index;
}

static auto rejects_index_suffix(
    Allocator::Arena& domain,
    Library::Language::Monograph& monograph,
    View::Bytes suffix) -> Bool {
  Errors receiver_errors;
  Tokenizer receiver_tokenizer(
      domain, "storage"_view, "index-receiver.ttx"_view);
  Cursor receiver_cursor(receiver_tokenizer, receiver_errors);
  auto receiver_pack = Library::Language::Parser::Expression::parse(
      domain, monograph, receiver_cursor);
  auto receiver = receiver_pack.visit(
      []() -> Option<Library::Language::Expression&> { return {}; },
      [](Library::Language::Model::Pack& selected) {
        return selected.select<Library::Language::Expression>();
      });
  BAIL_IF(
      !receiver || !receiver_cursor.matches(Code::Type::Terminal) ||
      !receiver_errors.is_empty());

  Errors errors;
  Tokenizer tokenizer(domain, suffix, "invalid-index.ttx"_view);
  Cursor cursor(tokenizer, errors);
  Token opening = cursor.current();
  auto parsed = Library::Language::Access::Index::parse(
      domain, monograph, cursor, *receiver);
  Token ending = cursor.current();
  return !parsed && opening.get_code() == ending.get_code() &&
         opening.get_offset() == ending.get_offset();
}

PERIMORTEM_UNIT_TEST(LibraryIndex, exact_edges_and_delayed_link) {
  Allocator::Arena domain;
  Library::Language::Types::Unsigned_8 element;
  Library::Language::Types::Access access("Access[Unsigned_8]"_view, element);
  IndexBinding binding("storage"_view, access);
  IndexContext context(binding);
  Library::Dialect dialect;
  auto monograph = create_monograph(domain, dialect, context);
  ASSERT(monograph);
  Errors unsigned_errors;
  auto unsigned_index =
      parse_index(domain, *monograph, "storage[1]"_view, unsigned_errors);
  ASSERT(unsigned_index);

  EXPECT(&unsigned_index->get_element_type() == &Invalid::get_invalid());
  EXPECT(&unsigned_index->resolve() == &Invalid::get_invalid());
  EXPECT(unsigned_index->is<Library::Language::Expression>());
  EXPECT(unsigned_index->is<Library::Language::Model::Pack>());
  ASSERT(unsigned_index->link(*monograph, context));
  ASSERT(unsigned_index->link(*monograph, context));
  EXPECT(&unsigned_index->get_element_type() == &element);
  EXPECT(&unsigned_index->get_type() == &element);
  EXPECT(&unsigned_index->resolve() == &*unsigned_index);
  ASSERT_EQ(unsigned_index->get_layout().get_size(), Count(1));
  EXPECT(&*unsigned_index->get_layout().get_abstract(0) == &*unsigned_index);
  EXPECT(unsigned_index->fits(element));
  unsigned_index->finalize();
  EXPECT(unsigned_errors.is_empty());
  EXPECT(monograph->get_diagnostics().is_empty());

  Errors signed_errors;
  auto signed_index =
      parse_index(domain, *monograph, "storage[-1]"_view, signed_errors);
  ASSERT(signed_index);
  EXPECT(signed_index->link(*monograph, context));
  EXPECT(&signed_index->get_element_type() == &element);
  EXPECT(signed_errors.is_empty());
}

PERIMORTEM_UNIT_TEST(LibraryIndex, invalid_domains_are_rejected) {
  Allocator::Arena domain;
  Library::Language::Types::Unsigned_8 element;
  Library::Language::Types::Access access("Access[Unsigned_8]"_view, element);
  Library::Language::Types::Boolean value_type;
  IndexBinding storage("storage"_view, access);
  IndexBinding value("value"_view, value_type);
  IndexContext storage_context(storage);
  IndexContext value_context(value);
  Library::Dialect dialect;

  auto receiver_error = create_monograph(domain, dialect, value_context);
  ASSERT(receiver_error);
  Errors receiver_parse_errors;
  auto invalid_receiver = parse_index(
      domain, *receiver_error, "value[1]"_view, receiver_parse_errors);
  ASSERT(invalid_receiver);
  EXPECT_NOT(invalid_receiver->link(*receiver_error, value_context));
  EXPECT_NOT(receiver_error->get_diagnostics().is_empty());

  auto index_error = create_monograph(domain, dialect, storage_context);
  ASSERT(index_error);
  Errors index_parse_errors;
  auto invalid_index = parse_index(
      domain, *index_error, "storage[true]"_view, index_parse_errors);
  ASSERT(invalid_index);
  EXPECT_NOT(invalid_index->link(*index_error, storage_context));
  EXPECT_NOT(index_error->get_diagnostics().is_empty());
}

PERIMORTEM_UNIT_TEST(LibraryIndex, malformed_postfix_is_atomic) {
  Allocator::Arena domain;
  Library::Language::Types::Unsigned_8 element;
  Library::Language::Types::Access access("Access[Unsigned_8]"_view, element);
  IndexBinding storage("storage"_view, access);
  IndexContext context(storage);
  Library::Dialect dialect;
  auto monograph = create_monograph(domain, dialect, context);
  ASSERT(monograph);

  EXPECT(rejects_index_suffix(domain, *monograph, "[]"_view));
  EXPECT(rejects_index_suffix(domain, *monograph, "[1"_view));
  EXPECT(rejects_index_suffix(domain, *monograph, "[1, 2]"_view));
}
