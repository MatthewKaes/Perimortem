// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/environment/retention.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "ttx/concept/documentation.hpp"
#include "ttx/concept/invalid.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Utility;
using namespace Tetrodotoxin;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Validation;

static Harness EnvironmentRetention = {
  .name = "Tetrodotoxin::Environment::Retention"_view,
};

struct RetentionTrace {
  Count links[8]{};
  Count finalizers[8]{};
  Unsigned_8 events[32]{};
  Count event_count = 0;
  Bool retained_during_link = True;
  Bool retained_during_finalize = True;
};

class RetentionMonograph : public Language::Monograph {
 public:
  RetentionMonograph(
      Allocator::Arena& domain,
      RetentionTrace& trace,
      Unsigned_8 identity,
      Bool link_result = True,
      Bool finalize_result = True)
      : Monograph(domain, Documentation::get_empty()),
        trace(trace),
        identity(identity),
        link_result(link_result),
        finalize_result(finalize_result) {}

  auto get_name() const -> View::Bytes override {
    return "RetentionMonograph"_view;
  }

  auto resolve_context(View::Bytes) const -> const Abstract& override {
    return Invalid::get_invalid();
  }

  auto link() -> Bool override {
    trace.links[identity]++;
    trace.events[trace.event_count] = identity;
    trace.event_count++;
    if (retention != nullptr && candidate != nullptr) {
      trace.retained_during_link = retention->retain(*candidate, {});
    }

    if (!link_message.is_empty()) {
      report(link_anchor, link_message, link_hint);
    }
    return link_result;
  }

  auto finalize() -> Bool override {
    trace.finalizers[identity]++;
    trace.events[trace.event_count] = Unsigned_8(identity + 16);
    trace.event_count++;
    if (retention != nullptr && candidate != nullptr) {
      trace.retained_during_finalize = retention->retain(*candidate, {});
    }

    if (!finalize_message.is_empty()) {
      report(finalize_anchor, finalize_message, finalize_hint);
    }
    return finalize_result;
  }

  auto attempt_retention(
      Environment::Retention& selected_retention,
      Language::Monograph& selected_candidate) -> void {
    retention = &selected_retention;
    candidate = &selected_candidate;
  }

  auto report_during_link(
      Option<Anchor> anchor,
      View::Bytes message,
      View::Bytes hint = {}) -> void {
    link_anchor = anchor;
    link_message = message;
    link_hint = hint;
  }

  auto report_during_finalize(
      Option<Anchor> anchor,
      View::Bytes message,
      View::Bytes hint = {}) -> void {
    finalize_anchor = anchor;
    finalize_message = message;
    finalize_hint = hint;
  }

 private:
  RetentionTrace& trace;
  Environment::Retention* retention = nullptr;
  Language::Monograph* candidate = nullptr;
  View::Bytes link_message;
  View::Bytes link_hint;
  View::Bytes finalize_message;
  View::Bytes finalize_hint;
  Option<Anchor> link_anchor;
  Option<Anchor> finalize_anchor;
  Unsigned_8 identity;
  Bool link_result;
  Bool finalize_result;
};

static auto contains(View::Bytes text, View::Bytes fragment) -> Bool {
  if (fragment.get_size() > text.get_size()) {
    return False;
  }

  for (Count start = 0; start <= text.get_size() - fragment.get_size();
       start++) {
    Bool matches = True;
    for (Count i = 0; i < fragment.get_size(); i++) {
      if (text[start + i] != fragment[i]) {
        matches = False;
        break;
      }
    }

    if (matches) {
      return True;
    }
  }

  return False;
}

PERIMORTEM_UNIT_TEST(EnvironmentRetention, frozen_range) {
  Allocator::Arena domain;
  Environment::Retention retention(domain);
  RetentionTrace trace;
  auto& first = domain.construct<RetentionMonograph>(domain, trace, 0);
  auto& second = domain.construct<RetentionMonograph>(domain, trace, 1);
  first.attempt_retention(retention, second);
  Errors link_errors;

  ASSERT(retention.retain(first, {}));
  ASSERT(retention.link(link_errors));
  EXPECT_NOT(trace.retained_during_link);
  Errors finalize_errors;
  ASSERT(retention.finalize(finalize_errors));
  EXPECT_NOT(trace.retained_during_finalize);
  EXPECT_EQ(retention.get_size(), Count(1));

  Errors second_errors;
  ASSERT(retention.retain(second, {}));
  ASSERT(retention.link(second_errors));
  ASSERT(retention.finalize(second_errors));
  EXPECT_EQ(trace.links[0], Count(1));
  EXPECT_EQ(trace.links[1], Count(1));
  EXPECT_EQ(trace.finalizers[0], Count(1));
  EXPECT_EQ(trace.finalizers[1], Count(1));
}

PERIMORTEM_UNIT_TEST(EnvironmentRetention, wrong_phase_preserves_range) {
  Allocator::Arena domain;
  Environment::Retention retention(domain);
  RetentionTrace trace;
  auto& monograph = domain.construct<RetentionMonograph>(domain, trace, 0);
  Errors errors;

  ASSERT(retention.retain(monograph, {}));
  EXPECT_NOT(retention.finalize(errors));
  ASSERT(retention.has_staged());
  ASSERT(retention.link(errors));
  EXPECT(retention.awaits_finalize());
  EXPECT_NOT(retention.link(errors));
  EXPECT(retention.awaits_finalize());
  ASSERT(retention.finalize(errors));
  EXPECT_NOT(retention.finalize(errors));
  EXPECT_EQ(trace.links[0], Count(1));
  EXPECT_EQ(trace.finalizers[0], Count(1));
}

PERIMORTEM_UNIT_TEST(EnvironmentRetention, link_failure_suppresses_finalize) {
  Allocator::Arena domain;
  Environment::Retention retention(domain);
  RetentionTrace trace;
  auto& first = domain.construct<RetentionMonograph>(domain, trace, 0);
  auto& failed = domain.construct<RetentionMonograph>(domain, trace, 1, False);
  auto& last = domain.construct<RetentionMonograph>(domain, trace, 2);
  Errors errors;

  ASSERT(retention.retain(first, {}));
  ASSERT(retention.retain(failed, {}));
  ASSERT(retention.retain(last, {}));
  EXPECT_NOT(retention.link(errors));
  EXPECT_NOT(retention.finalize(errors));
  EXPECT_NOT(retention.link(errors));
  for (Count i = 0; i < 3; i++) {
    EXPECT_EQ(trace.links[i], Count(1));
    EXPECT_EQ(trace.finalizers[i], Count(0));
    EXPECT_EQ(trace.events[i], Unsigned_8(i));
  }
}

PERIMORTEM_UNIT_TEST(EnvironmentRetention, finalize_failure_continues_range) {
  Allocator::Arena domain;
  Environment::Retention retention(domain);
  RetentionTrace trace;
  auto& first = domain.construct<RetentionMonograph>(domain, trace, 0);
  auto& failed =
      domain.construct<RetentionMonograph>(domain, trace, 1, True, False);
  auto& last = domain.construct<RetentionMonograph>(domain, trace, 2);
  Errors errors;

  ASSERT(retention.retain(first, {}));
  ASSERT(retention.retain(failed, {}));
  ASSERT(retention.retain(last, {}));
  ASSERT(retention.link(errors));
  EXPECT_NOT(retention.finalize(errors));
  EXPECT_NOT(retention.has_staged());
  EXPECT_NOT(retention.awaits_finalize());
  EXPECT_NOT(retention.finalize(errors));
  ASSERT_EQ(trace.event_count, Count(6));
  for (Count i = 0; i < 3; i++) {
    EXPECT_EQ(trace.links[i], Count(1));
    EXPECT_EQ(trace.finalizers[i], Count(1));
    EXPECT_EQ(trace.events[i], Unsigned_8(i));
    EXPECT_EQ(trace.events[i + 3], Unsigned_8(i + 16));
  }
}

PERIMORTEM_UNIT_TEST(EnvironmentRetention, abandonment) {
  Allocator::Arena domain;
  Environment::Retention retention(domain);
  RetentionTrace trace;
  auto& staged = domain.construct<RetentionMonograph>(domain, trace, 0);
  auto& linked = domain.construct<RetentionMonograph>(domain, trace, 1);
  auto& completed = domain.construct<RetentionMonograph>(domain, trace, 2);
  Errors errors;

  ASSERT(retention.retain(staged, {}));
  retention.abandon();
  EXPECT_NOT(retention.has_staged());
  EXPECT_EQ(trace.links[0], Count(0));

  ASSERT(retention.retain(linked, {}));
  ASSERT(retention.link(errors));
  retention.abandon();
  EXPECT_NOT(retention.finalize(errors));
  EXPECT_EQ(trace.links[1], Count(1));
  EXPECT_EQ(trace.finalizers[1], Count(0));

  ASSERT(retention.retain(completed, {}));
  ASSERT(retention.link(errors));
  ASSERT(retention.finalize(errors));
  EXPECT_EQ(trace.links[2], Count(1));
  EXPECT_EQ(trace.finalizers[2], Count(1));
}

PERIMORTEM_UNIT_TEST(EnvironmentRetention, diagnostic_coordinates) {
  static constexpr View::Bytes authored_body = "zero target rest"_view;
  static constexpr View::Bytes inherited_body = "dependency"_view;
  Allocator::Arena domain;
  Environment::Retention retention(domain);
  RetentionTrace trace;
  auto& authored = domain.construct<RetentionMonograph>(domain, trace, 0);
  auto& inherited = domain.construct<RetentionMonograph>(domain, trace, 1);
  auto& source_free = domain.construct<RetentionMonograph>(domain, trace, 2);
  Token authored_full(0, 1, 1, 16, Code::Type::Addressable);
  Token target(5, 1, 6, 6, Code::Type::Addressable);
  Token rest(12, 1, 13, 4, Code::Type::Addressable);
  Token dependency(0, 1, 1, 10, Code::Type::Addressable);
  authored.report_during_link(
      Anchor::create(target, Span(authored_full)),
      "Authored link diagnostic."_view, "Authored hint."_view);
  authored.report_during_finalize(
      Anchor::create(Span(rest)), "Authored finalize diagnostic."_view);
  inherited.report_during_link({}, "Inherited diagnostic."_view);
  source_free.report_during_link({}, "Source free diagnostic."_view);
  Errors errors;

  ASSERT(retention.retain(
      authored, Environment::Origin(
                    "authored.ttx"_view, authored_body, Span(authored_full))));
  ASSERT(retention.retain(
      inherited, Environment::Origin(
                     "inherited.ttx"_view, inherited_body, Span(dependency))));
  ASSERT(retention.retain(source_free, {}));
  ASSERT(retention.link(errors));
  EXPECT_EQ(errors.get_size(), Count(2));
  ASSERT(retention.finalize(errors));
  EXPECT_EQ(errors.get_size(), Count(3));
  EXPECT_NOT(retention.finalize(errors));
  EXPECT_EQ(errors.get_size(), Count(3));
  ASSERT_EQ(trace.event_count, Count(6));
  for (Count i = 0; i < 3; i++) {
    EXPECT_EQ(trace.events[i], Unsigned_8(i));
    EXPECT_EQ(trace.events[i + 3], Unsigned_8(i + 16));
  }

  Allocator::Arena render_domain;
  View::Bytes first = errors.render_message(render_domain, 0);
  View::Bytes second = errors.render_message(render_domain, 1);
  View::Bytes third = errors.render_message(render_domain, 2);
  EXPECT(contains(first, "authored.ttx:1:6:"_view));
  EXPECT(contains(first, "Authored link diagnostic."_view));
  EXPECT(contains(first, "Authored hint."_view));
  EXPECT(contains(first, "^-----\n"_view));
  EXPECT(contains(second, "inherited.ttx:1:1:"_view));
  EXPECT(contains(second, "Inherited diagnostic."_view));
  EXPECT(contains(second, "dependency"_view));
  EXPECT(contains(second, "^---------\n"_view));
  EXPECT(contains(third, "authored.ttx:1:13:"_view));
  EXPECT(contains(third, "Authored finalize diagnostic."_view));
  EXPECT_NOT(contains(first, "Source free diagnostic."_view));
  EXPECT_NOT(contains(second, "Source free diagnostic."_view));
  EXPECT_NOT(contains(third, "Source free diagnostic."_view));
}
