// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/dynamic/bytes.hpp"

#include "perimortem/utility/option.hpp"

namespace Perimortem::System {

// Stateless filesystem transactions.
class File {
 public:
  // Retains one opened directory capability for confined member operations.
  class Root {
   public:
    Root(const Root&) = delete;
    auto operator=(const Root&) -> Root& = delete;
    Root(Root&& source);
    auto operator=(Root&& source) -> Root&;
    ~Root();

    static auto open(Core::View::Bytes location) -> Utility::Option<Root>;
    auto read(Core::View::Bytes relative_path) const
        -> Utility::Option<Memory::Dynamic::Bytes>;
    // Retains successful bytes in the caller Arena.
    auto read(Memory::Allocator::Arena& arena, Core::View::Bytes relative_path)
        const -> Utility::Option<Core::View::Bytes>;
    auto write(Core::View::Bytes data, Core::View::Bytes relative_path) const
        -> Bool;
    auto remove(Core::View::Bytes relative_path) const -> Bool;
    auto exists(Core::View::Bytes relative_path) const -> Bool;

   private:
    Root(Signed_32 descriptor);

    Signed_32 descriptor;
  };

  static auto read(Core::View::Bytes location)
      -> Utility::Option<Memory::Dynamic::Bytes>;
  // Retains successful bytes in the caller Arena.
  static auto read(Memory::Allocator::Arena& arena, Core::View::Bytes location)
      -> Utility::Option<Core::View::Bytes>;
  static auto write(Core::View::Bytes data, Core::View::Bytes location) -> Bool;
  static auto remove(Core::View::Bytes location) -> Bool;
  static auto exists(Core::View::Bytes location) -> Bool;
};

}  // namespace Perimortem::System
