// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/operations/range.hpp"

#include "validation/unit_test.hpp"
#include "validation/unit_tests/tetrodotoxin/library/language/fixture.hpp"

#include "tetrodotoxin/language/monograph.hpp"
#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/language/types/range.hpp"
#include "tetrodotoxin/library/language/types/real_32.hpp"
#include "tetrodotoxin/library/language/types/signed_8.hpp"
#include "tetrodotoxin/library/language/types/unsigned_16.hpp"
#include "tetrodotoxin/library/language/types/unsigned_8.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/lexical/errors.hpp"
#include "ttx/lexical/tokenizer.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Utility;
using namespace Tetrodotoxin::Library::Language;
using namespace Ttx::Concept;
using namespace Validation;

static Harness LibraryRange = {
  .name = "Tetrodotoxin::Library::Language::Operations::Range"_view,
};

class RangeExpression : public Expression {
 public:
  RangeExpression(View::Bytes name, const Abstract& type)
      : Expression({}), name(name), type(type) {}

  auto get_name() const -> View::Bytes override { return name; }

  auto get_documentation() const -> const Documentation& override {
    return Documentation::get_empty();
  }

  auto get_type() const -> const Abstract& override { return type; }

 private:
  View::Bytes name;
  const Abstract& type;
};

static auto fold_is_dynamic(Operations::Range& range) -> Bool {
  return range.fold().visit(
      [](const Option<Model::Pack&>& selected) { return Bool(!selected); },
      [](const Expression::Error&) { return False; });
}

PERIMORTEM_UNIT_TEST(LibraryRange, exact_materialization) {
  Allocator::Arena domain;
  Tetrodotoxin::Library::Dialect producer;
  auto& source = create_library_monograph(domain, producer);
  Ttx::Lexical::Errors errors;
  Ttx::Lexical::Tokenizer tokenizer(domain, {}, "range-link.ttx"_view);
  Ttx::Lexical::Cursor cursor(tokenizer, errors);
  Types::Unsigned_8 unsigned_8;
  Types::Signed_8 signed_8;
  RangeExpression unsigned_start("unsigned start"_view, unsigned_8);
  RangeExpression unsigned_end("unsigned end"_view, unsigned_8);
  RangeExpression signed_start("signed start"_view, signed_8);
  RangeExpression signed_end("signed end"_view, signed_8);
  auto& first =
      Operations::Range::create_synthetic(domain, unsigned_start, unsigned_end);
  auto& repeated =
      Operations::Range::create_synthetic(domain, unsigned_start, unsigned_end);
  auto& signed_range =
      Operations::Range::create_synthetic(domain, signed_start, signed_end);

  ASSERT(first.is<Operations::Range>());
  EXPECT(
      first.implements(Ttx::Concept::get_type_identity<Operations::Range>()));
  ASSERT(first.link(cursor, source));
  ASSERT(first.link(cursor, source));
  ASSERT(repeated.link(cursor, source));
  ASSERT(signed_range.link(cursor, source));
  ASSERT(first.get_type().is<Types::Range>());
  ASSERT(signed_range.get_type().is<Types::Range>());
  const auto& first_type = static_cast<const Types::Range&>(first.get_type());
  const auto& signed_type =
      static_cast<const Types::Range&>(signed_range.get_type());
  EXPECT(&first.get_type() == &repeated.get_type());
  EXPECT(&first_type.get_element_type() == &unsigned_8);
  EXPECT(&signed_type.get_element_type() == &signed_8);
  EXPECT(fold_is_dynamic(first));
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(LibraryRange, integer_legality) {
  Allocator::Arena domain;
  Tetrodotoxin::Library::Dialect producer;
  auto& source = create_library_monograph(domain, producer);
  Ttx::Lexical::Errors errors;
  Ttx::Lexical::Tokenizer tokenizer(domain, {}, "range-link.ttx"_view);
  Ttx::Lexical::Cursor cursor(tokenizer, errors);
  Types::Unsigned_8 unsigned_8;
  Types::Unsigned_16 unsigned_16;
  Types::Real_32 real_32;
  RangeExpression unsigned_value("unsigned"_view, unsigned_8);
  RangeExpression other_width("other width"_view, unsigned_16);
  RangeExpression flag("flag"_view, resolve_library_flag(source));
  RangeExpression real("real"_view, real_32);
  RangeExpression unresolved("unresolved"_view, Invalid::get_invalid());
  auto& mismatch =
      Operations::Range::create_synthetic(domain, unsigned_value, other_width);
  auto& bool_range = Operations::Range::create_synthetic(domain, flag, flag);
  auto& real_range = Operations::Range::create_synthetic(domain, real, real);
  auto& unresolved_range =
      Operations::Range::create_synthetic(domain, unsigned_value, unresolved);

  EXPECT_NOT(mismatch.link(cursor, source));
  EXPECT_NOT(bool_range.link(cursor, source));
  EXPECT_NOT(real_range.link(cursor, source));
  EXPECT_NOT(unresolved_range.link(cursor, source));
  EXPECT_EQ(errors.get_size(), Count(4));
}
