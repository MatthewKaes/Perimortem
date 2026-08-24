// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/object.hpp"
#include "perimortem/core/option.hpp"

#include "perimortem/memory/dynamic/vector.hpp"

#include "perimortem/graphics/frame/batch.hpp"
#include "tetrodotoxin/graphics/descriptor.hpp"

namespace Tetrodotoxin::Graphics {

// Submission collects one hosted Object tree into a stable draw sequence. The
// sequence is ordered back to front by z index and preserves authored traversal
// order for equal indices.
class Submission {
 public:
  static auto create(
      Perimortem::Core::Object<> root,
      const Descriptor& descriptor) -> Perimortem::Core::Option<Submission>;

  constexpr auto get_batches() const
      -> Perimortem::Core::View::Vector<Perimortem::Graphics::Frame::Batch> {
    return batches.get_view();
  }

 private:
  explicit Submission(
      Perimortem::Memory::Dynamic::Vector<Perimortem::Graphics::Frame::Batch>&&
          batches)
      : batches(
            static_cast<Perimortem::Memory::Dynamic::Vector<
                Perimortem::Graphics::Frame::Batch>&&>(batches)) {}

  Perimortem::Memory::Dynamic::Vector<Perimortem::Graphics::Frame::Batch>
      batches;
};

}  // namespace Tetrodotoxin::Graphics
