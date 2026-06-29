// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"

#include "tetrodotoxin/syntax/type/ref.hpp"
#include "tetrodotoxin/ttx/facts.hpp"

namespace Tetrodotoxin::Ttx {

class StandardPackage {
 public:
  using IsType = auto (*)(Perimortem::Core::View::Bytes) -> Bool;
  using FindType = auto (*)(Perimortem::Core::View::Bytes) -> const Type*;
  using TypeLayout = auto (*)(const Syntax::Type::Ref&) -> Layout;

  constexpr StandardPackage() = default;
  constexpr StandardPackage(
      Perimortem::Core::View::Bytes name,
      Bool valid,
      Bool implicit,
      IsType is_type_fn,
      FindType find_type_fn,
      TypeLayout type_layout_fn)
      : name(name),
        valid(valid),
        implicit(implicit),
        is_type_fn(is_type_fn),
        find_type_fn(find_type_fn),
        type_layout_fn(type_layout_fn) {}

  constexpr auto exists() const -> Bool { return valid; }
  constexpr auto is_implicit() const -> Bool { return implicit; }
  constexpr auto get_name() const -> Perimortem::Core::View::Bytes {
    return name;
  }
  constexpr auto as_implicit() const -> StandardPackage {
    return {
      name, valid, True, is_type_fn, find_type_fn, type_layout_fn,
    };
  }
  auto is_type(Perimortem::Core::View::Bytes type_name) const -> Bool;
  auto find_type(Perimortem::Core::View::Bytes type_name) const -> const Type*;
  auto type_class(Perimortem::Core::View::Bytes type_name) const -> Type::Class;
  auto find_field(
      Perimortem::Core::View::Bytes type_name,
      Perimortem::Core::View::Bytes field_name) const -> const Field*;
  auto find_method(
      Perimortem::Core::View::Bytes receiver_type,
      Perimortem::Core::View::Bytes method_name) const -> const Method*;
  auto type_layout(const Syntax::Type::Ref& type) const -> Layout;

 private:
  Perimortem::Core::View::Bytes name;
  Bool valid = False;
  Bool implicit = False;
  IsType is_type_fn = nullptr;
  FindType find_type_fn = nullptr;
  TypeLayout type_layout_fn = nullptr;
};

auto implicit_core_package() -> StandardPackage;
auto find_standard_package(Perimortem::Core::View::Bytes name)
    -> StandardPackage;
auto find_standard_package(
    Perimortem::Core::View::Vector<Perimortem::Core::View::Bytes> path)
    -> StandardPackage;
auto is_standard_type(Perimortem::Core::View::Bytes type_name) -> Bool;

}  // namespace Tetrodotoxin::Ttx
