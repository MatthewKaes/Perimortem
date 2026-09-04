// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "cross_language/cpp/value.hpp"

#include <algorithm>
#include <cstdlib>
#include <mutex>

struct CppBytesProjection {
  ttx_abstract candidate = {};
  ttx_abstract requirement = {};
  std::shared_ptr<TtxTest::Route> bytes;
};

static std::mutex cpp_bytes_mutex;
static std::vector<CppBytesProjection> cpp_bytes_projections;
static uint64_t cpp_bytes_provider_authority;

class CppBytes final : public TtxTest::AbstractModel {
 public:
  CppBytes(ttx_abstract view_bytes, std::shared_ptr<TtxTest::Route> bytes)
      : view_bytes(view_bytes), bytes(std::move(bytes)) {}

  auto name() const -> ttx_borrowed_bytes override {
    static const uint8_t value[] = "C++ string representation";
    return {value, sizeof(value) - 1};
  }

  void interface(
      ttx_abstract self,
      ttx_abstract requirement,
      ttx_interface_sink result) const override {
    TtxTest::answer_interface(
        {
          .requirement = requirement,
          .candidate = self,
          .relation = ttx_abstract_same(requirement, view_bytes)
                          ? TTX_INTERFACE_SATISFIED
                          : TTX_INTERFACE_REJECTED,
          .invoke = {},
        },
        result);
  }

  void domain(ttx_abstract, ttx_domain_result result) const override {
    result.operations->unknown(result);
  }

 private:
  ttx_abstract view_bytes;
  std::shared_ptr<TtxTest::Route> bytes;
};

class CppValue final : public TtxTest::AbstractModel {
 public:
  CppValue(
      ttx_abstract value,
      ttx_abstract view_bytes,
      ttx_abstract to_string,
      ttx_abstract text)
      : value(value),
        view_bytes(view_bytes),
        to_string(to_string),
        text(text) {}

  auto name() const -> ttx_borrowed_bytes override {
    static const uint8_t value[] = "C++ Value";
    return {value, sizeof(value) - 1};
  }

  auto concepts() const -> TtxTest::Routes override {
    return {{TtxTest::Route{'t', 'e', 'x', 't'}, text}};
  }

  void domain(ttx_abstract, ttx_domain_result result) const override {
    result.operations->unknown(result);
  }

  void interface(
      ttx_abstract self,
      ttx_abstract requirement,
      ttx_interface_sink result) const override {
    if (ttx_abstract_same(requirement, view_bytes)) {
      TtxTest::answer_interface(
          {
            .requirement = requirement,
            .candidate = self,
            .relation = TTX_INTERFACE_SATISFIED,
            .invoke = {},
          },
          result);
      return;
    }
    const ttx_abstract selected_operation = to_string;
    const ttx_abstract selected_text = text;
    TtxTest::answer_interface(
        {
          .requirement = requirement,
          .candidate = self,
          .relation = ttx_abstract_same(requirement, value)
                          ? TTX_INTERFACE_SATISFIED
                          : TTX_INTERFACE_REJECTED,
          .invoke =
              [selected_operation, selected_text](
                  ttx_abstract operation, ttx_pack input, ttx_context context,
                  ttx_pack_result output) {
                const std::optional<uint64_t> cardinality =
                    TtxTest::pack_cardinality(input);
                if (!ttx_abstract_same(operation, selected_operation) ||
                    !cardinality.has_value() || cardinality.value() != 0) {
                  output.operations->none(output);
                  return;
                }
                TtxTest::return_pack(
                    context, {{TtxTest::Route{0}, selected_text}}, output);
              },
        },
        result);
  }

 private:
  ttx_abstract value;
  ttx_abstract view_bytes;
  ttx_abstract to_string;
  ttx_abstract text;
};

static void TTX_CALL cpp_bytes_project(
    ttx_test_bytes_terminal,
    ttx_abstract producer,
    ttx_abstract requirement,
    ttx_test_bytes_sink result) {
  std::lock_guard lock(cpp_bytes_mutex);
  const auto found = std::find_if(
      cpp_bytes_projections.begin(), cpp_bytes_projections.end(),
      [producer, requirement](const CppBytesProjection& projection) {
        return ttx_abstract_same(projection.candidate, producer) &&
               ttx_abstract_same(projection.requirement, requirement);
      });
  if (found == cpp_bytes_projections.end()) {
    result.operations->rejected(result);
    return;
  }
  result.operations->projected(
      result, {
                .data = found->bytes->data(),
                .size = found->bytes->size(),
              });
}

static const ttx_test_bytes_terminal_ops cpp_bytes_provider_operations = {
  .header =
      {
        .size = sizeof(ttx_test_bytes_terminal_ops),
        .abi_major = TTX_ABI_MAJOR,
        .abi_minor = TTX_ABI_MINOR,
      },
  .project = cpp_bytes_project,
};

auto TtxTest::create_value(
    ttx_abstract value,
    ttx_abstract view_bytes,
    ttx_abstract to_string,
    Route bytes) -> ttx_abstract {
  auto retained = std::make_shared<Route>(std::move(bytes));
  const ttx_abstract text =
      register_abstract(std::make_shared<CppBytes>(view_bytes, retained));
  const ttx_abstract result = register_abstract(
      std::make_shared<CppValue>(value, view_bytes, to_string, text));
  std::lock_guard lock(cpp_bytes_mutex);
  cpp_bytes_projections.push_back(
      {.candidate = text, .requirement = view_bytes, .bytes = retained});
  cpp_bytes_projections.push_back(
      {.candidate = result, .requirement = view_bytes, .bytes = retained});
  return result;
}

auto TtxTest::value_bytes_provider() -> ttx_test_bytes_terminal {
  if (cpp_bytes_provider_authority == 0) {
    cpp_bytes_provider_authority = ttx_authority_create();
  }
  return {
    .operations = &cpp_bytes_provider_operations,
    .owner = cpp_bytes_provider_authority,
    .value = 1,
  };
}
