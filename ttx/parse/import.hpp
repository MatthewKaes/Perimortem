// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "ttx/parse/type_path.hpp"

namespace Ttx::Parse {

class Import {
 public:
  static auto parse(Cursor& cursor) -> Import;

  auto get_local_name() const -> Perimortem::Core::View::Bytes;
  auto get_dialect_name() const -> const TypePath&;
  auto get_package_path() const -> const TypePath&;
  auto get_file_path() const -> Perimortem::Core::View::Bytes;
  auto is_package_source() const -> Bool;
  auto is_file_source() const -> Bool;
  auto is_empty() const -> Bool;

 private:
  static auto inner_text(Perimortem::Core::View::Bytes text)
      -> Perimortem::Core::View::Bytes;

  Perimortem::Core::View::Bytes local_name;
  TypePath dialect_name;
  TypePath package_path;
  Perimortem::Core::View::Bytes file_path;
};

}  // namespace Ttx::Parse
