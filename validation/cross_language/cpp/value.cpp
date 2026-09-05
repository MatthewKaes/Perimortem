// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "cross_language/cpp/value.hpp"

#include <algorithm>
#include <cstdlib>
#include <mutex>

class CppBytes final : public TtxTest::AbstractModel {
 public:
  CppBytes(ttx_abstract view_bytes, std::shared_ptr<TtxTest::Route> bytes)
      : view_bytes(view_bytes), payload(std::move(bytes)) {}

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
          .relation = (ttx_abstract_same(requirement, view_bytes) ||
                       ttx_abstract_same(requirement, ttx_bytes_requirement()))
                          ? TTX_INTERFACE_SATISFIED
                          : TTX_INTERFACE_REJECTED,
          .invoke = {},
        },
        result);
  }

  void domain(ttx_abstract, ttx_domain_result result) const override {
    result.operations->unknown(result);
  }

  void bytes(ttx_abstract, ttx_bytes_result result) const override {
    static const ttx_bytes_ops ops = {
      .header = {sizeof(ttx_bytes_ops), TTX_ABI_MAJOR, TTX_ABI_MINOR},
      .candidate =
          [](ttx_bytes v) {
            return reinterpret_cast<const CppBytes*>(v.self)->get_abi();
          },
      .size = [](ttx_bytes v) -> uint64_t {
        return reinterpret_cast<const CppBytes*>(v.self)->payload->size();
      },
      .visit =
          [](ttx_bytes v, ttx_bytes_sink sink) {
            const auto& data =
                *reinterpret_cast<const CppBytes*>(v.self)->payload;
            sink.operations->bytes(sink, {data.data(), data.size()});
            sink.operations->completed(sink);
          },
    };
    result.operations->resolved(
        result, {.operations = &ops,
                 .self = reinterpret_cast<ttx_bytes_self*>(
                     const_cast<CppBytes*>(this))});
  }

 private:
  ttx_abstract view_bytes;
  std::shared_ptr<TtxTest::Route> payload;
};

class CppValue final : public TtxTest::AbstractModel {
 public:
  CppValue(
      ttx_abstract value,
      ttx_abstract view_bytes,
      ttx_abstract to_string,
      ttx_abstract text,
      std::shared_ptr<TtxTest::Route> payload)
      : value(value),
        view_bytes(view_bytes),
        to_string(to_string),
        text(text),
        payload(std::move(payload)) {}

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
    if (ttx_abstract_same(requirement, view_bytes) ||
        ttx_abstract_same(requirement, ttx_bytes_requirement())) {
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

  void bytes(ttx_abstract, ttx_bytes_result result) const override {
    static const ttx_bytes_ops ops = {
      .header = {sizeof(ttx_bytes_ops), TTX_ABI_MAJOR, TTX_ABI_MINOR},
      .candidate =
          [](ttx_bytes v) {
            return reinterpret_cast<const CppValue*>(v.self)->get_abi();
          },
      .size = [](ttx_bytes v) -> uint64_t {
        return reinterpret_cast<const CppValue*>(v.self)->payload->size();
      },
      .visit =
          [](ttx_bytes v, ttx_bytes_sink sink) {
            const auto& data =
                *reinterpret_cast<const CppValue*>(v.self)->payload;
            sink.operations->bytes(sink, {data.data(), data.size()});
            sink.operations->completed(sink);
          },
    };
    result.operations->resolved(
        result, {.operations = &ops,
                 .self = reinterpret_cast<ttx_bytes_self*>(
                     const_cast<CppValue*>(this))});
  }

 private:
  ttx_abstract value;
  ttx_abstract view_bytes;
  ttx_abstract to_string;
  ttx_abstract text;
  std::shared_ptr<TtxTest::Route> payload;
};

auto TtxTest::create_value(
    ttx_abstract value,
    ttx_abstract view_bytes,
    ttx_abstract to_string,
    Route bytes) -> ttx_abstract {
  auto retained = std::make_shared<Route>(std::move(bytes));
  const ttx_abstract text =
      retain_abstract(std::make_shared<CppBytes>(view_bytes, retained));
  const ttx_abstract result = retain_abstract(
      std::make_shared<CppValue>(value, view_bytes, to_string, text, retained));
  return result;
}
