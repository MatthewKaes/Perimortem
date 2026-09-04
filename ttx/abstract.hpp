// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "ttx/abi.h"

namespace Ttx {

// Abstract is the modern C++ owner for one canonical TTX identity. The C ABI
// remains the complete observable surface, which lets another language replace
// this owner without reproducing its C++ object model.
//
// Each instance owns its dispatch table beside one private pointer back to the
// C++ object. That pointer participates only in local dispatch. Authority and
// value remain the semantic identity seen by every caller.
class Abstract {
 public:
  Abstract();
  Abstract(uint64_t authority, uint64_t value);
  virtual ~Abstract() = default;

  Abstract(const Abstract&) = delete;
  Abstract(Abstract&&) = delete;
  auto operator=(const Abstract&) -> Abstract& = delete;
  auto operator=(Abstract&&) -> Abstract& = delete;

  auto get_abi() const -> ttx_abstract;

  virtual auto name() const -> ttx_borrowed_bytes = 0;
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

 protected:
  virtual auto negotiate(ttx_abstract requirement) const
      -> ttx_interface_relation;
  virtual void invoke(
      ttx_abstract operation,
      ttx_pack input,
      ttx_context context,
      ttx_pack_result result) const;

 private:
  struct Binding {
    ttx_abstract_ops operations;
    Abstract* owner;
    uint64_t authority;
    uint64_t value;
  };

  struct InterfaceBinding {
    ttx_interface_ops operations;
    const Abstract* owner;
    ttx_abstract requirement;
    ttx_abstract candidate;
    ttx_interface_relation relation;
  };

  static auto select(ttx_abstract self) -> Abstract&;
  static auto select(ttx_interface self) -> const InterfaceBinding&;

  static auto TTX_CALL get_name(ttx_abstract self) -> ttx_borrowed_bytes;
  static auto TTX_CALL get_documentation(ttx_abstract self)
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

  static auto TTX_CALL interface_requirement(ttx_interface self)
      -> ttx_abstract;
  static auto TTX_CALL interface_candidate(ttx_interface self) -> ttx_abstract;
  static auto TTX_CALL interface_negotiate(ttx_interface self)
      -> ttx_interface_relation;
  static void TTX_CALL interface_invoke(
      ttx_interface self,
      ttx_abstract operation,
      ttx_pack input,
      ttx_context context,
      ttx_pack_result result);

  const Binding binding;
};

}  // namespace Ttx
