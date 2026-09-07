// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/language/dialect.hpp"

#include <new>

#include "tetrodotoxin/language/parser/comment.hpp"
#include "tetrodotoxin/language/parser/dialect.hpp"
#include "tetrodotoxin/language/parser/import.hpp"
#include "ttx/lexical/tokenizer.hpp"

using namespace Tetrodotoxin::Language;
using namespace Perimortem;
using namespace Ttx::Lexical;

static auto view(ttx_borrowed_bytes value) -> Core::View::Bytes {
  return {value.data, value.size};
}

// SourceInput and graph allocations need the same lifetime, not the same
// allocator. The native provider borrows retained input bytes and puts its
// semantic objects in this Arena. Releasing the graph returns both resources
// to their actual owners, including when no Monograph could be constructed.
struct NativeSource {
  Dialect& dialect;
  tetrodotoxin_source_input input;
  Memory::Allocator::Arena arena;
  Errors errors;
  Tokenizer tokenizer;
  Associations associations;
  Monograph* root = nullptr;
  size_t references = 1;
  bool accepted = false;

  NativeSource(Dialect& dialect, tetrodotoxin_source_input input)
      : dialect(dialect),
        input(input),
        tokenizer(
            arena,
            view(input.operations->bytes(input.self)),
            view(input.operations->diagnostic_path(input.self))),
        associations(arena) {
    input.operations->retain(input.self);
  }
  ~NativeSource() {
    if (root) {
      root->~Monograph();
    }
    input.operations->release(input.self);
  }
};

static auto source(tetrodotoxin_source_graph_self* self) -> NativeSource& {
  return *reinterpret_cast<NativeSource*>(self);
}

static auto source_anchor(Anchor anchor) -> tetrodotoxin_source_anchor {
  const auto span = anchor.get_span();
  const auto focus = anchor.get_token();
  return {
    span.get_offset(), span.get_size(), focus.get_offset(), focus.get_size()};
}

static auto validate(NativeSource& state, Errors& errors) -> bool {
  if (!state.root || !state.accepted) {
    return false;
  }
  Cursor cursor(state.tokenizer, errors, state.associations);
  return bool(state.root->validate(cursor) && errors.is_empty());
}

static void TTX_CALL retain_source(tetrodotoxin_source_graph_self* self) {
  ++source(self).references;
}

static void TTX_CALL release_source(tetrodotoxin_source_graph_self* self) {
  auto& state = source(self);
  if (--state.references == 0) {
    delete &state;
  }
}

static auto TTX_CALL source_root(tetrodotoxin_source_graph_self* self)
    -> ttx_abstract {
  const auto& state = source(self);
  return state.root ? state.root->get_root() : ttx_unknown();
}

static auto TTX_CALL source_dialect(tetrodotoxin_source_graph_self* self)
    -> ttx_abstract {
  return source(self).dialect.get_abi();
}

static void TTX_CALL dependencies(
    tetrodotoxin_source_graph_self* self,
    tetrodotoxin_source_dependency_sink sink) {
  const auto& state = source(self);
  if (state.root) {
    for (auto* imported : state.root->get_imports()) {
      sink.operations->dependency(sink.self, imported->dependency());
    }
  }
  sink.operations->completed(sink.self);
}

static void TTX_CALL validate_source(
    tetrodotoxin_source_graph_self* self,
    tetrodotoxin_validation_result result) {
  Errors errors;
  const bool accepted = validate(source(self), errors);
  if (accepted) {
    result.operations->accepted(result.self);
  } else {
    result.operations->incomplete(result.self);
  }
}

static void TTX_CALL produce_source(
    tetrodotoxin_source_graph_self* self,
    ttx_context context,
    tetrodotoxin_production_result result) {
  auto& state = source(self);
  Errors errors;
  if (!validate(state, errors)) {
    result.operations->unknown(result.self);
    return;
  }
  state.dialect.produce(*state.root, context, result);
}

static void TTX_CALL associations(
    tetrodotoxin_source_graph_self* self,
    tetrodotoxin_source_associations sink) {
  for (const auto& entry : source(self).associations.get_entries()) {
    sink.operations->association(
        sink, source_anchor(entry.get_anchor()), entry.get_semantic());
  }
  sink.operations->completed(sink);
}

static void TTX_CALL diagnostics(
    tetrodotoxin_source_graph_self* self,
    tetrodotoxin_source_diagnostics sink) {
  auto& state = source(self);
  const auto emit = [&](const Errors& errors) {
    for (Count i = 0; i < errors.get_size(); ++i) {
      const auto message = errors.get_message(i);
      sink.operations->diagnostic(
          sink, source_anchor(errors.get_anchor(i)),
          {message.get_data(), message.get_size()});
    }
  };
  emit(state.errors);
  Errors current;
  validate(state, current);
  emit(current);
  sink.operations->completed(sink);
}

static void TTX_CALL interpret_source(
    tetrodotoxin_dialect_provider_self* self,
    tetrodotoxin_source_input input,
    ttx_abstract context,
    tetrodotoxin_interpret_result result) {
  auto& dialect = *reinterpret_cast<Dialect*>(self);
  auto* state = new NativeSource(dialect, input);
  Cursor cursor(state->tokenizer, state->errors, state->associations);
  const auto opening = cursor.current();
  if (!cursor.get_code().is_comment()) {
    cursor.create_token_error(
        "Source requires an opening documentation comment."_view);
  } else {
    const auto& documentation = Parser::Comment::parse(cursor);
    if (documentation.is_empty()) {
      cursor.create_token_error(
          opening,
          "Raw comments do not provide the required source documentation."_view);
    }
    const auto declaration = cursor.current();
    const auto name = Parser::Dialect::parse(cursor);
    if (name != dialect.get_name()) {
      cursor.create_token_error(
          declaration, "Source names a different installed Dialect."_view);
    } else {
      // Parse shared imports once, then let the chosen frontend construct its
      // real root. No later completion pass rewrites that root or replays
      // source.
      Memory::Managed::Vector<Import::Description> imports(state->arena);
      while (Parser::Import::is_next(cursor)) {
        const auto& comment = Parser::Comment::parse(cursor);
        auto imported = Parser::Import::parse(cursor, comment);
        if (!imported) {
          break;
        }
        imports.insert(*imported);
      }
      auto interpreted = dialect.interpret(
          cursor, documentation,
          Anchor::create(declaration, Span(opening, cursor.peek(-1))), context);
      if (interpreted) {
        state->root = &*interpreted;
        for (const auto& imported : imports.get_view()) {
          const auto retained =
              state->root->retain_import(imported, state->associations);
          if (!retained) {
            cursor.create_token_error("Duplicate import name."_view);
          }
        }
        state->accepted = bool(state->errors.is_empty());
      }
    }
  }
  static const tetrodotoxin_source_graph_ops operations = {
    .header =
        {sizeof(tetrodotoxin_source_graph_ops), TTX_ABI_MAJOR, TTX_ABI_MINOR},
    .retain = retain_source,
    .release = release_source,
    .root = source_root,
    .dialect = source_dialect,
    .visit_dependencies = dependencies,
    .validate = validate_source,
    .produce = produce_source,
    .visit_associations = associations,
    .visit_diagnostics = diagnostics,
  };
  result.operations->constructed(
      result.self,
      {&operations, reinterpret_cast<tetrodotoxin_source_graph_self*>(state)});
}

Dialect::Dialect(Core::View::Bytes name) : name(name) {}
auto Dialect::get_name() const -> Core::View::Bytes {
  return name;
}

auto Dialect::get_provider() -> tetrodotoxin_dialect_provider {
  // Native installation owns the Dialect separately from its source graphs.
  // These borrowed provider operations require that installation to outlive
  // every graph it creates, just as a loaded foreign provider does.
  static const tetrodotoxin_dialect_provider_ops operations = {
    .header =
        {sizeof(tetrodotoxin_dialect_provider_ops), TTX_ABI_MAJOR,
         TTX_ABI_MINOR},
    .retain = [](tetrodotoxin_dialect_provider_self*) {},
    .release = [](tetrodotoxin_dialect_provider_self*) {},
    .candidate =
        [](tetrodotoxin_dialect_provider_self* self) {
          return reinterpret_cast<Dialect*>(self)->get_abi();
        },
    .name = [](tetrodotoxin_dialect_provider_self* self) -> ttx_borrowed_bytes {
      const auto name = reinterpret_cast<Dialect*>(self)->get_name();
      return {name.get_data(), name.get_size()};
    },
    .interpret = interpret_source,
  };
  return {
    &operations, reinterpret_cast<tetrodotoxin_dialect_provider_self*>(this)};
}

void Dialect::produce(
    const Monograph&,
    ttx_context,
    tetrodotoxin_production_result result) const {
  result.operations->none(result.self);
}

auto Dialect::negotiate(ttx_abstract requirement) const
    -> ttx_interface_relation {
  return ttx_abstract_same(requirement, tetrodotoxin_dialect_requirement())
             ? TTX_INTERFACE_SATISFIED
             : Abstract::negotiate(requirement);
}
