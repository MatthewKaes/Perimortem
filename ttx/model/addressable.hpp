// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include <memory>
#include <vector>

#include "ttx/concept/abstract.hpp"
#include "ttx/query.hpp"

namespace Ttx {

// An Alias disappears into its referent, which is useful until one use needs
// to reveal less or answer an additional question. Addressable preserves that
// indirection while keeping a visible subject in front of it. Each local Layer
// gets the first chance to answer, and the referent is reached only when every
// layer explicitly passes. This lets a language add visibility, writability,
// receiver, or other policy without moving those decisions into Abstract.
//
// Every policy crosses the typed ttx_addressable_policy carrier, so the chain
// can combine owners written in C++, C, Rust, or another host. Layer is only
// the C++ authoring helper for that carrier. Callers still observe one
// Addressable identity and cannot inspect which language implemented a policy.
class Addressable final : public Abstract {
 public:
  template <typename Answer>
  struct LayerAnswer {
    bool stops;
    Answer answer;

    static auto pass() -> LayerAnswer { return {.stops = false, .answer = {}}; }
    static auto stop(Answer answer) -> LayerAnswer {
      return {.stops = true, .answer = answer};
    }
  };

  enum class InterfaceAnswer {
    Pass,
    Unknown,
    Rejected,
    Satisfied,
    Equivalent,
  };

  class Layer {
   public:
    Layer();
    virtual ~Layer() = default;

    Layer(const Layer&) = delete;
    Layer(Layer&&) = delete;
    auto operator=(const Layer&) -> Layer& = delete;
    auto operator=(Layer&&) -> Layer& = delete;

    auto get_abi() const -> ttx_addressable_policy;

    virtual auto resolve_concept(
        ttx_abstract candidate,
        ttx_borrowed_bytes route) const -> ttx_abstract;
    virtual void visit_concepts(ttx_abstract candidate, ttx_concept_sink result)
        const;
    virtual void domain(ttx_abstract candidate, ttx_domain_result result) const;
    virtual auto callable(ttx_abstract candidate) const
        -> LayerAnswer<CallableObservation>;
    virtual auto route(ttx_abstract candidate) const
        -> LayerAnswer<RouteObservation>;
    virtual auto extent(ttx_abstract candidate) const
        -> LayerAnswer<ExtentObservation>;
    virtual auto bytes(ttx_abstract candidate) const
        -> LayerAnswer<BytesObservation>;
    virtual auto interface(ttx_abstract candidate, ttx_abstract requirement)
        const -> InterfaceAnswer;
    virtual void invoke(
        ttx_abstract candidate,
        ttx_abstract requirement,
        ttx_abstract operation,
        ttx_pack input,
        ttx_context context,
        ttx_pack_result result) const;

   private:
    static const ttx_addressable_policy_ops operations;

    static auto select(ttx_addressable_policy self) -> const Layer&;
    static void TTX_CALL resolve_concept_abi(
        ttx_addressable_policy self,
        ttx_abstract candidate,
        ttx_borrowed_bytes route,
        ttx_abstract_sink result);
    static void TTX_CALL visit_concepts_abi(
        ttx_addressable_policy self,
        ttx_abstract candidate,
        ttx_concept_sink result);
    static void TTX_CALL interface_abi(
        ttx_addressable_policy self,
        ttx_abstract candidate,
        ttx_abstract requirement,
        ttx_addressable_interface_result result);
    static void TTX_CALL domain_abi(
        ttx_addressable_policy self,
        ttx_abstract candidate,
        ttx_domain_result result);
    static void TTX_CALL callable_abi(
        ttx_addressable_policy self,
        ttx_abstract candidate,
        ttx_callable_result result);
    static void TTX_CALL route_abi(
        ttx_addressable_policy self,
        ttx_abstract candidate,
        ttx_route_result result);
    static void TTX_CALL extent_abi(
        ttx_addressable_policy self,
        ttx_abstract candidate,
        ttx_finite_extent_result result);
    static void TTX_CALL bytes_abi(
        ttx_addressable_policy self,
        ttx_abstract candidate,
        ttx_bytes_result result);
    static void TTX_CALL invoke_abi(
        ttx_addressable_policy self,
        ttx_abstract candidate,
        ttx_abstract requirement,
        ttx_abstract operation,
        ttx_pack input,
        ttx_context context,
        ttx_pack_result result);
  };

  Addressable(
      ttx_abstract referent,
      std::vector<std::shared_ptr<const Layer>> layers = {});
  Addressable(
      ttx_abstract referent,
      std::vector<ttx_addressable_policy> policies);

  auto name() const -> ttx_borrowed_bytes override;
  auto documentation(ttx_abstract self) const -> ttx_documentation override;
  auto resolve(ttx_abstract self) const -> ttx_abstract override;
  auto resolve_concept(ttx_borrowed_bytes route) const -> ttx_abstract override;
  void visit_concepts(ttx_concept_sink result) const override;
  void interface(
      ttx_abstract self,
      ttx_abstract requirement,
      ttx_interface_sink result) const override;
  void domain(ttx_abstract self, ttx_domain_result result) const override;
  void callable(ttx_abstract self, ttx_callable_result result) const override;
  void route(ttx_abstract self, ttx_route_result result) const override;
  void finite_extent(ttx_abstract self, ttx_finite_extent_result result)
      const override;
  void bytes(ttx_abstract self, ttx_bytes_result result) const override;

 private:
  const ttx_abstract referent;
  const std::vector<std::shared_ptr<const Layer>> retained_layers;
  const std::vector<ttx_addressable_policy> policies;
};

}  // namespace Ttx
