// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/object.hpp"

namespace Perimortem::Graphics::Frame {

// Resource keeps one worker local runtime Object alive for the immutable frame
// that references it. The concrete Object descriptor continues to own the
// resource's payload and destruction behavior.
class Resource {
 public:
  Resource() = default;
  explicit Resource(Perimortem::Core::Object<> object);
  Resource(const Resource& source);
  Resource(Resource&& source);
  ~Resource();

  auto operator=(const Resource& source) -> Resource&;
  auto operator=(Resource&& source) -> Resource&;

  constexpr auto is_empty() const -> Bool { return object.is_empty(); }
  constexpr auto get_payload() const -> const U8* {
    return object.get_payload();
  }
  auto get_reservations() const -> Count;

 private:
  Perimortem::Core::Object<> object;
};

}  // namespace Perimortem::Graphics::Frame
