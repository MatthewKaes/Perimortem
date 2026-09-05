// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "ttx/abi.h"

namespace Ttx {

// Interface owns the C++ support for one synchronous negotiation answer.
// The candidate constructs it after receiving an exact requirement, which lets
// that candidate return a witness assembled from local policy, another
// capability, or an entirely synthesized implementation. The witness keeps the
// ordered pair visible while hiding how the relationship was established.
//
// A positive witness may implement operations without changing either
// Abstract. That is the important distinction from casting a candidate to a
// native class: the relationship belongs to this observation and can be
// projected by any language that can honor the same C contract.
class Interface {
 public:
  Interface(
      ttx_abstract requirement,
      ttx_abstract candidate,
      ttx_interface_relation relation);
  virtual ~Interface() = default;

  Interface(const Interface&) = delete;
  Interface(Interface&&) = delete;
  auto operator=(const Interface&) -> Interface& = delete;
  auto operator=(Interface&&) -> Interface& = delete;

  auto get_abi() const -> ttx_interface;
  void publish(ttx_interface_sink result) const;

  auto get_requirement() const -> ttx_abstract { return requirement; }
  auto get_candidate() const -> ttx_abstract { return candidate; }
  auto get_relation() const -> ttx_interface_relation { return relation; }

 protected:
  virtual void invoke(
      ttx_abstract operation,
      ttx_pack input,
      ttx_context context,
      ttx_pack_result result) const;

 private:
  struct Binding final : ttx_interface_capability {
    constexpr explicit Binding(const Interface* owner)
        : ttx_interface_capability{&interface_operations}, owner(owner) {}

    const Interface* owner;
  };

  static const ttx_interface_ops interface_operations;
  static auto select(ttx_interface self) -> const Interface&;
  static auto TTX_CALL get_requirement_abi(ttx_interface self) -> ttx_abstract;
  static auto TTX_CALL get_candidate_abi(ttx_interface self) -> ttx_abstract;
  static auto TTX_CALL get_relation_abi(ttx_interface self)
      -> ttx_interface_relation;
  static void TTX_CALL invoke_abi(
      ttx_interface self,
      ttx_abstract operation,
      ttx_pack input,
      ttx_context context,
      ttx_pack_result result);

  const Binding binding;
  const ttx_abstract requirement;
  const ttx_abstract candidate;
  const ttx_interface_relation relation;
};

}  // namespace Ttx
