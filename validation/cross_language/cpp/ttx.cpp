// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "cross_language/cpp/ttx.hpp"

#include <cstdlib>
#include <mutex>

#include "ttx/concept/interface.hpp"
#include "ttx/model/addressable.hpp"
#include "ttx/query.hpp"

struct CppRuntime {
  std::vector<std::shared_ptr<Ttx::Abstract>> abstracts;
  std::mutex mutex;
};

class CppInterfaceFrame final : public Ttx::Interface {
 public:
  explicit CppInterfaceFrame(TtxTest::InterfaceModel model)
      : Interface(model.requirement, model.candidate, model.relation),
        invocation(std::move(model.invoke)) {}

 private:
  void invoke(
      ttx_abstract operation,
      ttx_pack input,
      ttx_context context,
      ttx_pack_result result) const override {
    if (invocation) {
      invocation(operation, input, context, result);
    } else {
      result.operations->none(result);
    }
  }
  TtxTest::Invocation invocation;
};

struct CppForwardFrame {
  ttx_interface_sink_ops operations;
  ttx_abstract source = {};
  ttx_abstract visible_candidate = {};
  ttx_abstract requirement = {};
  ttx_interface_sink result = {};
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

static void TTX_CALL
    cpp_forward_answer(ttx_interface_sink value, ttx_interface upstream) {
  if (value.operations == nullptr) {
    std::abort();
  }
  const auto& frame = *reinterpret_cast<const CppForwardFrame*>(value.self);
  if (!upstream ||
      !ttx_abstract_same(
          upstream->operations->candidate(upstream),
          Ttx::resolve(frame.source)) ||
      !ttx_abstract_same(
          upstream->operations->requirement(upstream), frame.requirement)) {
    Ttx::Interface invalid(
        frame.requirement, frame.visible_candidate, TTX_INTERFACE_UNKNOWN);
    invalid.publish(frame.result);
    return;
  }
  const ttx_abstract requirement = upstream->operations->requirement(upstream);
  const ttx_interface_relation relation =
      upstream->operations->negotiate(upstream);
  TtxTest::answer_interface(
      {
        .requirement = requirement,
        .candidate = frame.visible_candidate,
        .relation = relation,
        .invoke =
            [upstream](
                ttx_abstract operation, ttx_pack input, ttx_context context,
                ttx_pack_result result) {
              upstream->operations->invoke(
                  upstream, operation, input, context, result);
            },
      },
      frame.result);
}

static auto cpp_pack_capture(ttx_pack_result value) -> CppPackCapture& {
  if (value.operations == nullptr) {
    std::abort();
  }
  return *reinterpret_cast<CppPackCapture*>(value.self);
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
  return *reinterpret_cast<CppEnumerableCapture*>(value.self);
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

auto TtxTest::retain_abstract(std::shared_ptr<Ttx::Abstract> model)
    -> ttx_abstract {
  std::lock_guard lock(cpp_runtime.mutex);
  cpp_runtime.abstracts.push_back(std::move(model));
  return cpp_runtime.abstracts.back()->get_abi();
}

auto TtxTest::redispatch(ttx_abstract original) -> ttx_abstract {
  return retain_abstract(std::make_shared<Ttx::Addressable>(original));
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
  return Ttx::resolve(value);
}

auto TtxTest::resolve_concept(ttx_abstract value, ttx_borrowed_bytes route)
    -> ttx_abstract {
  return Ttx::resolve_concept(value, route);
}

auto TtxTest::relation(ttx_abstract candidate, ttx_abstract requirement)
    -> ttx_interface_relation {
  return Ttx::relation(candidate, requirement);
}

void TtxTest::answer_interface(
    InterfaceModel model,
    ttx_interface_sink result) {
  const CppInterfaceFrame witness(std::move(model));
  witness.publish(result);
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
    .self = reinterpret_cast<ttx_interface_sink_self*>(&frame),
  };
  source = Ttx::resolve(source);
  source->operations->interface(source, requirement, forward);
}

void TtxTest::forward_bytes(
    ttx_abstract source,
    ttx_abstract candidate,
    ttx_bytes_result result) {
  const auto observed = Ttx::resolve_bytes(source);
  if (observed.state == Ttx::Observation::Unknown) {
    result.operations->unknown(result);
    return;
  }
  if (observed.state == Ttx::Observation::None) {
    result.operations->none(result);
    return;
  }
  struct View {
    ttx_abstract candidate;
    ttx_bytes bytes;
  } view{candidate, observed.bytes};
  static const ttx_bytes_ops ops = {
    .header = {sizeof(ttx_bytes_ops), TTX_ABI_MAJOR, TTX_ABI_MINOR},
    .candidate =
        [](ttx_bytes v) {
          return reinterpret_cast<const View*>(v.self)->candidate;
        },
    .size =
        [](ttx_bytes v) {
          const auto b = reinterpret_cast<const View*>(v.self)->bytes;
          return b.operations->size(b);
        },
    .visit =
        [](ttx_bytes v, ttx_bytes_sink sink) {
          const auto b = reinterpret_cast<const View*>(v.self)->bytes;
          b.operations->visit(b, sink);
        },
  };
  result.operations->resolved(
      result,
      {.operations = &ops, .self = reinterpret_cast<ttx_bytes_self*>(&view)});
}

void TtxTest::invoke(
    ttx_abstract candidate,
    ttx_abstract requirement,
    ttx_abstract operation,
    ttx_pack input,
    ttx_context context,
    ttx_pack_result result) {
  const auto answer =
      Ttx::invoke(candidate, requirement, operation, input, context);
  switch (answer.state) {
  case Ttx::PackObservationState::Unknown:
    result.operations->unknown(result);
    break;
  case Ttx::PackObservationState::None:
    result.operations->none(result);
    break;
  case Ttx::PackObservationState::Packed:
    result.operations->packed(result, answer.pack);
    break;
  case Ttx::PackObservationState::SupportFailed:
    result.operations->support_failed(result, answer.failure);
    break;
  }
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
  Ttx::Layouts::Fluid source(std::move(projected));
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
    .self = reinterpret_cast<ttx_pack_result_self*>(&capture),
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
    .self = reinterpret_cast<ttx_enumerable_result_self*>(&capture),
  };
  layout.operations->enumerable(layout, result);
  if (!capture.answered || capture.enumerable.operations == nullptr) {
    return std::nullopt;
  }
  return capture.enumerable.operations->cardinality(capture.enumerable);
}
