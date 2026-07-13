// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

namespace Tetrodotoxin::Archiver {

class Terminal {
 public:
  Terminal() = default;
  Terminal(
      Perimortem::Core::View::Bytes group,
      Perimortem::Core::View::Bytes path,
      Perimortem::Core::View::Bytes content)
      : group(group), path(path), content(content) {}

  constexpr auto get_group() const -> Perimortem::Core::View::Bytes {
    return group;
  }

  constexpr auto get_path() const -> Perimortem::Core::View::Bytes {
    return path;
  }

  constexpr auto get_content() const -> Perimortem::Core::View::Bytes {
    return content;
  }

 private:
  Perimortem::Core::View::Bytes group;
  Perimortem::Core::View::Bytes path;
  Perimortem::Core::View::Bytes content;
};

}  // namespace Tetrodotoxin::Archiver
