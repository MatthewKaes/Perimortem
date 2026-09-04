// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "cross_language/cpp/ttx.hpp"

#include <cstdlib>
#include <mutex>

struct CppRuntime {
  uint64_t callback_authority = 0;
  std::vector<std::shared_ptr<TtxTest::AbstractModel>> abstracts;
  std::mutex mutex;
};

struct CppInterfaceFrame {
  struct Binding {
    ttx_interface_ops operations;
    CppInterfaceFrame* frame;
  } binding;
  TtxTest::InterfaceModel model;
};

struct CppForwardFrame {
  ttx_interface_sink_ops operations;
  ttx_abstract source = {};
  ttx_abstract visible_candidate = {};
  ttx_abstract requirement = {};
  ttx_interface_sink result = {};
};

struct CppResolveFrame {
  ttx_abstract_sink_ops operations;
  bool answered = false;
  ttx_abstract answer = {};
};

struct CppRelationFrame {
  ttx_interface_sink_ops operations;
  bool answered = false;
  ttx_abstract requirement = {};
  ttx_abstract candidate = {};
  ttx_interface_relation relation = TTX_INTERFACE_UNKNOWN;
};

struct CppInvokeFrame {
  ttx_interface_sink_ops operations;
  ttx_abstract candidate = {};
  ttx_abstract requirement = {};
  ttx_abstract operation = {};
  ttx_pack input = {};
  ttx_context context = {};
  ttx_pack_result result = {};
};

struct CppPackCapture {
  ttx_pack_result_ops operations;
  bool answered = false;
  ttx_pack pack = {};
};

struct CppEnumerableCapture {
  ttx_enumerable_result_ops operations;
  bool answered = false;
  ttx_enumerable enumerable = {};
};

static CppRuntime cpp_runtime;

static auto cpp_interface_frame(ttx_interface value) -> CppInterfaceFrame& {
  if (value.operations == nullptr) {
    std::abort();
  }
  static_assert(offsetof(CppInterfaceFrame::Binding, operations) == 0);
  auto& binding = *reinterpret_cast<CppInterfaceFrame::Binding*>(
      const_cast<ttx_interface_ops*>(value.operations));
  if (binding.frame == nullptr) {
    std::abort();
  }
  return *binding.frame;
}

static auto TTX_CALL cpp_interface_requirement(ttx_interface value)
    -> ttx_abstract {
  return cpp_interface_frame(value).model.requirement;
}

static auto TTX_CALL cpp_interface_candidate(ttx_interface value)
    -> ttx_abstract {
  return cpp_interface_frame(value).model.candidate;
}

static auto TTX_CALL cpp_interface_negotiate(ttx_interface value)
    -> ttx_interface_relation {
  return cpp_interface_frame(value).model.relation;
}

static void TTX_CALL cpp_interface_invoke(
    ttx_interface value,
    ttx_abstract operation,
    ttx_pack input,
    ttx_context context,
    ttx_pack_result result) {
  const TtxTest::InterfaceModel& model = cpp_interface_frame(value).model;
  if (model.relation != TTX_INTERFACE_SATISFIED &&
      model.relation != TTX_INTERFACE_EQUIVALENT) {
    result.operations->none(result);
  } else if (model.invoke) {
    model.invoke(operation, input, context, result);
  } else {
    result.operations->none(result);
  }
}

static void TTX_CALL
    cpp_forward_answer(ttx_interface_sink value, ttx_interface upstream) {
  if (value.operations == nullptr) {
    std::abort();
  }
  static_assert(offsetof(CppForwardFrame, operations) == 0);
  const auto& frame =
      *reinterpret_cast<const CppForwardFrame*>(value.operations);
  const ttx_abstract requirement = upstream.operations->requirement(upstream);
  const ttx_interface_relation relation =
      upstream.operations->negotiate(upstream);
  TtxTest::answer_interface(
      {
        .requirement = requirement,
        .candidate = frame.visible_candidate,
        .relation = relation,
        .invoke =
            [upstream](
                ttx_abstract operation, ttx_pack input, ttx_context context,
                ttx_pack_result result) {
              upstream.operations->invoke(
                  upstream, operation, input, context, result);
            },
      },
      frame.result);
}

static void TTX_CALL
    cpp_resolve_answer(ttx_abstract_sink value, ttx_abstract answer) {
  if (value.operations == nullptr) {
    std::abort();
  }
  static_assert(offsetof(CppResolveFrame, operations) == 0);
  auto& frame = *reinterpret_cast<CppResolveFrame*>(
      const_cast<ttx_abstract_sink_ops*>(value.operations));
  frame.answered = true;
  frame.answer = answer;
}

static void TTX_CALL
    cpp_relation_answer(ttx_interface_sink value, ttx_interface interface) {
  if (value.operations == nullptr) {
    std::abort();
  }
  static_assert(offsetof(CppRelationFrame, operations) == 0);
  auto& frame = *reinterpret_cast<CppRelationFrame*>(
      const_cast<ttx_interface_sink_ops*>(value.operations));
  frame.answered = true;
  frame.requirement = interface.operations->requirement(interface);
  frame.candidate = interface.operations->candidate(interface);
  frame.relation = interface.operations->negotiate(interface);
}

static void TTX_CALL
    cpp_invoke_answer(ttx_interface_sink value, ttx_interface interface) {
  if (value.operations == nullptr) {
    std::abort();
  }
  static_assert(offsetof(CppInvokeFrame, operations) == 0);
  const auto& frame =
      *reinterpret_cast<const CppInvokeFrame*>(value.operations);
  if (!ttx_abstract_same(
          interface.operations->requirement(interface), frame.requirement) ||
      !ttx_abstract_same(
          interface.operations->candidate(interface), frame.candidate)) {
    frame.result.operations->support_failed(
        frame.result, TTX_PACK_SUPPORT_INVALID_LAYOUT);
    return;
  }
  const ttx_interface_relation relation =
      interface.operations->negotiate(interface);
  if (relation == TTX_INTERFACE_UNKNOWN) {
    frame.result.operations->unknown(frame.result);
  } else if (relation == TTX_INTERFACE_REJECTED) {
    frame.result.operations->none(frame.result);
  } else {
    interface.operations->invoke(
        interface, frame.operation, frame.input, frame.context, frame.result);
  }
}

static auto cpp_pack_capture(ttx_pack_result value) -> CppPackCapture& {
  if (value.operations == nullptr) {
    std::abort();
  }
  static_assert(offsetof(CppPackCapture, operations) == 0);
  return *reinterpret_cast<CppPackCapture*>(
      const_cast<ttx_pack_result_ops*>(value.operations));
}

static void TTX_CALL cpp_pack_unknown(ttx_pack_result value) {
  cpp_pack_capture(value).answered = true;
}

static void TTX_CALL cpp_pack_none(ttx_pack_result value) {
  cpp_pack_capture(value).answered = true;
}

static void TTX_CALL cpp_pack_packed(ttx_pack_result value, ttx_pack pack) {
  CppPackCapture& capture = cpp_pack_capture(value);
  capture.answered = true;
  capture.pack = pack;
}

static void TTX_CALL
    cpp_pack_support_failed(ttx_pack_result value, ttx_pack_support_failure) {
  cpp_pack_capture(value).answered = true;
}

static auto cpp_enumerable_capture(ttx_enumerable_result value)
    -> CppEnumerableCapture& {
  if (value.operations == nullptr) {
    std::abort();
  }
  static_assert(offsetof(CppEnumerableCapture, operations) == 0);
  return *reinterpret_cast<CppEnumerableCapture*>(
      const_cast<ttx_enumerable_result_ops*>(value.operations));
}

static void TTX_CALL cpp_enumerable_rejected(ttx_enumerable_result value) {
  cpp_enumerable_capture(value).answered = true;
}

static void TTX_CALL cpp_enumerable_satisfied(
    ttx_enumerable_result value,
    ttx_enumerable enumerable) {
  CppEnumerableCapture& capture = cpp_enumerable_capture(value);
  capture.answered = true;
  capture.enumerable = enumerable;
}

class CppRedispatch final : public TtxTest::AbstractModel {
 public:
  explicit CppRedispatch(ttx_abstract source)
      : AbstractModel(source.owner, source.value), source(source) {}

  auto name() const -> ttx_borrowed_bytes override {
    return source.operations->name(source);
  }

  auto documentation(ttx_abstract) const -> ttx_documentation override {
    return source.operations->documentation(source);
  }

  auto resolve(ttx_abstract) const -> ttx_abstract override {
    return TtxTest::spot_resolve(source);
  }

  auto resolve_concept(ttx_borrowed_bytes route) const
      -> ttx_abstract override {
    return TtxTest::resolve_concept(source, route);
  }

  void visit_concepts(ttx_concept_sink result) const override {
    source.operations->visit_concepts(source, result);
  }

  void interface(
      ttx_abstract,
      ttx_abstract requirement,
      ttx_interface_sink result) const override {
    source.operations->interface(source, requirement, result);
  }

  void domain(ttx_abstract, ttx_domain_result result) const override {
    source.operations->resolve_domain(source, result);
  }

  void callable(ttx_abstract, ttx_callable_result result) const override {
    source.operations->resolve_callable(source, result);
  }

  void route(ttx_abstract, ttx_route_result result) const override {
    source.operations->resolve_route(source, result);
  }

  void finite_extent(ttx_abstract, ttx_finite_extent_result result)
      const override {
    source.operations->resolve_finite_extent(source, result);
  }

 private:
  ttx_abstract source;
};

auto TtxTest::AbstractModel::concepts() const -> Routes {
  return {};
}

void TtxTest::AbstractModel::visit_concepts(ttx_concept_sink result) const {
  for (const auto& [route, answer] : concepts()) {
    result.operations->item(
        result,
        {
          .data = route.data(),
          .size = route.size(),
        },
        answer);
  }
  result.operations->completed(result);
}

void TtxTest::install(uint64_t callback_authority) {
  if (callback_authority == 0 || cpp_runtime.callback_authority != 0) {
    std::abort();
  }
  cpp_runtime.callback_authority = callback_authority;
}

auto TtxTest::register_abstract(std::shared_ptr<AbstractModel> model)
    -> ttx_abstract {
  std::lock_guard lock(cpp_runtime.mutex);
  cpp_runtime.abstracts.push_back(std::move(model));
  return cpp_runtime.abstracts.back()->get_abi();
}

auto TtxTest::redispatch(ttx_abstract original) -> ttx_abstract {
  return register_abstract(std::make_shared<CppRedispatch>(original));
}

auto TtxTest::make_alias(ttx_abstract target) -> AliasBinding {
  const ttx_abstract selected = spot_resolve(target);
  if (ttx_abstract_same(selected, ttx_none())) {
    return {};
  }
  auto owner = std::make_shared<Ttx::Alias>(selected);
  const ttx_abstract abstract = owner->get_abi();
  return {
    .owner = std::move(owner),
    .abstract = abstract,
    .target = selected,
  };
}

auto TtxTest::alias_target(const AliasBinding& alias) -> ttx_abstract {
  return alias.owner == nullptr ? ttx_unknown() : alias.target;
}

auto TtxTest::spot_resolve(ttx_abstract value) -> ttx_abstract {
  CppResolveFrame capture = {
    .operations =
        {
          .header =
              {
                .size = sizeof(ttx_abstract_sink_ops),
                .abi_major = TTX_ABI_MAJOR,
                .abi_minor = TTX_ABI_MINOR,
              },
          .answer = cpp_resolve_answer,
        },
  };
  const ttx_abstract_sink result = {
    .operations = &capture.operations,
    .owner = cpp_runtime.callback_authority,
    .value = 1,
  };
  value.operations->resolve(value, result);
  return capture.answered ? capture.answer : ttx_unknown();
}

auto TtxTest::resolve_concept(ttx_abstract value, ttx_borrowed_bytes route)
    -> ttx_abstract {
  CppResolveFrame capture = {
    .operations =
        {
          .header =
              {
                .size = sizeof(ttx_abstract_sink_ops),
                .abi_major = TTX_ABI_MAJOR,
                .abi_minor = TTX_ABI_MINOR,
              },
          .answer = cpp_resolve_answer,
        },
  };
  const ttx_abstract_sink result = {
    .operations = &capture.operations,
    .owner = cpp_runtime.callback_authority,
    .value = 1,
  };
  value.operations->resolve_concept(value, route, result);
  return capture.answered ? capture.answer : ttx_unknown();
}

auto TtxTest::relation(ttx_abstract candidate, ttx_abstract requirement)
    -> ttx_interface_relation {
  CppRelationFrame capture = {
    .operations =
        {
          .header =
              {
                .size = sizeof(ttx_interface_sink_ops),
                .abi_major = TTX_ABI_MAJOR,
                .abi_minor = TTX_ABI_MINOR,
              },
          .answer = cpp_relation_answer,
        },
  };
  const ttx_interface_sink result = {
    .operations = &capture.operations,
    .owner = cpp_runtime.callback_authority,
    .value = 1,
  };
  candidate.operations->interface(candidate, requirement, result);
  if (!capture.answered ||
      !ttx_abstract_same(capture.requirement, requirement) ||
      !ttx_abstract_same(capture.candidate, candidate)) {
    return TTX_INTERFACE_REJECTED;
  }
  return capture.relation;
}

void TtxTest::answer_interface(
    InterfaceModel model,
    ttx_interface_sink result) {
  CppInterfaceFrame frame = {
    .binding =
        {
          .operations =
              {
                .header =
                    {
                      .size = sizeof(ttx_interface_ops),
                      .abi_major = TTX_ABI_MAJOR,
                      .abi_minor = TTX_ABI_MINOR,
                    },
                .requirement = cpp_interface_requirement,
                .candidate = cpp_interface_candidate,
                .negotiate = cpp_interface_negotiate,
                .invoke = cpp_interface_invoke,
              },
          .frame = nullptr,
        },
    .model = std::move(model),
  };
  frame.binding.frame = &frame;
  const ttx_interface interface = {
    .operations = &frame.binding.operations,
    .owner = cpp_runtime.callback_authority,
    .value = 1,
  };
  result.operations->answer(result, interface);
}

void TtxTest::forward_interface(
    ttx_abstract source,
    ttx_abstract visible_candidate,
    ttx_abstract requirement,
    ttx_interface_sink result) {
  CppForwardFrame frame = {
    .operations =
        {
          .header =
              {
                .size = sizeof(ttx_interface_sink_ops),
                .abi_major = TTX_ABI_MAJOR,
                .abi_minor = TTX_ABI_MINOR,
              },
          .answer = cpp_forward_answer,
        },
    .source = source,
    .visible_candidate = visible_candidate,
    .requirement = requirement,
    .result = result,
  };
  const ttx_interface_sink forward = {
    .operations = &frame.operations,
    .owner = cpp_runtime.callback_authority,
    .value = 1,
  };
  source.operations->interface(source, requirement, forward);
}

void TtxTest::invoke(
    ttx_abstract candidate,
    ttx_abstract requirement,
    ttx_abstract operation,
    ttx_pack input,
    ttx_context context,
    ttx_pack_result result) {
  CppInvokeFrame frame = {
    .operations =
        {
          .header =
              {
                .size = sizeof(ttx_interface_sink_ops),
                .abi_major = TTX_ABI_MAJOR,
                .abi_minor = TTX_ABI_MINOR,
              },
          .answer = cpp_invoke_answer,
        },
    .candidate = candidate,
    .requirement = requirement,
    .operation = operation,
    .input = input,
    .context = context,
    .result = result,
  };
  const ttx_interface_sink callback = {
    .operations = &frame.operations,
    .owner = cpp_runtime.callback_authority,
    .value = 1,
  };
  candidate.operations->interface(candidate, requirement, callback);
}

void TtxTest::return_pack(
    ttx_context context,
    Routes entries,
    ttx_pack_result result) {
  std::vector<Ttx::Layouts::Fluid::Entry> projected;
  projected.reserve(entries.size());
  for (auto& [path, producer] : entries) {
    projected.push_back({.path = std::move(path), .producer = producer});
  }
  Ttx::Layouts::Fluid source(
      std::move(projected), cpp_runtime.callback_authority, 1);
  context.operations->pack(context, source.get_abi(), result);
}

auto TtxTest::retain_pack(ttx_context context, Routes entries) -> ttx_pack {
  CppPackCapture capture = {
    .operations =
        {
          .header =
              {
                .size = sizeof(ttx_pack_result_ops),
                .abi_major = TTX_ABI_MAJOR,
                .abi_minor = TTX_ABI_MINOR,
              },
          .unknown = cpp_pack_unknown,
          .none = cpp_pack_none,
          .packed = cpp_pack_packed,
          .support_failed = cpp_pack_support_failed,
        },
  };
  const ttx_pack_result result = {
    .operations = &capture.operations,
    .owner = cpp_runtime.callback_authority,
    .value = 1,
  };
  return_pack(context, std::move(entries), result);
  return capture.pack;
}

auto TtxTest::pack_cardinality(ttx_pack pack) -> std::optional<uint64_t> {
  if (pack.operations == nullptr) {
    return std::nullopt;
  }
  const ttx_layout layout = pack.operations->layout(pack);
  if (layout.operations == nullptr) {
    return std::nullopt;
  }
  CppEnumerableCapture capture = {
    .operations =
        {
          .header =
              {
                .size = sizeof(ttx_enumerable_result_ops),
                .abi_major = TTX_ABI_MAJOR,
                .abi_minor = TTX_ABI_MINOR,
              },
          .rejected = cpp_enumerable_rejected,
          .satisfied = cpp_enumerable_satisfied,
        },
  };
  const ttx_enumerable_result result = {
    .operations = &capture.operations,
    .owner = cpp_runtime.callback_authority,
    .value = 1,
  };
  layout.operations->enumerable(layout, result);
  if (!capture.answered || capture.enumerable.operations == nullptr) {
    return std::nullopt;
  }
  return capture.enumerable.operations->cardinality(capture.enumerable);
}
