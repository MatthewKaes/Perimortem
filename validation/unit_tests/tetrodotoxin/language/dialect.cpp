// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/language/dialect.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/static/vector.hpp"

#include "ttx/concept/invalid.hpp"

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
  Unsigned_8 post_order[3]{};
  Count post_calls[3]{};
  const Errors* error_objects[3]{};
  Count observed_error_counts[3]{};
  Count destruction_count = 0;
  Count post_count = 0;
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
      -> Option<Monograph&> override {
    return {};
  }

  auto observe_host_use() -> void { trace.host_uses++; }

 private:
  LifecycleTrace& trace;
};

class LifecycleMonograph : public Language::Dialect::Monograph {
 public:
  LifecycleMonograph(
      Allocator::Arena& domain,
      LifecycleDialect& host,
      LifecycleTrace& trace,
      Unsigned_8 identity)
      : Monograph(domain, Documentation::get_empty(), host),
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

  auto post_pass(Errors& errors) -> void override {
    const Count index = trace.post_count;

    trace.post_order[index] = identity;
    trace.post_calls[identity]++;
    trace.error_objects[index] = &errors;
    trace.observed_error_counts[index] = errors.get_size();
    trace.post_count++;

    static_cast<LifecycleDialect&>(host).observe_host_use();
    errors.create_general_error("post pass diagnostic"_view);
  }

 private:
  LifecycleTrace& trace;
  Unsigned_8 identity;
};

struct PersistenceTrace {
  Count restored = 0;
};

class PersistedMonograph : public Language::Dialect::Monograph {
 public:
  PersistedMonograph(
      Allocator::Arena& domain,
      Language::Dialect& host,
      View::Bytes fact)
      : Monograph(domain, Documentation::get_empty(), host), fact(fact) {}

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
      -> Option<Monograph&> override {
    return {};
  }

  auto encode(const Monograph& monograph) const
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
      -> Option<Monograph&> override {
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
        domain.construct<PersistedMonograph>(domain, *this, durable_fact);
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
      -> Option<Monograph&> override {
    return {};
  }
};

class DefaultMonograph : public Language::Dialect::Monograph {
 public:
  DefaultMonograph(Allocator::Arena& domain, Language::Dialect& host)
      : Monograph(domain, Documentation::get_empty(), host) {}

  auto get_name() const -> View::Bytes override { return "Default"_view; }

  auto resolve_context(View::Bytes) const -> const Abstract& override {
    return Invalid::get_invalid();
  }
};

class EmptyEncodingDialect : public DefaultDialect {
 public:
  EmptyEncodingDialect(Abstract& registry) : DefaultDialect(registry) {}

  auto encode(const Monograph&) const -> Option<Dynamic::Bytes> override {
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
  Language::Dialect::Monograph* first_base = &first;
  Language::Dialect::Monograph* second_base = &second;
  Language::Dialect* host_base = &host;

  first_base->~Monograph();
  second_base->~Monograph();
  host_base->~Dialect();

  EXPECT_EQ(trace.destruction_count, 3);
  EXPECT_EQ(trace.destruction_order[0], 0);
  EXPECT_EQ(trace.destruction_order[1], 1);
  EXPECT_EQ(trace.destruction_order[2], 3);
}

PERIMORTEM_UNIT_TEST(LanguageDialect, ordered_post_pass) {
  TestGraph registry;
  LifecycleTrace trace;
  Allocator::Arena arena;
  Errors errors;
  LifecycleDialect host(registry, trace);
  LifecycleMonograph first(arena, host, trace, 0);
  LifecycleMonograph second(arena, host, trace, 1);
  LifecycleMonograph third(arena, host, trace, 2);
  Static::Vector<Language::Dialect::Monograph*, 3> retained = {
    {&first, &second, &third},
  };

  for (Count i = 0; i < retained.get_size(); i++) {
    retained[i]->post_pass(errors);
  }

  EXPECT_EQ(trace.post_count, 3);
  EXPECT_EQ(trace.post_order[0], 0);
  EXPECT_EQ(trace.post_order[1], 1);
  EXPECT_EQ(trace.post_order[2], 2);
  EXPECT_EQ(trace.post_calls[0], 1);
  EXPECT_EQ(trace.post_calls[1], 1);
  EXPECT_EQ(trace.post_calls[2], 1);
  EXPECT(trace.error_objects[0] == &errors);
  EXPECT(trace.error_objects[1] == &errors);
  EXPECT(trace.error_objects[2] == &errors);
  EXPECT_EQ(trace.observed_error_counts[0], 0);
  EXPECT_EQ(trace.observed_error_counts[1], 1);
  EXPECT_EQ(trace.observed_error_counts[2], 2);
  EXPECT_EQ(trace.host_uses, 3);
  EXPECT_EQ(errors.get_size(), 3);
}

PERIMORTEM_UNIT_TEST(LanguageDialect, payload_round_trip) {
  TestGraph registry;
  PersistenceTrace trace;
  Allocator::Arena source_arena;
  Allocator::Arena restored_arena;
  PersistingDialect<0xA1> dialect(registry, trace);
  PersistedMonograph source(source_arena, dialect, "durable fact"_view);
  auto encoded = dialect.encode(source);
  auto restored = encoded.visit(
      []() { return Option<Language::Dialect::Monograph&>(); },
      [&](Dynamic::Bytes& payload) {
        return dialect.restore(restored_arena, payload);
      });

  encoded.visit([]() {}, [](Dynamic::Bytes& payload) { payload.set(0); });

  Bool durable = restored.visit(
      []() { return False; },
      [](Language::Dialect::Monograph& monograph) {
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
  PersistedMonograph other_source(
      source_arena, other_dialect, "other dialect"_view);
  auto wrong_payload = other_dialect.encode(other_source);

  auto truncated =
      dialect.restore(restored_arena, View::Bytes(truncated_bytes));
  auto invalid = dialect.restore(restored_arena, View::Bytes(invalid_bytes));
  auto wrong = wrong_payload.visit(
      []() { return Option<Language::Dialect::Monograph&>(); },
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
  Errors errors;
  DefaultDialect dialect(registry);
  DefaultMonograph monograph(arena, dialect);
  EmptyEncodingDialect empty_dialect(registry);
  DefaultMonograph empty_monograph(arena, empty_dialect);

  monograph.post_pass(errors);
  auto unsupported = dialect.encode(monograph);
  auto missing = dialect.restore(arena, "unsupported"_view);
  auto empty = empty_dialect.encode(empty_monograph);
  Bool successful_empty = empty.visit(
      []() { return False; },
      [](const Dynamic::Bytes& payload) {
        return payload.is_empty() ? True : False;
      });

  EXPECT(errors.is_empty());
  EXPECT_NOT(unsupported);
  EXPECT_NOT(missing);
  EXPECT(successful_empty);
}
