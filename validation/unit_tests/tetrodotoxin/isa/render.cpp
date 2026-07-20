// // Perimortem Engine
// // Copyright © Matt Kaes

// #include "validation/unit_test.hpp"

// #include "tetrodotoxin/puffer/resolution/resolver.hpp"
// #include "tetrodotoxin/puffer/toolchain.hpp"
// #include "ttx/type.hpp"

// using namespace Perimortem::Core;
// using namespace Tetrodotoxin::Puffer;
// using namespace Validation;

// static Harness TtxRender = {
//   .name = "TTX::Render"_view,
// };

// static auto first_error(const Resolution::Resolver::Context& source_context)
//     -> View::Bytes {
//   return source_context.get_errors()[0].get_message();
// }

// static auto render_type(const Resolution::Source::Record* record)
//     -> const Ttx::Type* {
//   if (record == nullptr) {
//     return nullptr;
//   }

//   return record->get_type().find_type("Render2D"_view);
// }

// PERIMORTEM_UNIT_TEST(TtxRender, render_shape) {
//   Tetrodotoxin::Isa::Registry isa_registry =
//       Tetrodotoxin::Puffer::Toolchain::standard_registry();
//   Resolution::Resolver resolver(isa_registry);
//   Resolution::Resolver::Context render_source_context;

//   const Resolution::Source::Record* record = resolver.load_source(
//       render_source_context, "unit/render.ttx"_view,
//       "dialect : Render;\n"
//       "public Render2D : Render {\n"
//       "  public position : Vec2D;\n"
//       "  public vertex : stage {\n"
//       "    input [.vertex_index : Unsigned_32];\n"
//       "    output [.screen_position : Vec4D];\n"
//       "  }\n"
//       "}\n"_view);

//   ASSERT(record != nullptr);
//   EXPECT_NOT(render_source_context.has_errors());
//   EXPECT_TEXT(record->get_type().get_name(), "Render"_view);
//   const Ttx::Type* render = render_type(record);
//   ASSERT(render != nullptr);
//   EXPECT(render->find_member("position"_view) != nullptr);

//   const Ttx::Function* vertex = render->find_type_function("vertex"_view);
//   ASSERT(vertex != nullptr);
//   ASSERT_EQ(vertex->get_parameters().get_member_count(), Count(1));
//   ASSERT_EQ(vertex->get_result().get_member_count(), Count(1));
//   EXPECT_TEXT(
//       vertex->get_parameters().member_at(0).get_name(), "vertex_index"_view);
//   EXPECT_TEXT(
//       vertex->get_result().member_at(0).get_name(), "screen_position"_view);
// }

// PERIMORTEM_UNIT_TEST(TtxRender, fact_blocks) {
//   Tetrodotoxin::Isa::Registry isa_registry =
//       Tetrodotoxin::Puffer::Toolchain::standard_registry();
//   Resolution::Resolver resolver(isa_registry);
//   Resolution::Resolver::Context render_source_context;

//   const Resolution::Source::Record* record = resolver.load_source(
//       render_source_context, "unit/render.ttx"_view,
//       "dialect : Render;\n"
//       "public Render2D : Render {\n"
//       "  constants {\n"
//       "    const quad_count : Unsigned_32;\n"
//       "  }\n"
//       "  push_constants {\n"
//       "    const position : Vec2D;\n"
//       "  }\n"
//       "  resources {\n"
//       "    state texture : View[Bytes];\n"
//       "  }\n"
//       "}\n"_view);

//   ASSERT(record != nullptr);
//   EXPECT_NOT(render_source_context.has_errors());
//   const Ttx::Type* render = render_type(record);
//   ASSERT(render != nullptr);
//   ASSERT(render->find_type("constant"_view) != nullptr);
//   ASSERT(render->find_type("push"_view) != nullptr);
//   ASSERT(render->find_type("resource"_view) != nullptr);
//   EXPECT(
//       render->find_type("constant"_view)->find_member("quad_count"_view) !=
//       nullptr);
//   EXPECT(
//       render->find_type("push"_view)->find_member("position"_view) !=
//       nullptr);
//   EXPECT(
//       render->find_type("resource"_view)->find_member("texture"_view) !=
//       nullptr);
// }

// PERIMORTEM_UNIT_TEST(TtxRender, stage_reads) {
//   Tetrodotoxin::Isa::Registry isa_registry =
//       Tetrodotoxin::Puffer::Toolchain::standard_registry();
//   Resolution::Resolver resolver(isa_registry);
//   Resolution::Resolver::Context render_source_context;

//   const Resolution::Source::Record* record = resolver.load_source(
//       render_source_context, "unit/render.ttx"_view,
//       "dialect : Render;\n"
//       "public Render2D : Render {\n"
//       "  constants { const quad_count : Unsigned_32; }\n"
//       "  push_constants { const position : Vec2D; }\n"
//       "  resources { state texture : View[Bytes]; }\n"
//       "  public vertex : stage {\n"
//       "    reads constant[quad_count];\n"
//       "    reads push[position];\n"
//       "    reads resource[texture];\n"
//       "    input [];\n"
//       "    output [];\n"
//       "  }\n"
//       "}\n"_view);

//   ASSERT(record != nullptr);
//   EXPECT_NOT(render_source_context.has_errors());
//   const Ttx::Type* render = render_type(record);
//   ASSERT(render != nullptr);
//   const Ttx::Type* vertex = render->find_type("vertex"_view);
//   ASSERT(vertex != nullptr);
//   EXPECT(vertex->find_type("constant"_view)->find_member("quad_count"_view));
//   EXPECT(vertex->find_type("push"_view)->find_member("position"_view));
//   EXPECT(vertex->find_type("resource"_view)->find_member("texture"_view));
// }

// PERIMORTEM_UNIT_TEST(TtxRender, duplicate_stage) {
//   Tetrodotoxin::Isa::Registry isa_registry =
//       Tetrodotoxin::Puffer::Toolchain::standard_registry();
//   Resolution::Resolver resolver(isa_registry);
//   Resolution::Resolver::Context render_source_context;

//   EXPECT_NOT(resolver.load_source(
//       render_source_context, "unit/render.ttx"_view,
//       "dialect : Render;\n"
//       "public Render2D : Render {\n"
//       "  public vertex : stage { input []; output []; }\n"
//       "  public vertex : stage { input []; output []; }\n"
//       "}\n"_view));

//   ASSERT(render_source_context.has_errors());
//   EXPECT_TEXT(
//       first_error(render_source_context),
//       "Render stage name is already defined."_view);
// }

// PERIMORTEM_UNIT_TEST(TtxRender, missing_read) {
//   Tetrodotoxin::Isa::Registry isa_registry =
//       Tetrodotoxin::Puffer::Toolchain::standard_registry();
//   Resolution::Resolver resolver(isa_registry);
//   Resolution::Resolver::Context render_source_context;

//   EXPECT_NOT(resolver.load_source(
//       render_source_context, "unit/render.ttx"_view,
//       "dialect : Render;\n"
//       "public Render2D : Render {\n"
//       "  push_constants { const position : Vec2D; }\n"
//       "  public vertex : stage {\n"
//       "    reads push[missing];\n"
//       "    input [];\n"
//       "    output [];\n"
//       "  }\n"
//       "}\n"_view));

//   ASSERT(render_source_context.has_errors());
//   EXPECT_TEXT(
//       first_error(render_source_context),
//       "Render stage read fact could not be resolved."_view);
// }

// PERIMORTEM_UNIT_TEST(TtxRender, bad_member_type) {
//   Tetrodotoxin::Isa::Registry isa_registry =
//       Tetrodotoxin::Puffer::Toolchain::standard_registry();
//   Resolution::Resolver resolver(isa_registry);
//   Resolution::Resolver::Context render_source_context;

//   EXPECT_NOT(resolver.load_source(
//       render_source_context, "unit/render.ttx"_view,
//       "dialect : Render;\n"
//       "public Render2D : Render {\n"
//       "  public position : MissingType;\n"
//       "}\n"_view));

//   ASSERT(render_source_context.has_errors());
//   EXPECT_TEXT(
//       first_error(render_source_context),
//       "Render member type could not be resolved."_view);
// }
