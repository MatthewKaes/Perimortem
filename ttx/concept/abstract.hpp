// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "ttx/abi.h"
#include "ttx/concept/documentation.hpp"

namespace Ttx {

// Abstract gives one C++ capability the canonical TTX operations without
// making its class hierarchy part of the contract. A larger owner may compose
// any number of these capability objects, and each object may retain or
// synthesize state unrelated to the address of that owner. Foreign calls see
// only the C17 capability pointer and its attached operations.
class Abstract {
 public:
  constexpr Abstract() : binding(this) {}
  virtual constexpr ~Abstract() = default;

  Abstract(const Abstract&) = delete;
  Abstract(Abstract&&) = delete;
  auto operator=(const Abstract&) -> Abstract& = delete;
  auto operator=(Abstract&&) -> Abstract& = delete;

  auto get_abi() const -> ttx_abstract;
  auto get_handle() const -> ttx_abstract { return get_abi(); }

  // Bundled C++ owners use these spellings to implement their own capability.
  // Consumers use the C surface and negotiate every relationship instead of
  // asking the C++ hierarchy what an arbitrary candidate might be.
  virtual constexpr auto get_name() const -> Perimortem::Core::View::Bytes {
    return {};
  }
  virtual constexpr auto get_documentation() const
      -> const Concept::Documentation& {
    return Concept::Documentation::get_empty();
  }

  virtual auto name() const -> ttx_borrowed_bytes;
  virtual auto documentation(ttx_abstract self) const -> ttx_documentation;
  virtual auto resolve(ttx_abstract self) const -> ttx_abstract;
  virtual auto resolve_concept(ttx_borrowed_bytes route) const -> ttx_abstract;
  virtual void visit_concepts(ttx_concept_sink result) const;
  virtual void interface(
      ttx_abstract self,
      ttx_abstract requirement,
      ttx_interface_sink result) const;
  virtual void domain(ttx_abstract self, ttx_domain_result result) const;
  virtual void callable(ttx_abstract self, ttx_callable_result result) const;
  virtual void route(ttx_abstract self, ttx_route_result result) const;
  virtual void finite_extent(ttx_abstract self, ttx_finite_extent_result result)
      const;
  virtual void bytes(ttx_abstract self, ttx_bytes_result result) const;

 protected:
  virtual auto negotiate(ttx_abstract requirement) const
      -> ttx_interface_relation;

 private:
  // This binding remembers the C++ implementation pointer supplied at
  // construction. Recovering the binding in its own callback then uses that
  // pointer, including any adjustment the compiler made for a base subobject.
  // A foreign candidate can return a different binding with different state
  // because consumers negotiate its contract before invoking its operations.
  struct AbiBinding final : ttx_abstract_capability {
    constexpr explicit AbiBinding(const Abstract* owner)
        : ttx_abstract_capability{&abstract_operations}, owner(owner) {}

    const Abstract* owner;
  };

  static const ttx_abstract_ops abstract_operations;

  static auto dispatch(ttx_abstract self) -> const Abstract&;

  static auto TTX_CALL get_name_abi(ttx_abstract self) -> ttx_borrowed_bytes;
  static auto TTX_CALL get_documentation_abi(ttx_abstract self)
      -> ttx_documentation;
  static void TTX_CALL resolve_abi(ttx_abstract self, ttx_abstract_sink result);
  static void TTX_CALL resolve_concept_abi(
      ttx_abstract self,
      ttx_borrowed_bytes route,
      ttx_abstract_sink result);
  static void TTX_CALL
      visit_concepts_abi(ttx_abstract self, ttx_concept_sink result);
  static void TTX_CALL interface_abi(
      ttx_abstract self,
      ttx_abstract requirement,
      ttx_interface_sink result);
  static void TTX_CALL
      resolve_domain_abi(ttx_abstract self, ttx_domain_result result);
  static void TTX_CALL
      resolve_callable_abi(ttx_abstract self, ttx_callable_result result);
  static void TTX_CALL
      resolve_route_abi(ttx_abstract self, ttx_route_result result);
  static void TTX_CALL resolve_finite_extent_abi(
      ttx_abstract self,
      ttx_finite_extent_result result);
  static void TTX_CALL
      resolve_bytes_abi(ttx_abstract self, ttx_bytes_result result);

  AbiBinding binding;
};

}  // namespace Ttx

namespace Ttx::Concept {

using Abstract = Ttx::Abstract;

}  // namespace Ttx::Concept

#define TTX_NAME(expression)                                                  \
  constexpr auto get_name() const -> Perimortem::Core::View::Bytes override { \
    return expression;                                                        \
  }
