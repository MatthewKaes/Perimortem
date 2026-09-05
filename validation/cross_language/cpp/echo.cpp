// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "cross_language/cpp/echo.hpp"

#include <cstdio>
#include <cstring>

#include "ttx/query.hpp"

class CppEchoSimulacrum final : public TtxTest::AbstractModel {
 public:
  CppEchoSimulacrum(
      ttx_abstract echo,
      ttx_abstract operation,
      TtxTest::AliasBinding alias)
      : echo(echo), operation(operation), alias(std::move(alias)) {}

  auto name() const -> ttx_borrowed_bytes override {
    static const uint8_t value[] = "C++ Echo simulacrum";
    return {value, sizeof(value) - 1};
  }

  auto resolve_concept(ttx_borrowed_bytes route) const
      -> ttx_abstract override {
    static const uint8_t echo_route[] = "echo";
    if (route.data != nullptr && route.size == sizeof(echo_route) - 1 &&
        std::memcmp(route.data, echo_route, sizeof(echo_route) - 1) == 0) {
      return TtxTest::alias_target(alias);
    }
    return TtxTest::resolve_concept(alias.abstract, route);
  }

  void visit_concepts(ttx_concept_sink result) const override {
    static const uint8_t echo_route[] = "echo";
    result.operations->item(
        result, {echo_route, sizeof(echo_route) - 1},
        TtxTest::alias_target(alias));
    alias.abstract->operations->visit_concepts(alias.abstract, result);
  }

  void bytes(ttx_abstract self, ttx_bytes_result result) const override {
    TtxTest::forward_bytes(TtxTest::alias_target(alias), self, result);
  }

  void interface(
      ttx_abstract self,
      ttx_abstract requirement,
      ttx_interface_sink result) const override {
    if (!ttx_abstract_same(requirement, echo)) {
      TtxTest::forward_interface(alias.abstract, self, requirement, result);
      return;
    }
    const ttx_abstract selected = TtxTest::alias_target(alias);
    const ttx_abstract selected_echo = echo;
    const ttx_abstract intercepted_operation = operation;
    TtxTest::answer_interface(
        {
          .requirement = requirement,
          .candidate = self,
          .relation = TTX_INTERFACE_SATISFIED,
          .invoke =
              [selected, selected_echo, intercepted_operation](
                  ttx_abstract invoked, ttx_pack input, ttx_context context,
                  ttx_pack_result output) {
                const auto admitted =
                    Ttx::fit(ttx_empty_layout(), input, context);
                if (admitted.state != Ttx::PackObservationState::Packed) {
                  if (admitted.state == Ttx::PackObservationState::Unknown) {
                    output.operations->unknown(output);
                  } else if (
                      admitted.state == Ttx::PackObservationState::None) {
                    output.operations->none(output);
                  } else {
                    output.operations->support_failed(output, admitted.failure);
                  }
                  return;
                }
                const bool intercepted =
                    ttx_abstract_same(invoked, intercepted_operation);
                if (intercepted) {
                  static constexpr char prefix[] = "C++ Simulacra layering: ";
                  std::fwrite(prefix, 1, sizeof(prefix) - 1, stdout);
                  std::fflush(stdout);
                }
                if (ttx_abstract_same(selected, ttx_unknown())) {
                  if (intercepted) {
                    static constexpr char unresolved[] =
                        "C++ Simulacra points to Unknown\n";
                    std::fwrite(unresolved, 1, sizeof(unresolved) - 1, stdout);
                    std::fflush(stdout);
                    context.operations->pack(
                        context, ttx_empty_layout(), output);
                  } else {
                    output.operations->none(output);
                  }
                  return;
                }
                TtxTest::invoke(
                    selected, selected_echo, invoked, input, context, output);
              },
        },
        result);
  }

 private:
  ttx_abstract echo;
  ttx_abstract operation;

  // The shared pointer is a private C++ ownership choice. The Alias itself is
  // still a transparent Abstract and no caller can inspect this representation.
  TtxTest::AliasBinding alias;
};

auto TtxTest::create_echo_simulacrum(
    ttx_abstract echo,
    ttx_abstract operation,
    ttx_abstract child) -> ttx_abstract {
  const ttx_abstract selected = spot_resolve(child);
  if (ttx_abstract_same(selected, ttx_none())) {
    return {};
  }
  if (!ttx_abstract_same(selected, ttx_unknown())) {
    const ttx_interface_relation proof = relation(selected, echo);
    if (proof != TTX_INTERFACE_SATISFIED && proof != TTX_INTERFACE_EQUIVALENT) {
      return {};
    }
  }
  AliasBinding alias = make_alias(selected);
  if (alias.abstract == nullptr || alias.abstract->operations == nullptr) {
    return {};
  }
  return retain_abstract(
      std::make_shared<CppEchoSimulacrum>(echo, operation, std::move(alias)));
}
