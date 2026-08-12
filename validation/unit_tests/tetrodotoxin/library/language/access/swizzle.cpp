// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/access/swizzle.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/static/vector.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/environment/workspace.hpp"
#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/language/access/call.hpp"
#include "tetrodotoxin/library/language/field.hpp"
#include "tetrodotoxin/library/language/function.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "tetrodotoxin/library/language/operations/add.hpp"
#include "tetrodotoxin/library/language/parser/expression.hpp"
#include "tetrodotoxin/library/language/types/composite.hpp"
#include "tetrodotoxin/library/language/types/fixed.hpp"
#include "tetrodotoxin/library/language/types/source.hpp"
#include "tetrodotoxin/library/language/types/structure.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/lexical/errors.hpp"
#include "ttx/lexical/tokenizer.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Library;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using Tetrodotoxin::Environment::Workspace;
using namespace Validation;

static Harness SwizzleTests = {
  .name = "Tetrodotoxin::Library::Language::Access::Swizzle"_view,
};

static auto interpret(Workspace& workspace, Errors& errors, View::Bytes source)
    -> Option<Language::Monograph&> {
  BAIL_IF(!workspace.install_dialect<Dialect>("Library"_view));

  auto interpreted = workspace.interpret_source(
      errors, "SwizzleTest"_view, "swizzle.ttx"_view, source);
  BAIL_IF(!interpreted || !interpreted->is<Language::Monograph>());
  return static_cast<Language::Monograph&>(*interpreted);
}

static auto rejects_interpretation(View::Bytes source) -> Bool {
  Workspace workspace;
  Errors errors;
  return !interpret(workspace, errors, source) && !errors.is_empty();
}

static auto rejects_link(View::Bytes source) -> Bool {
  Workspace workspace;
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  return monograph && !workspace.link(errors) && !errors.is_empty();
}

static auto find_field(
    const Language::Types::Composite& composite,
    View::Bytes name) -> Option<const Language::Field&> {
  auto fields = composite.get_addressables();
  for (auto field = fields.begin(); field != fields.end(); ++field) {
    const Abstract& candidate = (*field).get();
    if (candidate.get_name() == name && candidate.is<Language::Field>()) {
      return static_cast<const Language::Field&>(candidate);
    }
  }

  return {};
}

static auto find_function(
    const Language::Types::Composite& composite,
    View::Bytes name) -> Option<const Language::Function&> {
  auto functions = composite.get_callables();
  for (auto function = functions.begin(); function != functions.end();
       ++function) {
    const Abstract& candidate = (*function).get();
    if (candidate.get_name() == name && candidate.is<Language::Function>()) {
      return static_cast<const Language::Function&>(candidate);
    }
  }

  return {};
}

PERIMORTEM_UNIT_TEST(SwizzleTests, exact_layout_fitting_and_precedence) {
  static constexpr View::Bytes source =
      "// Swizzle value flow.\n"
      "dialect : Library;\n"
      "public Packet : struct {\n"
      "  private secret : Unsigned_64;\n"
      "  public width : Unsigned_64;\n"
      "  public height : Unsigned_64;\n"
      "  public gather : func = [self] -> [Unsigned_64, Unsigned_64] {\n"
      "    return self.[secret, height];\n"
      "  }\n"
      "  public echo : func = [.value : Packet] -> Packet {\n"
      "    return value;\n"
      "  }\n"
      "}\n"
      "public Empty : struct {}\n"
      "public Dimensions : struct {\n"
      "  public first : Unsigned_64; public second : Unsigned_64;\n"
      "}\n"
      "private packet : Packet;\n"
      "private empty : Empty = packet.[];\n"
      "private single : Unsigned_64 = packet.[width,];\n"
      "private dimensions : Dimensions = "
      "packet.[height, width];\n"
      "private chained : Unsigned_64 = "
      "Packet -> echo(packet).[width] + 1;"_view;
  Workspace workspace;
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  ASSERT(monograph);
  ASSERT(workspace.link(errors));
  ASSERT(workspace.finalize(errors));

  const Abstract& packet_identity =
      monograph->get_source().resolve_context("Packet"_view);
  ASSERT(packet_identity.is<Language::Types::Structure>());
  const auto& packet =
      static_cast<const Language::Types::Structure&>(packet_identity);
  auto secret = find_field(packet, "secret"_view);
  auto width = find_field(packet, "width"_view);
  auto height = find_field(packet, "height"_view);
  auto gather = find_function(packet, "gather"_view);
  ASSERT(secret);
  ASSERT(width);
  ASSERT(height);
  ASSERT(gather && gather->get_return_expression());

  auto empty = find_field(monograph->get_source(), "empty"_view);
  auto single = find_field(monograph->get_source(), "single"_view);
  auto dimensions = find_field(monograph->get_source(), "dimensions"_view);
  auto chained = find_field(monograph->get_source(), "chained"_view);
  ASSERT(empty && empty->get_initializer());
  ASSERT(single && single->get_initializer());
  ASSERT(dimensions && dimensions->get_initializer());
  ASSERT(chained && chained->get_initializer());
  ASSERT(empty->get_initializer()->is<Language::Access::Swizzle>());
  ASSERT(single->get_initializer()->is<Language::Access::Swizzle>());
  ASSERT(dimensions->get_initializer()->is<Language::Access::Swizzle>());
  ASSERT(gather->get_return_expression()->is<Language::Access::Swizzle>());

  const auto& empty_swizzle =
      static_cast<const Language::Access::Swizzle&>(*empty->get_initializer());
  const auto& single_swizzle =
      static_cast<const Language::Access::Swizzle&>(*single->get_initializer());
  const auto& dimensions_swizzle =
      static_cast<const Language::Access::Swizzle&>(
          *dimensions->get_initializer());
  const auto& private_swizzle = static_cast<const Language::Access::Swizzle&>(
      *gather->get_return_expression());

  EXPECT(empty_swizzle.get_results().is_empty());
  EXPECT(&empty_swizzle.get_type() == &Invalid::get_invalid());
  EXPECT(empty_swizzle.fits(empty->get_type()));
  ASSERT_EQ(single_swizzle.get_results().get_size(), Count(1));
  EXPECT(&*single_swizzle.get_results().get_abstract(0) == &*width);
  EXPECT(&single_swizzle.get_type() == &Dialect::get_unsigned_64());
  EXPECT(single_swizzle.fits(single->get_type()));
  ASSERT_EQ(dimensions_swizzle.get_results().get_size(), Count(2));
  EXPECT(&*dimensions_swizzle.get_results().get_abstract(0) == &*height);
  EXPECT(&*dimensions_swizzle.get_results().get_abstract(1) == &*width);
  EXPECT(&dimensions_swizzle.get_type() == &Invalid::get_invalid());
  EXPECT(dimensions_swizzle.fits(dimensions->get_type()));
  Language::Types::Fixed fixed_pair(
      "Fixed[Unsigned_64, 2]"_view, Dialect::get_unsigned_64(), 2);
  Language::Types::Fixed fixed_empty(
      "Fixed[Unsigned_64, 0]"_view, Dialect::get_unsigned_64(), 0);
  EXPECT(dimensions_swizzle.fits(fixed_pair));
  EXPECT(empty_swizzle.fits(fixed_empty));
  ASSERT_EQ(private_swizzle.get_results().get_size(), Count(2));
  EXPECT(&*private_swizzle.get_results().get_abstract(0) == &*secret);
  EXPECT(&*private_swizzle.get_results().get_abstract(1) == &*height);

  Perimortem::Memory::Allocator::Arena relink_domain;
  Tokenizer relink_tokens(
      relink_domain, "packet.[height, width]"_view, "relink-swizzle.ttx"_view);
  Cursor relink_cursor(relink_tokens, errors);
  auto relink_expression = Language::Parser::Expression::parse(
      relink_domain, monograph->get_materializations(), relink_cursor,
      *dimensions);
  ASSERT(relink_expression && relink_cursor.matches(Code::Type::Terminal));
  ASSERT(relink_expression->is<Language::Access::Swizzle>());
  auto& relink_swizzle =
      static_cast<Language::Access::Swizzle&>(*relink_expression);
  ASSERT(relink_swizzle.link(
      *monograph, *dimensions, monograph->get_materializations()));
  ASSERT(relink_swizzle.link(
      *monograph, *dimensions, monograph->get_materializations()));
  ASSERT_EQ(relink_swizzle.get_results().get_size(), Count(2));
  EXPECT(&*relink_swizzle.get_results().get_abstract(0) == &*height);
  EXPECT(&*relink_swizzle.get_results().get_abstract(1) == &*width);

  ASSERT(chained->get_initializer()->is<Language::Operations::Add>());
  auto chained_left = chained->get_initializer()->get_inputs().get_abstract(0);
  ASSERT(chained_left && chained_left->is<Language::Access::Swizzle>());
  const auto& chained_swizzle =
      static_cast<const Language::Access::Swizzle&>(*chained_left);
  EXPECT(chained_swizzle.get_receiver().is<Language::Access::Call>());
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(SwizzleTests, invalid_selections_are_rejected) {
  static constexpr Static::Vector<View::Bytes, 3> sources = {{
    "// Unknown Swizzle name.\ndialect : Library;\n"
    "public Packet : struct { public width : Unsigned_64; }\n"
    "private packet : Packet;\n"
    "private invalid : Unsigned_64 = packet.[missing];"_view,
    "// Inaccessible Swizzle name.\ndialect : Library;\n"
    "public Packet : struct { private secret : Unsigned_64; }\n"
    "private packet : Packet;\n"
    "private invalid : Unsigned_64 = packet.[secret];"_view,
    "// Incompatible Swizzle shape.\ndialect : Library;\n"
    "public Packet : struct {\n"
    "  public width : Unsigned_64; public height : Unsigned_64;\n"
    "}\n"
    "public Pair : struct { public first : Bool; public second : Bool; }\n"
    "private packet : Packet;\n"
    "private invalid : Pair = packet.[width, height];"_view,
  }};

  for (Count index = 0; index < sources.get_size(); index++) {
    EXPECT(rejects_link(sources[index]));
  }
}

PERIMORTEM_UNIT_TEST(SwizzleTests, malformed_selection_is_atomic) {
  static constexpr Static::Vector<View::Bytes, 3> sources = {{
    "// Missing Swizzle close.\ndialect : Library;\n"
    "public Packet : struct { public width : Unsigned_64; }\n"
    "private packet : Packet; private invalid := packet.[width;"_view,
    "// Missing Swizzle name.\ndialect : Library;\n"
    "public Packet : struct { public width : Unsigned_64; }\n"
    "private packet : Packet; private invalid := packet.[,width];"_view,
    "// Repeated Swizzle separator.\ndialect : Library;\n"
    "public Packet : struct { public width : Unsigned_64; }\n"
    "private packet : Packet; private invalid := packet.[width,,];"_view,
  }};

  for (Count index = 0; index < sources.get_size(); index++) {
    EXPECT(rejects_interpretation(sources[index]));
  }
}
