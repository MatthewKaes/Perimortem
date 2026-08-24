// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/graphics/submission.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/data.hpp"
#include "perimortem/core/object.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"

using namespace Perimortem;
using namespace Perimortem::Graphics::Frame;
using namespace Tetrodotoxin::Graphics;
using namespace Validation;

static Harness GraphicsSubmission = {
  .name = "Tetrodotoxin::Graphics::Submission"_view,
};

extern "C" {
extern const U8 TTX_DATA_Validation_2eShader__Shader__TestShader[];
extern const U8 TTX_DATA_Validation_2eShader__Shader__TestShader_end[];
}

static Count finalized_resources = 0;

class SubmissionResource {
 public:
  static auto create(U8 value) -> Core::Object<>;

 private:
  static auto finalize(U8*) -> void;

  static const Core::Object<>::Descriptor descriptor;
};

const Core::Object<>::Descriptor SubmissionResource::descriptor(
    sizeof(U8),
    alignof(U8),
    SubmissionResource::finalize);

auto SubmissionResource::create(U8 value) -> Core::Object<> {
  Core::Object<> object = Core::Object<>::create(descriptor);
  *object.get_payload() = value;
  return object;
}

auto SubmissionResource::finalize(U8*) -> void {
  finalized_resources++;
}

class SubmissionNode {
 public:
  static auto create() -> Core::Object<>;
  static auto get(Core::Object<> object) -> SubmissionNode&;

  ~SubmissionNode();

  auto add_child(Core::Object<> child) -> Bool;
  auto clear_children() -> void;
  auto set_resource(Core::Object<> selected) -> void;

  Transform transform;
  Bool visible = True;
  Count draw_count = 0;
  S64 z_index = 0;
  Count vertex_count = 3;
  Memory::Dynamic::Bytes inputs;

  static const Descriptor graphics_descriptor;

 private:
  static auto finalize(U8* payload) -> void;
  static auto read_placement(const U8* product, Core::Object<> object)
      -> Descriptor::Placement;
  static auto read_child_count(const U8* product, Core::Object<> object)
      -> Count;
  static auto read_child(const U8* product, Core::Object<> object, Count index)
      -> Descriptor::Child;
  static auto read_draw_count(const U8* product, Core::Object<> object)
      -> Count;
  static auto read_draw(const U8* product, Core::Object<> object, Count index)
      -> Descriptor::Draw;

  static const Core::Object<>::Descriptor object_descriptor;
  Core::Object<> children[4];
  Count child_count = 0;
  Core::Object<> resource;
};

const Core::Object<>::Descriptor SubmissionNode::object_descriptor(
    sizeof(SubmissionNode),
    alignof(SubmissionNode),
    SubmissionNode::finalize);

const Descriptor SubmissionNode::graphics_descriptor(
    TTX_DATA_Validation_2eShader__Shader__TestShader,
    SubmissionNode::read_placement,
    SubmissionNode::read_child_count,
    SubmissionNode::read_child,
    SubmissionNode::read_draw_count,
    SubmissionNode::read_draw);

auto SubmissionNode::create() -> Core::Object<> {
  Core::Object<> object = Core::Object<>::create(object_descriptor);
  new (object.get_payload(), Perimortem::Core::Placement::Construct)
      SubmissionNode();
  return object;
}

auto SubmissionNode::get(Core::Object<> object) -> SubmissionNode& {
  return *Core::Data::cast<SubmissionNode>(object.get_payload());
}

SubmissionNode::~SubmissionNode() {
  clear_children();
  resource.release();
}

auto SubmissionNode::add_child(Core::Object<> child) -> Bool {
  BAIL_IF(child.is_empty() || child_count == 4);
  child.retain();
  children[child_count++] = child;
  return True;
}

auto SubmissionNode::clear_children() -> void {
  for (Count index = 0; index < child_count; index++) {
    children[index].release();
    children[index] = Core::Object<>();
  }
  child_count = 0;
}

auto SubmissionNode::set_resource(Core::Object<> selected) -> void {
  selected.retain();
  resource.release();
  resource = selected;
}

auto SubmissionNode::finalize(U8* payload) -> void {
  Core::Data::cast<SubmissionNode>(payload)->~SubmissionNode();
}

auto SubmissionNode::read_placement(const U8*, Core::Object<> object)
    -> Descriptor::Placement {
  const SubmissionNode& node = get(object);
  return Descriptor::Placement(node.transform, node.visible, node.z_index);
}

auto SubmissionNode::read_child_count(const U8*, Core::Object<> object)
    -> Count {
  return get(object).child_count;
}

auto SubmissionNode::read_child(const U8*, Core::Object<> object, Count index)
    -> Descriptor::Child {
  const SubmissionNode& node = get(object);
  return Descriptor::Child(node.children[index], graphics_descriptor);
}

auto SubmissionNode::read_draw_count(const U8*, Core::Object<> object)
    -> Count {
  return get(object).draw_count;
}

auto SubmissionNode::read_draw(
    const U8* product,
    Core::Object<> object,
    Count index) -> Descriptor::Draw {
  const SubmissionNode& node = get(object);
  Memory::Dynamic::Vector<Resource> resources;
  if (!node.resource.is_empty()) {
    resources.emplace(Resource(node.resource));
  }
  return Descriptor::Draw(
      Program(product),
      static_cast<Memory::Dynamic::Vector<Resource>&&>(resources),
      node.inputs.get_view(), node.vertex_count, -S64(index));
}

PERIMORTEM_UNIT_TEST(GraphicsSubmission, stabilizes_hosted_frame) {
  finalized_resources = 0;
  Core::Object<> resource = SubmissionResource::create(0x5A);
  Core::Object<> root = SubmissionNode::create();
  Core::Object<> back = SubmissionNode::create();
  Core::Object<> front = SubmissionNode::create();
  SubmissionNode& root_node = SubmissionNode::get(root);
  SubmissionNode& back_node = SubmissionNode::get(back);
  SubmissionNode& front_node = SubmissionNode::get(front);

  root_node.transform = Transform::create(10.0, 20.0, 2.0, 3.0, 0.0);
  ASSERT(root_node.add_child(front));
  ASSERT(root_node.add_child(back));

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
    auto submission =
        Submission::create(root, SubmissionNode::graphics_descriptor);
    ASSERT(submission);
    auto batches = submission->get_batches();
    ASSERT_EQ(batches.get_size(), Count(2));
    const Batch& back_batch = batches.get_data()[0];
    const Batch& front_batch = batches.get_data()[1];
    EXPECT_EQ(back_batch.get_inputs(), "back"_view);
    EXPECT_EQ(front_batch.get_inputs(), "front"_view);
    EXPECT_EQ(back_batch.get_z_index(), S64(-2));
    EXPECT_EQ(front_batch.get_z_index(), S64(4));
    EXPECT_EQ(back_batch.get_transform().get_x(), R64(16.0));
    EXPECT_EQ(back_batch.get_transform().get_y(), R64(35.0));
    EXPECT_EQ(front_batch.get_transform().get_x(), R64(12.0));
    EXPECT_EQ(front_batch.get_transform().get_y(), R64(26.0));
    EXPECT_EQ(
        back_batch.get_program().get_locator(),
        TTX_DATA_Validation_2eShader__Shader__TestShader);
    EXPECT_EQ(
        *Core::Data::cast<const U32>(back_batch.get_program().get_locator()),
        U32(0x07230203));

    back_node.inputs = "changed"_view;
    back_node.transform = Transform();
    back_node.set_resource({});
    front_node.set_resource({});
    resource.release();
    resource = Core::Object<>();
    back.release();
    back = Core::Object<>();
    front.release();
    front = Core::Object<>();
    root.release();
    root = Core::Object<>();

    EXPECT_EQ(back_batch.get_inputs(), "back"_view);
    EXPECT_EQ(back_batch.get_transform().get_x(), R64(16.0));
    ASSERT_EQ(back_batch.get_resources().get_size(), Count(1));
    EXPECT_EQ(
        back_batch.get_resources().get_data()[0].get_reservations(), Count(2));
    EXPECT_EQ(finalized_resources, Count(0));
  }

  EXPECT_EQ(finalized_resources, Count(1));
}

PERIMORTEM_UNIT_TEST(GraphicsSubmission, preserves_equal_order) {
  Core::Object<> root = SubmissionNode::create();
  Core::Object<> first = SubmissionNode::create();
  Core::Object<> second = SubmissionNode::create();
  SubmissionNode& first_node = SubmissionNode::get(first);
  SubmissionNode& second_node = SubmissionNode::get(second);
  first_node.draw_count = 1;
  first_node.z_index = 3;
  first_node.inputs = "first"_view;
  second_node.draw_count = 1;
  second_node.z_index = 3;
  second_node.inputs = "second"_view;
  ASSERT(SubmissionNode::get(root).add_child(first));
  ASSERT(SubmissionNode::get(root).add_child(second));

  auto submission =
      Submission::create(root, SubmissionNode::graphics_descriptor);
  ASSERT(submission);
  auto batches = submission->get_batches();
  ASSERT_EQ(batches.get_size(), Count(2));
  EXPECT_EQ(batches.get_data()[0].get_inputs(), "first"_view);
  EXPECT_EQ(batches.get_data()[1].get_inputs(), "second"_view);
  EXPECT_EQ(batches.get_data()[0].get_authored_order(), Count(0));
  EXPECT_EQ(batches.get_data()[1].get_authored_order(), Count(1));

  first.release();
  second.release();
  root.release();
}

PERIMORTEM_UNIT_TEST(GraphicsSubmission, rejects_visible_cycle) {
  Core::Object<> root = SubmissionNode::create();
  SubmissionNode& node = SubmissionNode::get(root);
  ASSERT(node.add_child(root));
  EXPECT_NOT(Submission::create(root, SubmissionNode::graphics_descriptor));

  node.visible = False;
  auto hidden = Submission::create(root, SubmissionNode::graphics_descriptor);
  ASSERT(hidden);
  EXPECT(hidden->get_batches().is_empty());

  node.clear_children();
  root.release();
}

PERIMORTEM_UNIT_TEST(GraphicsSubmission, scales_draw_order) {
  constexpr Count draw_count = 100000;
  Core::Object<> root = SubmissionNode::create();
  SubmissionNode& node = SubmissionNode::get(root);
  node.draw_count = draw_count;
  node.z_index = S64(draw_count);

  auto submission =
      Submission::create(root, SubmissionNode::graphics_descriptor);
  ASSERT(submission);
  auto batches = submission->get_batches();
  ASSERT_EQ(batches.get_size(), draw_count);
  EXPECT_EQ(batches.get_data()[0].get_z_index(), S64(1));
  EXPECT_EQ(batches.get_data()[draw_count - 1].get_z_index(), S64(draw_count));

  root.release();
}
