// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/graphics/submission.hpp"

#include "perimortem/core/algorithm/sort.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin;

class SubmissionBuilder {
 public:
  auto collect(Object<> root, const Graphics::Descriptor& descriptor) -> Bool {
    return collect(
        root, descriptor, Perimortem::Graphics::Frame::Transform(), 0);
  }

  auto finish() -> Dynamic::Vector<Perimortem::Graphics::Frame::Batch> {
    Algorithm::sort(batches.get_access());
    return static_cast<Dynamic::Vector<Perimortem::Graphics::Frame::Batch>&&>(
        batches);
  }

 private:
  auto collect(
      Object<> object,
      const Graphics::Descriptor& descriptor,
      const Perimortem::Graphics::Frame::Transform& parent,
      S64 parent_z_index) -> Bool {
    BAIL_IF(object.is_empty() || path.contains(object.get_payload()));
    Perimortem::Graphics::Frame::Resource reservation(object);
    auto placement = descriptor.placement(object.get_payload());
    BAIL_IF(!placement);
    if (!placement->is_visible()) {
      return True;
    }

    Perimortem::Graphics::Frame::Transform transform =
        Perimortem::Graphics::Frame::Transform::compose(
            parent, placement->get_transform());
    S64 z_index = 0;
    BAIL_IF(__builtin_add_overflow(
        parent_z_index, placement->get_z_index(), &z_index));
    path.insert(object.get_payload());
    Bool complete = collect_draws(object, descriptor, transform, z_index) &&
                    collect_children(object, descriptor, transform, z_index);
    path.remove(path.get_size() - 1);
    return complete;
  }

  auto collect_draws(
      Object<> object,
      const Graphics::Descriptor& descriptor,
      const Perimortem::Graphics::Frame::Transform& transform,
      S64 host_z_index) -> Bool {
    Count count = descriptor.draw_count(object.get_payload());
    for (Count index = 0; index < count; index++) {
      auto draw = descriptor.draw(object.get_payload(), index);
      BAIL_IF(!draw);
      S64 z_index = 0;
      BAIL_IF(
          __builtin_add_overflow(host_z_index, draw->get_z_offset(), &z_index));
      batches.emplace(
          Perimortem::Graphics::Frame::Batch(
              draw->get_program(), draw->get_resources(), draw->get_inputs(),
              transform, draw->get_vertex_count(), z_index, order++));
    }
    return True;
  }

  auto collect_children(
      Object<> object,
      const Graphics::Descriptor& descriptor,
      const Perimortem::Graphics::Frame::Transform& transform,
      S64 z_index) -> Bool {
    Count count = descriptor.child_count(object.get_payload());
    for (Count index = 0; index < count; index++) {
      auto child = descriptor.child(object.get_payload(), index);
      BAIL_IF(
          !child || !collect(
                        child->get_object(), *child->get_descriptor(),
                        transform, z_index));
    }
    return True;
  }

  Dynamic::Vector<Perimortem::Graphics::Frame::Batch> batches;
  Dynamic::Vector<const U8*> path;
  Count order = 0;
};

auto Graphics::Submission::create(Object<> root, const Descriptor& descriptor)
    -> Option<Submission> {
  SubmissionBuilder builder;
  BAIL_IF(!builder.collect(root, descriptor));
  return Submission(builder.finish());
}
