// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/utility/result.hpp"

#include "ttx/bootstrap/concept/abstract.hpp"
#include "ttx/concept/layout.h"

namespace Ttx::Concept {

// Layout describes one target neutral semantic shape. Concrete Layouts own
// composition and fitting while every entry continues to name its real graph
// producer.
class Layout {
 public:
  constexpr Layout() : abi{&abi_operations} {}
  enum class Errors : U8 {
    IndexOutOfBounds,
    SizeMismatch,
    IncompatibleFit,
  };

  constexpr virtual ~Layout() = default;

  virtual constexpr auto get_size() const -> Count = 0;
  virtual constexpr auto get_abstract(Count index) const
      -> Perimortem::Core::Option<const Abstract&> = 0;
  virtual constexpr auto get_name(Count index) const
      -> Perimortem::Core::Option<Perimortem::Core::View::Bytes> {
    return {};
  }
  virtual constexpr auto fits_entry(
      const Layout& target,
      Count source_index,
      Count target_index) const -> Bool = 0;
  constexpr auto fits(const Layout& target) const -> Bool {
    return get_size() == target.get_size() && fits_at(target, 0);
  }
  virtual constexpr auto fits_at(const Layout& target, Count target_offset)
      const -> Bool = 0;
  constexpr auto get_fitted(const Layout& target, Count target_index) const
      -> Perimortem::Utility::Result<const Abstract&, Errors> {
    if (target_index >= get_size()) {
      return Errors::IndexOutOfBounds;
    }
    if (get_size() != target.get_size()) {
      return Errors::SizeMismatch;
    }
    return get_fitted_at(target, 0, target_index);
  }
  virtual constexpr auto get_fitted_at(
      const Layout& target,
      Count target_offset,
      Count target_index) const
      -> Perimortem::Utility::Result<const Abstract&, Errors> = 0;

  constexpr auto is_empty() const -> Bool { return get_size() == 0; }

  constexpr auto get_abi() const -> const ttx_layout* { return &abi; }
  static auto from_abi(const ttx_layout* layout) -> const Layout&;

  virtual auto negotiate_interface(
      const ttx_layout_interface_requirement* requirement) const
      -> ttx_layout_interface {
    return ttx_layout_interface_rejected(get_abi(), requirement);
  }

 protected:
  constexpr auto has_target_segment(const Layout& target, Count target_offset)
      const -> Bool {
    return target_offset <= target.get_size() &&
           get_size() <= target.get_size() - target_offset;
  }

 private:
  static auto visit_abi(
      const ttx_layout* layout,
      ttx_abstract_callable* visitor) -> void;
  static auto fits_abi(const ttx_layout* source, const ttx_layout* target)
      -> perimortem_bool;
  static auto interface_abi(
      const ttx_layout* layout,
      const ttx_layout_interface_requirement* requirement)
      -> ttx_layout_interface;
  static const ttx_layout_operations abi_operations;

  ttx_layout abi;
};

}  // namespace Ttx::Concept
