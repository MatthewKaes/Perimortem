// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/graphics/runtime/submission.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/data.hpp"
#include "perimortem/core/object.h"

#include "perimortem/memory/dynamic/bytes.hpp"

using namespace Perimortem;
using namespace Perimortem::Graphics::Frame;
using namespace Tetrodotoxin::Graphics::Runtime;
using namespace Validation;

static Harness GraphicsSubmission = {
  .name = "Tetrodotoxin::Graphics::Submission"_view,
};

alignas(U32) static constexpr U32 submission_program[] = {0x07230203};
static Count finalized_resources = 0;

class SubmissionResource {
 public:
  static auto create(U8 value) -> U8*;

 private:
  static auto finalize(U8*) -> void;
  static const perimortem_object_descriptor descriptor;
};

const perimortem_object_descriptor SubmissionResource::descriptor{
  .size = sizeof(U8),
  .alignment = alignof(U8),
  .finalize = SubmissionResource::finalize,
};

auto SubmissionResource::create(U8 value) -> U8* {
  U8* object = perimortem_core_object_allocate(&descriptor);
  *object = value;
  return object;
}

auto SubmissionResource::finalize(U8*) -> void {
  finalized_resources++;
}

class SubmissionNode {
 public:
  static auto create() -> U8*;
  static auto get(U8* object) -> SubmissionNode&;

  ~SubmissionNode();

  auto add_child(U8* child) -> Bool;
  auto clear_children() -> void;
  auto set_resource(U8* selected) -> void;

  Transform transform;
  Bool visible = True;
  Count draw_count = 0;
  S64 z_index = 0;
  Memory::Dynamic::Bytes inputs;

  static const Placement2D placement;
  static const Children2D children;
  static const Drawable2D drawable;
  static auto placements() -> Core::View::Vector<const Placement2D*>;
  static auto child_capabilities() -> Core::View::Vector<const Children2D*>;
  static auto drawables() -> Core::View::Vector<const Drawable2D*>;

 private:
  static auto finalize(U8* payload) -> void;
  static auto read_placement(const U8* context, U8* object)
      -> Placement2D::Placement;
  static auto read_child_count(const U8* context, U8* object) -> Count;
  static auto read_child(const U8* context, U8* object, Count index, U8** child)
      -> Count;
  static auto read_draw_count(U8* object) -> Count;
  static auto read_draw(U8* object, Count index) -> Drawable2D::Draw;

  static const perimortem_object_descriptor object_descriptor;
  U8* retained_children[4] = {};
  Count retained_child_count = 0;
  U8* resource = nullptr;
};

const perimortem_object_descriptor SubmissionNode::object_descriptor{
  .size = sizeof(SubmissionNode),
  .alignment = alignof(SubmissionNode),
  .finalize = SubmissionNode::finalize,
};

const Placement2D SubmissionNode::placement(
    nullptr,
    SubmissionNode::read_placement);

const Children2D SubmissionNode::children(
    nullptr,
    SubmissionNode::read_child_count,
    SubmissionNode::read_child);

const Drawable2D SubmissionNode::drawable(
    SubmissionNode::read_draw_count,
    SubmissionNode::read_draw);

auto SubmissionNode::placements() -> Core::View::Vector<const Placement2D*> {
  static const Placement2D* values[] = {&placement};
  return values;
}

auto SubmissionNode::child_capabilities()
    -> Core::View::Vector<const Children2D*> {
  static const Children2D* values[] = {&children};
  return values;
}

auto SubmissionNode::drawables() -> Core::View::Vector<const Drawable2D*> {
  static const Drawable2D* values[] = {&drawable};
  return values;
}

auto SubmissionNode::create() -> U8* {
  U8* object = perimortem_core_object_allocate(&object_descriptor);
  new (object, Core::Placement::Construct) SubmissionNode();
  return object;
}

auto SubmissionNode::get(U8* object) -> SubmissionNode& {
  return *Core::Data::cast<SubmissionNode>(object);
}

SubmissionNode::~SubmissionNode() {
  clear_children();
  perimortem_core_object_release(resource);
}

auto SubmissionNode::add_child(U8* child) -> Bool {
  BAIL_IF(child == nullptr || retained_child_count == 4);
  perimortem_core_object_retain(child);
  retained_children[retained_child_count++] = child;
  return True;
}

auto SubmissionNode::clear_children() -> void {
  for (Count index = 0; index < retained_child_count; index++) {
    perimortem_core_object_release(retained_children[index]);
    retained_children[index] = nullptr;
  }
  retained_child_count = 0;
}

auto SubmissionNode::set_resource(U8* selected) -> void {
  perimortem_core_object_retain(selected);
  perimortem_core_object_release(resource);
  resource = selected;
}

auto SubmissionNode::finalize(U8* payload) -> void {
  Core::Data::cast<SubmissionNode>(payload)->~SubmissionNode();
}

auto SubmissionNode::read_placement(const U8*, U8* object)
    -> Placement2D::Placement {
  const SubmissionNode& node = get(object);
  return Placement2D::Placement(node.transform, node.visible, node.z_index);
}

auto SubmissionNode::read_child_count(const U8*, U8* object) -> Count {
  return get(object).retained_child_count;
}

auto SubmissionNode::read_child(const U8*, U8* object, Count index, U8** child)
    -> Count {
  const SubmissionNode& node = get(object);
  if (child == nullptr || index >= node.retained_child_count) {
    return Count(-1);
  }
  *child = node.retained_children[index];
  return 0;
}

auto SubmissionNode::read_draw_count(U8* object) -> Count {
  return get(object).draw_count;
}

auto SubmissionNode::read_draw(U8* object, Count index) -> Drawable2D::Draw {
  const SubmissionNode& node = get(object);
  Memory::Dynamic::Vector<Resource> resources;
  if (node.resource != nullptr) {
    resources.emplace(Resource(node.resource));
  }
  Memory::Dynamic::Bytes inputs(node.inputs.get_view());
  return Drawable2D::Draw(
      Program(Core::Data::cast<const U8>(submission_program)),
      static_cast<Memory::Dynamic::Vector<Resource>&&>(resources),
      static_cast<Memory::Dynamic::Bytes&&>(inputs), {1, 1},
      Perimortem::Graphics::Frame::Pipeline(
          Perimortem::Graphics::Frame::Pipeline::Topology::TriangleList,
          Perimortem::Graphics::Frame::Pipeline::Blend::Alpha,
          Perimortem::Graphics::Frame::Pipeline::Geometry::UnitQuad2D),
      6, -S64(index));
}

static auto submit(U8* root) -> Core::Option<PassUI> {
  return PassUI::create(
      root, SubmissionNode::children, SubmissionNode::placements(),
      SubmissionNode::child_capabilities(), SubmissionNode::drawables());
}

PERIMORTEM_UNIT_TEST(GraphicsSubmission, stabilizes_native_frame) {
  finalized_resources = 0;
  U8* resource = SubmissionResource::create(0x5A);
  U8* root = SubmissionNode::create();
  U8* back = SubmissionNode::create();
  U8* front = SubmissionNode::create();
  SubmissionNode& back_node = SubmissionNode::get(back);
  SubmissionNode& front_node = SubmissionNode::get(front);
  ASSERT(SubmissionNode::get(root).add_child(front));
  ASSERT(SubmissionNode::get(root).add_child(back));

  front_node.draw_count = 1;
  front_node.z_index = 4;
  front_node.inputs = "front"_view;
  front_node.transform = Transform::create(1.0, 2.0, 1.0, 1.0, 0.0);
  front_node.set_resource(resource);
  back_node.draw_count = 1;
  back_node.z_index = -2;
  back_node.inputs = "back"_view;
  back_node.transform = Transform::create(3.0, 5.0, 1.0, 1.0, 0.0);
  back_node.set_resource(resource);

  {
    auto submission = submit(root);
    ASSERT(submission);
    auto batches = submission->get_batches();
    ASSERT_EQ(batches.get_size(), Count(2));
    const Batch& back_batch = batches.get_data()[0];
    const Batch& front_batch = batches.get_data()[1];
    EXPECT_EQ(back_batch.get_inputs(), "back"_view);
    EXPECT_EQ(front_batch.get_inputs(), "front"_view);
    EXPECT_EQ(back_batch.get_z_index(), S64(-2));
    EXPECT_EQ(front_batch.get_z_index(), S64(4));
    EXPECT_EQ(back_batch.get_transform().get_x(), R64(3.0));
    EXPECT_EQ(back_batch.get_transform().get_y(), R64(5.0));
    EXPECT_EQ(front_batch.get_transform().get_x(), R64(1.0));
    EXPECT_EQ(front_batch.get_transform().get_y(), R64(2.0));
    EXPECT_EQ(back_batch.get_vertex_count(), Count(6));
    EXPECT(
        back_batch.get_pipeline().get_geometry() ==
        Perimortem::Graphics::Frame::Pipeline::Geometry::UnitQuad2D);
    EXPECT(
        back_batch.get_pipeline().get_topology() ==
        Perimortem::Graphics::Frame::Pipeline::Topology::TriangleList);
    EXPECT(
        back_batch.get_pipeline().get_blend() ==
        Perimortem::Graphics::Frame::Pipeline::Blend::Alpha);

    back_node.inputs = "changed"_view;
    back_node.set_resource({});
    front_node.set_resource({});
    perimortem_core_object_release(resource);
    resource = nullptr;
    perimortem_core_object_release(back);
    back = nullptr;
    perimortem_core_object_release(front);
    front = nullptr;
    perimortem_core_object_release(root);
    root = nullptr;

    EXPECT_EQ(back_batch.get_inputs(), "back"_view);
    ASSERT_EQ(back_batch.get_resources().get_size(), Count(1));
    EXPECT_EQ(
        back_batch.get_resources().get_data()[0].get_reservations(), Count(2));
    EXPECT_EQ(finalized_resources, Count(0));
  }
  EXPECT_EQ(finalized_resources, Count(1));
}

PERIMORTEM_UNIT_TEST(GraphicsSubmission, preserves_equal_order) {
  U8* root = SubmissionNode::create();
  U8* first = SubmissionNode::create();
  U8* second = SubmissionNode::create();
  SubmissionNode::get(first).draw_count = 1;
  SubmissionNode::get(first).z_index = 3;
  SubmissionNode::get(first).inputs = "first"_view;
  SubmissionNode::get(second).draw_count = 1;
  SubmissionNode::get(second).z_index = 3;
  SubmissionNode::get(second).inputs = "second"_view;
  ASSERT(SubmissionNode::get(root).add_child(first));
  ASSERT(SubmissionNode::get(root).add_child(second));

  auto submission = submit(root);
  ASSERT(submission);
  auto batches = submission->get_batches();
  ASSERT_EQ(batches.get_size(), Count(2));
  EXPECT_EQ(batches.get_data()[0].get_inputs(), "first"_view);
  EXPECT_EQ(batches.get_data()[1].get_inputs(), "second"_view);
  EXPECT_EQ(batches.get_data()[0].get_authored_order(), Count(0));
  EXPECT_EQ(batches.get_data()[1].get_authored_order(), Count(1));
  perimortem_core_object_release(first);
  perimortem_core_object_release(second);
  perimortem_core_object_release(root);
}

PERIMORTEM_UNIT_TEST(GraphicsSubmission, rejects_visible_cycle) {
  U8* root = SubmissionNode::create();
  SubmissionNode& node = SubmissionNode::get(root);
  ASSERT(node.add_child(root));
  EXPECT_NOT(submit(root));
  node.visible = False;
  auto hidden = submit(root);
  ASSERT(hidden);
  EXPECT(hidden->get_batches().is_empty());
  node.clear_children();
  perimortem_core_object_release(root);
}

PERIMORTEM_UNIT_TEST(GraphicsSubmission, scales_draw_order) {
  constexpr Count draw_count = 100000;
  U8* root = SubmissionNode::create();
  U8* child = SubmissionNode::create();
  SubmissionNode::get(child).draw_count = draw_count;
  SubmissionNode::get(child).z_index = S64(draw_count);
  ASSERT(SubmissionNode::get(root).add_child(child));
  auto submission = submit(root);
  ASSERT(submission);
  auto batches = submission->get_batches();
  ASSERT_EQ(batches.get_size(), draw_count);
  EXPECT_EQ(batches.get_data()[0].get_z_index(), S64(1));
  EXPECT_EQ(batches.get_data()[draw_count - 1].get_z_index(), S64(draw_count));
  perimortem_core_object_release(child);
  perimortem_core_object_release(root);
}
