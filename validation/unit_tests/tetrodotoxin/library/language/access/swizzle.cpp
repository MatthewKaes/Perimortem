// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/access/swizzle.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/static/vector.hpp"

#include "tetrodotoxin/environment/workspace.hpp"
#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/language/access/address.hpp"
#include "tetrodotoxin/library/language/access/call.hpp"
#include "tetrodotoxin/library/language/field.hpp"
#include "tetrodotoxin/library/language/flow/return.hpp"
#include "tetrodotoxin/library/language/function.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "tetrodotoxin/library/language/operation.hpp"
#include "tetrodotoxin/library/language/operations/add.hpp"
#include "tetrodotoxin/library/language/types/composite.hpp"
#include "tetrodotoxin/library/language/types/fixed.hpp"
#include "tetrodotoxin/library/language/types/source.hpp"
#include "tetrodotoxin/library/language/types/structure.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/lexical/errors.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Library;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using Tetrodotoxin::Environment::Workspace;
using namespace Validation;

static Harness SwizzleTests = {
  .name = "Tetrodotoxin::Library::Language::Access::Swizzle"_view,
};

static auto find_return(const Language::Function& function)
    -> Option<const Language::Flow::Return&> {
  auto body = function.get_body();
  BAIL_IF(!body);
  for (const Language::Statement& statement : body->get_statements()) {
    auto returned = statement.get_root().select<Language::Flow::Return>();
    if (returned) {
      return *returned;
    }
  }

  return {};
}

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
  return !monograph && !errors.is_empty();
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

static auto is_projection(
    const Language::Access::Swizzle& swizzle,
    Count index,
    const Language::Field& field) -> Bool {
  auto output = swizzle.get_layout().get_abstract(index);
  BAIL_IF(!output || !output->is<Language::Access::Address>());

  const auto& projection =
      static_cast<const Language::Access::Address&>(*output);
  return &projection.get_receiver() == &swizzle.get_receiver() &&
         &projection.get_result() == &field;
}

PERIMORTEM_UNIT_TEST(SwizzleTests, exact_layout_fitting_and_precedence) {
  static constexpr View::Bytes source =
      "// Swizzle value flow.\n"
      "dialect : Library;\n"
      "public Packet : struct {\n"
      "  private state secret : Unsigned_64;\n"
      "  public state width : Unsigned_64;\n"
      "  public state height : Unsigned_64;\n"
      "  public gather : func = [self] -> [Unsigned_64, Unsigned_64] {\n"
      "    return self.[secret, height];\n"
      "  }\n"
      "  public echo : func = [.value : Packet] -> Packet {\n"
      "    return value;\n"
      "  }\n"
      "}\n"
      "public Dimensions : struct {\n"
      "  public state first : Unsigned_64; public state second : Unsigned_64;\n"
      "}\n"
      "private packet : Packet;\n"
      "private empty : func = [] -> [] { return packet.[]; }\n"
      "private single : Unsigned_64 = packet.[width,];\n"
      "private dimensions : Dimensions = "
      "packet.[height, width];\n"
      "private chained : Unsigned_64 = "
      "Packet -> echo(packet).[width] + 1;"_view;
  Workspace workspace;
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  ASSERT(monograph);

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
  ASSERT(gather);
  auto gather_return = find_return(*gather);
  ASSERT(gather_return);
  const Abstract& unsigned_64 = monograph->resolve_context("Unsigned_64"_view);
  EXPECT_TEXT(
      gather_return->get_anchor().get_span().caculate_text(source),
      "return self.[secret, height];"_view);
  ASSERT_EQ(gather->get_results().get_size(), Count(2));
  EXPECT(&*gather->get_results().get_abstract(0) == &unsigned_64);
  EXPECT(&*gather->get_results().get_abstract(1) == &unsigned_64);

  auto empty = find_function(monograph->get_source(), "empty"_view);
  auto single = find_field(monograph->get_source(), "single"_view);
  auto dimensions = find_field(monograph->get_source(), "dimensions"_view);
  auto chained = find_field(monograph->get_source(), "chained"_view);
  ASSERT(empty);
  auto empty_return = find_return(*empty);
  ASSERT(empty_return);
  EXPECT_TEXT(
      empty_return->get_anchor().get_span().caculate_text(source),
      "return packet.[];"_view);
  EXPECT(empty->get_results().is_empty());
  ASSERT(single && single->get_initializer());
  ASSERT(dimensions && dimensions->get_initializer());
  ASSERT(chained && chained->get_initializer());
  ASSERT(single->get_initializer()->is<Language::Access::Swizzle>());
  ASSERT(dimensions->get_initializer()->is<Language::Access::Swizzle>());

  const auto& single_swizzle =
      static_cast<const Language::Access::Swizzle&>(*single->get_initializer());
  const auto& dimensions_swizzle =
      static_cast<const Language::Access::Swizzle&>(
          *dimensions->get_initializer());

  ASSERT_EQ(single_swizzle.get_layout().get_size(), Count(1));
  EXPECT(is_projection(single_swizzle, 0, *width));
  EXPECT(&single_swizzle.get_type() == &unsigned_64);
  EXPECT(single_swizzle.fits(single->get_type()));
  ASSERT_EQ(dimensions_swizzle.get_layout().get_size(), Count(2));
  EXPECT(is_projection(dimensions_swizzle, 0, *height));
  EXPECT(is_projection(dimensions_swizzle, 1, *width));
  EXPECT(&dimensions_swizzle.get_type() == &Invalid::get_invalid());
  EXPECT(dimensions_swizzle.fits(dimensions->get_type()));
  Language::Types::Fixed fixed_pair(
      "Fixed[Unsigned_64, 2]"_view,
      static_cast<const Language::Model::Type&>(unsigned_64), 2);
  EXPECT(dimensions_swizzle.fits(fixed_pair));

  ASSERT(chained->get_initializer()->is<Language::Operations::Add>());
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(SwizzleTests, named_pack_reorders_real_producers) {
  static constexpr View::Bytes source =
      "// Named Pack Swizzle.\n"
      "dialect : Library;\n"
      "public Pair : struct {\n"
      "  public state first : Bool; public state second : Unsigned_64;\n"
      "}\n"
      "private left : Unsigned_64 = 7;\n"
      "private right : Bool = false;\n"
      "private reordered : Pair = "
      "(.x = left, .y = right).[y, x];"_view;
  Workspace workspace;
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  ASSERT(monograph);

  auto left = find_field(monograph->get_source(), "left"_view);
  auto right = find_field(monograph->get_source(), "right"_view);
  auto reordered = find_field(monograph->get_source(), "reordered"_view);
  ASSERT(left && right && reordered && reordered->get_initializer());
  ASSERT(reordered->get_initializer()->is<Language::Access::Swizzle>());

  const auto& swizzle = static_cast<const Language::Access::Swizzle&>(
      *reordered->get_initializer());
  const Layout& receiver = swizzle.get_receiver().get_layout();
  const Layout& output = swizzle.get_layout();
  ASSERT_EQ(receiver.get_size(), Count(2));
  ASSERT_EQ(output.get_size(), Count(2));
  auto x_name = receiver.get_name(0);
  auto y_name = receiver.get_name(1);
  ASSERT(x_name && y_name);
  EXPECT_TEXT(*x_name, "x"_view);
  EXPECT_TEXT(*y_name, "y"_view);
  EXPECT(!output.get_name(0));
  EXPECT(!output.get_name(1));

  auto source_x = receiver.get_abstract(0);
  auto source_y = receiver.get_abstract(1);
  auto selected_y = output.get_abstract(0);
  auto selected_x = output.get_abstract(1);
  ASSERT(source_x && source_y && selected_y && selected_x);
  EXPECT(&*selected_y == &*source_y);
  EXPECT(&*selected_x == &*source_x);
  auto y_producer = selected_y->select<Language::Expression>();
  auto x_producer = selected_x->select<Language::Expression>();
  ASSERT(y_producer && x_producer);
  EXPECT(&y_producer->get_result() == &*right);
  EXPECT(&x_producer->get_result() == &*left);
  EXPECT(swizzle.fits(reordered->get_type()));
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(
    SwizzleTests,
    named_call_preserves_selected_descriptor_and_producer) {
  static constexpr View::Bytes source =
      "// Named Call result Swizzle.\n"
      "dialect : Library;\n"
      "public Results : struct {\n"
      "  public produce : func = [] -> [\n"
      "    .count : Unsigned_64, .flag : Bool,\n"
      "  ] {\n"
      "    return (.flag = false, .count = 7);\n"
      "  }\n"
      "}\n"
      "public Reordered : struct {\n"
      "  public state first : Bool; public state second : Unsigned_64;\n"
      "}\n"
      "public Original : struct {\n"
      "  public state first : Unsigned_64; public state second : Bool;\n"
      "}\n"
      "private reordered : Reordered = "
      "Results -> produce().[flag, count];\n"
      "private selected : Bool = Results -> produce().[flag];"_view;
  Workspace workspace;
  Errors errors;
  auto monograph = interpret(workspace, errors, source);
  ASSERT(monograph);

  auto reordered = find_field(monograph->get_source(), "reordered"_view);
  auto selected = find_field(monograph->get_source(), "selected"_view);
  ASSERT(reordered && selected);
  ASSERT(reordered->get_initializer() && selected->get_initializer());
  ASSERT(reordered->get_initializer()->is<Language::Access::Swizzle>());
  ASSERT(selected->get_initializer()->is<Language::Access::Swizzle>());

  const auto& reordered_swizzle = static_cast<const Language::Access::Swizzle&>(
      *reordered->get_initializer());
  const auto& selected_swizzle = static_cast<const Language::Access::Swizzle&>(
      *selected->get_initializer());
  ASSERT(reordered_swizzle.get_receiver().is<Language::Access::Call>());
  ASSERT(selected_swizzle.get_receiver().is<Language::Access::Call>());
  const auto& reordered_call = static_cast<const Language::Access::Call&>(
      reordered_swizzle.get_receiver());
  const auto& selected_call = static_cast<const Language::Access::Call&>(
      selected_swizzle.get_receiver());

  const Layout& call_output = reordered_call.get_layout();
  ASSERT_EQ(call_output.get_size(), Count(2));
  auto count_name = call_output.get_name(0);
  auto flag_name = call_output.get_name(1);
  ASSERT(count_name && flag_name);
  EXPECT_TEXT(*count_name, "count"_view);
  EXPECT_TEXT(*flag_name, "flag"_view);
  auto count_producer = call_output.get_abstract(0);
  auto flag_producer = call_output.get_abstract(1);
  ASSERT(count_producer && flag_producer);
  EXPECT(&*count_producer == &reordered_call);
  EXPECT(&*flag_producer == &reordered_call);

  const Layout& reordered_output = reordered_swizzle.get_layout();
  ASSERT_EQ(reordered_output.get_size(), Count(2));
  EXPECT(!reordered_output.get_name(0));
  EXPECT(!reordered_output.get_name(1));
  EXPECT(reordered_swizzle.fits(reordered->get_type()));
  const Abstract& original_identity =
      monograph->get_source().resolve_context("Original"_view);
  ASSERT(original_identity.is<Language::Types::Structure>());
  EXPECT(!reordered_swizzle.fits(
      static_cast<const Language::Types::Structure&>(original_identity)));

  for (Count index = 0; index < reordered_output.get_size(); index++) {
    EXPECT(
        reordered_output.get_fitted(reordered->get_type().get_layout(), index)
            .visit(
                [&](const Abstract& producer) {
                  return Bool(&producer == &reordered_call);
                },
                [](Layout::Errors) { return False; }));
  }

  const Layout& selected_output = selected_swizzle.get_layout();
  ASSERT_EQ(selected_output.get_size(), Count(1));
  EXPECT(selected_swizzle.fits(selected->get_type()));
  EXPECT(selected_output.get_fitted(selected->get_type().get_layout(), 0)
             .visit(
                 [&](const Abstract& producer) {
                   return Bool(&producer == &selected_call);
                 },
                 [](Layout::Errors) { return False; }));
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(SwizzleTests, invalid_selections_are_rejected) {
  static constexpr Static::Vector<View::Bytes, 4> sources = {{
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
    "// Unknown named Pack slot.\ndialect : Library;\n"
    "private value : Unsigned_64 = 0;\n"
    "private invalid : Unsigned_64 = (.known = value).[missing];"_view,
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
