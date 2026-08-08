// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/language/dialect.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/static/vector.hpp"

#include "ttx/concept/invalid.hpp"
#include "ttx/model/type.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Utility;
using namespace Tetrodotoxin;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Validation;

class TestGraph : public Abstract {
 public:
  auto get_name() const -> View::Bytes override { return "Graph"_view; }

  auto get_documentation() const -> const Documentation& override {
    return Documentation::get_empty();
  }

  auto resolve_context(View::Bytes) const -> const Abstract& override {
    return Invalid::get_invalid();
  }
};

struct LifecycleTrace {
  Unsigned_8 destruction_order[3]{};
  Unsigned_8 link_order[3]{};
  Unsigned_8 finalize_order[3]{};
  Count link_calls[3]{};
  Count finalize_calls[3]{};
  Count destruction_count = 0;
  Count link_count = 0;
  Count finalize_count = 0;
  Count host_uses = 0;
};

class LifecycleDialect : public Language::Dialect {
 public:
  LifecycleDialect(Abstract& registry, LifecycleTrace& trace)
      : Dialect(registry), trace(trace) {}

  ~LifecycleDialect() override {
    trace.destruction_order[trace.destruction_count] = 3;
    trace.destruction_count++;
  }

  auto interpret(Allocator::Arena&, Cursor&, const Documentation&, Abstract&)
      -> Option<Language::Monograph&> override {
    return {};
  }

  auto observe_host_use() -> void { trace.host_uses++; }

 private:
  LifecycleTrace& trace;
};

class LifecycleMonograph : public Language::Monograph {
 public:
  LifecycleMonograph(
      Allocator::Arena& domain,
      LifecycleDialect& host,
      LifecycleTrace& trace,
      Unsigned_8 identity)
      : Monograph(domain, Documentation::get_empty()),
        lifecycle_host(host),
        trace(trace),
        identity(identity) {}

  ~LifecycleMonograph() override {
    trace.destruction_order[trace.destruction_count] = identity;
    trace.destruction_count++;
  }

  auto get_name() const -> View::Bytes override { return "Lifecycle"_view; }

  auto resolve_context(View::Bytes) const -> const Abstract& override {
    return Invalid::get_invalid();
  }

  auto link() -> Bool override {
    const Count index = trace.link_count;

    trace.link_order[index] = identity;
    trace.link_calls[identity]++;
    trace.link_count++;

    lifecycle_host.observe_host_use();
    return identity == 1 ? False : True;
  }

  auto finalize() -> Bool override {
    const Count index = trace.finalize_count;

    trace.finalize_order[index] = identity;
    trace.finalize_calls[identity]++;
    trace.finalize_count++;

    lifecycle_host.observe_host_use();
    return identity == 2 ? False : True;
  }

 private:
  LifecycleDialect& lifecycle_host;
  LifecycleTrace& trace;
  Unsigned_8 identity;
};

struct PersistenceTrace {
  Count restored = 0;
};

class PersistedMonograph : public Language::Monograph {
 public:
  PersistedMonograph(Allocator::Arena& domain, View::Bytes fact)
      : Monograph(domain, Documentation::get_empty()), fact(fact) {}

  auto get_name() const -> View::Bytes override { return "Persisted"_view; }

  auto resolve_context(View::Bytes) const -> const Abstract& override {
    return Invalid::get_invalid();
  }

  auto get_fact() const -> View::Bytes { return fact; }

 private:
  View::Bytes fact;
};

template <Unsigned_8 marker>
class PersistingDialect : public Language::Dialect {
 public:
  PersistingDialect(Abstract& registry, PersistenceTrace& trace)
      : Dialect(registry), trace(trace) {}

  auto interpret(Allocator::Arena&, Cursor&, const Documentation&, Abstract&)
      -> Option<Language::Monograph&> override {
    return {};
  }

  auto encode(const Language::Monograph& monograph) const
      -> Option<Dynamic::Bytes> override {
    const auto& selected = static_cast<const PersistedMonograph&>(monograph);
    const View::Bytes fact = selected.get_fact();
    Dynamic::Bytes payload;

    if (fact.get_size() > 255) {
      return {};
    }

    payload.append(marker);
    payload.append(static_cast<Unsigned_8>(fact.get_size()));
    payload.concat(fact);
    return payload;
  }

  auto restore(Allocator::Arena& domain, View::Bytes payload)
      -> Option<Language::Monograph&> override {
    if (payload.get_size() < 2) {
      return {};
    }

    if (payload[0] != marker) {
      return {};
    }

    const Count fact_size = payload[1];
    if (payload.get_size() != fact_size + 2) {
      return {};
    }

    const View::Bytes durable_fact = domain.proxy(payload.slice(2, fact_size));
    auto& monograph =
        domain.construct<PersistedMonograph>(domain, durable_fact);
    trace.restored++;
    return monograph;
  }

 private:
  PersistenceTrace& trace;
};

class DefaultDialect : public Language::Dialect {
 public:
  DefaultDialect(Abstract& registry) : Dialect(registry) {}

  auto interpret(Allocator::Arena&, Cursor&, const Documentation&, Abstract&)
      -> Option<Language::Monograph&> override {
    return {};
  }
};

class DefaultMonograph : public Language::Monograph {
 public:
  DefaultMonograph(Allocator::Arena& domain)
      : Monograph(domain, Documentation::get_empty()) {}

  auto get_name() const -> View::Bytes override { return "Default"_view; }

  auto resolve_context(View::Bytes) const -> const Abstract& override {
    return Invalid::get_invalid();
  }
};

class EmptyEncodingDialect : public DefaultDialect {
 public:
  EmptyEncodingDialect(Abstract& registry) : DefaultDialect(registry) {}

  auto encode(const Language::Monograph&) const
      -> Option<Dynamic::Bytes> override {
    return Dynamic::Bytes();
  }
};

static Harness LanguageDialect = {
  .name = "Tetrodotoxin::Language::Dialect"_view,
};

PERIMORTEM_UNIT_TEST(LanguageDialect, base_destruction_order) {
  TestGraph registry;
  LifecycleTrace trace;
  Allocator::Arena arena;
  auto& host = arena.construct<LifecycleDialect>(registry, trace);
  auto& first = arena.construct<LifecycleMonograph>(arena, host, trace, 0);
  auto& second = arena.construct<LifecycleMonograph>(arena, host, trace, 1);
  Language::Monograph* first_base = &first;
  Language::Monograph* second_base = &second;
  Language::Dialect* host_base = &host;

  first_base->~Monograph();
  second_base->~Monograph();
  host_base->~Dialect();

  EXPECT_EQ(trace.destruction_count, 3);
  EXPECT_EQ(trace.destruction_order[0], 0);
  EXPECT_EQ(trace.destruction_order[1], 1);
  EXPECT_EQ(trace.destruction_order[2], 3);
}

PERIMORTEM_UNIT_TEST(LanguageDialect, ordered_completion_hooks) {
  TestGraph registry;
  LifecycleTrace trace;
  Allocator::Arena arena;
  LifecycleDialect host(registry, trace);
  LifecycleMonograph first(arena, host, trace, 0);
  LifecycleMonograph second(arena, host, trace, 1);
  LifecycleMonograph third(arena, host, trace, 2);
  Bool link_results[3]{};
  Bool finalize_results[3]{};
  Static::Vector<Language::Monograph*, 3> retained = {
    {&first, &second, &third},
  };

  for (Count i = 0; i < retained.get_size(); i++) {
    link_results[i] = retained[i]->link();
  }

  for (Count i = 0; i < retained.get_size(); i++) {
    finalize_results[i] = retained[i]->finalize();
  }

  EXPECT_EQ(trace.link_count, 3);
  EXPECT_EQ(trace.link_order[0], 0);
  EXPECT_EQ(trace.link_order[1], 1);
  EXPECT_EQ(trace.link_order[2], 2);
  EXPECT_EQ(trace.link_calls[0], 1);
  EXPECT_EQ(trace.link_calls[1], 1);
  EXPECT_EQ(trace.link_calls[2], 1);
  EXPECT(link_results[0]);
  EXPECT_NOT(link_results[1]);
  EXPECT(link_results[2]);
  EXPECT_EQ(trace.finalize_count, 3);
  EXPECT_EQ(trace.finalize_order[0], 0);
  EXPECT_EQ(trace.finalize_order[1], 1);
  EXPECT_EQ(trace.finalize_order[2], 2);
  EXPECT_EQ(trace.finalize_calls[0], 1);
  EXPECT_EQ(trace.finalize_calls[1], 1);
  EXPECT_EQ(trace.finalize_calls[2], 1);
  EXPECT(finalize_results[0]);
  EXPECT(finalize_results[1]);
  EXPECT_NOT(finalize_results[2]);
  EXPECT_EQ(trace.host_uses, 6);
}

PERIMORTEM_UNIT_TEST(LanguageDialect, ordered_diagnostics_are_stable) {
  Unsigned_8 message[] = {'f', 'i', 'r', 's', 't'};
  Unsigned_8 hint[] = {'h', 'i', 'n', 't'};
  Allocator::Arena arena;
  DefaultMonograph monograph(arena);
  Token opening(2, 1, 2, 1, Code::Type::Addressable);
  Token closing(8, 1, 8, 1, Code::Type::Addressable);
  Span span(opening, closing);

  monograph.report(
      Anchor::create(opening, span), View::Bytes(message), View::Bytes(hint));
  message[0] = 'x';
  hint[0] = 'x';
  monograph.report(Anchor::create(Span(closing)), "second"_view);

  View::Vector<Language::Diagnostic> diagnostics = monograph.get_diagnostics();
  ASSERT_EQ(diagnostics.get_size(), Count(2));
  const Language::Diagnostic& first = diagnostics.get_data()[0];
  const Language::Diagnostic& second = diagnostics.get_data()[1];
  ASSERT(first.get_anchor());
  ASSERT(second.get_anchor());
  EXPECT_EQ(first.get_anchor()->get_token().get_offset(), opening.get_offset());
  EXPECT_EQ(first.get_anchor()->get_span().get_offset(), span.get_offset());
  EXPECT_EQ(first.get_anchor()->get_span().get_size(), span.get_size());
  EXPECT_TEXT(first.get_message(), "first"_view);
  EXPECT_TEXT(first.get_hint(), "hint"_view);
  EXPECT_EQ(
      second.get_anchor()->get_span().get_offset(),
      Count(closing.get_offset()));
  EXPECT_TEXT(second.get_message(), "second"_view);
  EXPECT(second.get_hint().is_empty());
}

PERIMORTEM_UNIT_TEST(LanguageDialect, payload_round_trip) {
  TestGraph registry;
  PersistenceTrace trace;
  Allocator::Arena source_arena;
  Allocator::Arena restored_arena;
  PersistingDialect<0xA1> dialect(registry, trace);
  PersistedMonograph source(source_arena, "durable fact"_view);
  auto encoded = dialect.encode(source);
  auto restored = encoded.visit(
      []() { return Option<Language::Monograph&>(); },
      [&](Dynamic::Bytes& payload) {
        return dialect.restore(restored_arena, payload);
      });

  encoded.visit([]() {}, [](Dynamic::Bytes& payload) { payload.set(0); });

  Bool durable = restored.visit(
      []() { return False; },
      [](Language::Monograph& monograph) {
        const auto& selected =
            static_cast<const PersistedMonograph&>(monograph);
        return selected.get_fact() == "durable fact"_view ? True : False;
      });

  EXPECT(durable);
  EXPECT_EQ(trace.restored, 1);
}

PERIMORTEM_UNIT_TEST(LanguageDialect, rejects_invalid_payloads) {
  const Unsigned_8 truncated_bytes[] = {0xA1};
  const Unsigned_8 invalid_bytes[] = {0xA1, 3, 'x'};
  TestGraph registry;
  PersistenceTrace trace;
  PersistenceTrace other_trace;
  Allocator::Arena source_arena;
  Allocator::Arena restored_arena;
  PersistingDialect<0xA1> dialect(registry, trace);
  PersistingDialect<0xB2> other_dialect(registry, other_trace);
  PersistedMonograph other_source(source_arena, "other dialect"_view);
  auto wrong_payload = other_dialect.encode(other_source);

  auto truncated =
      dialect.restore(restored_arena, View::Bytes(truncated_bytes));
  auto invalid = dialect.restore(restored_arena, View::Bytes(invalid_bytes));
  auto wrong = wrong_payload.visit(
      []() { return Option<Language::Monograph&>(); },
      [&](Dynamic::Bytes& payload) {
        return dialect.restore(restored_arena, payload);
      });

  EXPECT_NOT(truncated);
  EXPECT_NOT(invalid);
  EXPECT_NOT(wrong);
  EXPECT_EQ(trace.restored, 0);
}

PERIMORTEM_UNIT_TEST(LanguageDialect, explicit_default_persistence) {
  TestGraph registry;
  Allocator::Arena arena;
  DefaultDialect dialect(registry);
  DefaultMonograph monograph(arena);
  EmptyEncodingDialect empty_dialect(registry);
  DefaultMonograph empty_monograph(arena);

  const Bool linked = monograph.link();
  const Bool finalized = monograph.finalize();
  auto unsupported = dialect.encode(monograph);
  auto missing = dialect.restore(arena, "unsupported"_view);
  auto empty = empty_dialect.encode(empty_monograph);
  Bool successful_empty = empty.visit(
      []() { return False; },
      [](const Dynamic::Bytes& payload) {
        return payload.is_empty() ? True : False;
      });

  EXPECT(linked);
  EXPECT(finalized);
  EXPECT_NOT(monograph.is<Ttx::Model::Type>());
  EXPECT(&monograph.resolve_context("source"_view) == &Invalid::get_invalid());
  EXPECT_NOT(unsupported);
  EXPECT_NOT(missing);
  EXPECT(successful_empty);
}
