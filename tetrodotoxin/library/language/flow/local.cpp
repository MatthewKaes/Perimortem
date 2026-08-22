// Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/flow/local.hpp"

#include "perimortem/core/diagnostics/log.hpp"

#include "tetrodotoxin/library/language/diagnostics.hpp"
#include "tetrodotoxin/library/language/expressions/initializer.hpp"
#include "tetrodotoxin/library/language/model/parser/pack.hpp"
#include "tetrodotoxin/library/llvm/builder.hpp"
#include "ttx/concept/invalid.hpp"

using namespace Perimortem;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Ttx::Model;
using namespace Tetrodotoxin::Library;

auto Language::Flow::Local::interpret(Cursor& cursor, Block& host)
    -> Core::Option<Local&> {
  Memory::Allocator::Arena& domain = cursor.get_arena();
  Token evaluation = cursor.current();
  Writability writability = Writability::Full;
  switch (evaluation.get_code().get_type()) {
  case Code::Type::State:
    cursor.consume();
    break;
  case Code::Type::Const:
    cursor.consume();
    writability = Writability::Constant;
    break;
  default:
    cursor.create_token_error(
        "Library Local declarations require `state` or `const`."_view);
    return {};
  }

  Token name = cursor.require(
      Code::Type::Addressable,
      "Library Local declarations require one addressable name."_view);
  BAIL_IF(!name);
  BAIL_IF(!cursor.require(
      Code::Type::Define,
      "Library Local declarations require `:` after their name."_view));

  Core::Option<TypeReference> type_reference;
  Core::Option<Model::Pack&> initializer;
  if (cursor.matches(Code::Type::Assign)) {
    cursor.consume();
    if (Expressions::Initializer::is_next(cursor)) {
      auto object_initializer = Expressions::Initializer::parse(host, cursor);
      BAIL_IF(!object_initializer);
      initializer = *object_initializer;
    } else {
      initializer = Model::Parser::Pack::parse(host, cursor);
    }
    BAIL_IF(!initializer);
  } else {
    auto declared_type = TypeReference::parse(host, cursor);
    BAIL_IF(!declared_type);
    type_reference = *declared_type;

    if (cursor.matches(Code::Type::Assign)) {
      cursor.consume();
      if (Expressions::Initializer::is_next(cursor)) {
        auto object_initializer = Expressions::Initializer::parse(host, cursor);
        BAIL_IF(!object_initializer);
        initializer = *object_initializer;
      } else {
        initializer = Model::Parser::Pack::parse(host, cursor);
        BAIL_IF(!initializer);
      }
    }
  }

  if (writability == Writability::Constant && !initializer) {
    cursor.create_token_error(
        "Library const Locals require one compile-time initializer."_view);
    return {};
  }

  Token terminator = cursor.require(
      Code::Type::EndStatement,
      "Library Local declarations require one terminating `;`."_view);
  BAIL_IF(!terminator);

  Core::View::Bytes spelling = name.caculate_text(cursor.get_source_text());
  Anchor anchor = Anchor::create(name, Span(evaluation, terminator));
  Local& local = domain.construct_from<Local>([&]() -> Local {
    return Local(
        domain, host, name, spelling, writability, type_reference, initializer,
        anchor);
  });
  cursor.get_associations().create(anchor, local);
  return local;
}

auto Language::Flow::Local::link(
    Ttx::Lexical::Cursor& cursor,
    const Language::Model::Type& access_scope) -> Bool {
  if (type && initializer_linked) {
    return True;
  }

  if (type_reference) {
    auto selected = type_reference->resolve_authored(cursor, host);
    BAIL_IF(!selected);
    auto selected_type = selected->select<Language::Model::Type>();
    if (!selected_type) {
      cursor.create_expression_error(
          type_reference->get_anchor(),
          "Local Type route did not resolve to one stable Type."_view,
          "Publish the named Type before linking this Block."_view);
      return False;
    }
    if (selected_type->get_layout().is_empty()) {
      cursor.create_expression_error(
          type_reference->get_anchor(),
          "Local cannot bind an empty Type Layout."_view,
          "Choose a Type with one value leaf or remove the Local."_view);
      return False;
    }
    if (type && &type->get() != &*selected_type) {
      cursor.create_expression_error(
          type_reference->get_anchor(),
          "Local Type linking selected a different semantic identity."_view,
          "Repeat completion with the same resolved Type edge."_view);
      return False;
    }

    type = Reference<const Language::Model::Type>(*selected_type);
  }

  auto selected_initializer = initializer.visit(
      []() -> Core::Option<Model::Pack&> { return {}; },
      [](Model::Pack& selected) -> Core::Option<Model::Pack&> {
        return selected;
      });
  if (!selected_initializer) {
    if (!type) {
      cursor.create_expression_error(
          anchor, "Inferred Local requires one initializer Pack."_view,
          "Supply a value or name one explicit Type."_view);
      return False;
    }

    initializer_linked = True;
    return True;
  }

  // Block supplies the declarations that precede this Local and the enclosing
  // Function result contract used by flow operators. The Function host travels
  // separately so lexical shadowing never grants member access.
  BAIL_IF(!selected_initializer->link(cursor, host, access_scope));
  // Linking a Type name is valid when a later access consumes its identity.
  // Local is a value owner, so it proves real Pack flow before reading Layout.
  if (&selected_initializer->resolve() != &*selected_initializer) {
    cursor.create_expression_error(
        anchor, "Local initializer did not produce value flow."_view,
        "Use a Type result only as an access receiver."_view);
    return False;
  }

  if (!type) {
    if (selected_initializer->get_layout().get_size() != 1) {
      auto report = cursor.create_report(anchor);
      report << "Cannot infer Local '"_view << name << "' from "_view
             << selected_initializer->get_layout().get_size()
             << " initializer values.\nSource produces: "_view;
      Language::Diagnostics::write_pack(report, *selected_initializer);
      report.get_hint()
          << "Provide exactly one value or declare the Local's receiving Type."_view;
      return False;
    }

    const Abstract& output = selected_initializer->get_type();
    const Abstract& resolved =
        output.is<Language::Model::Type>() ? output : output.resolve();
    auto inferred_type = resolved.select<Language::Model::Type>();
    if (!inferred_type) {
      cursor.create_expression_error(
          anchor,
          "Inferred Local initializer did not complete one stable Type."_view,
          "Use an initializer with one exact scalar output Type."_view);
      return False;
    }
    if (inferred_type->get_layout().is_empty()) {
      cursor.create_expression_error(
          anchor, "Inferred Local cannot bind an empty Type Layout."_view,
          "Keep the empty result as flow or infer one value Type."_view);
      return False;
    }

    type = Reference<const Language::Model::Type>(*inferred_type);
    BAIL_IF(!link_constant(cursor));
    initializer_linked = True;
    return True;
  }

  if (!selected_initializer->fits_into(type->get())) {
    auto report = cursor.create_report(anchor);
    report << "Initializer for Local '"_view << name
           << "' does not fit declared Type '"_view << type->get().get_name()
           << "'.\nSource produces: "_view;
    Language::Diagnostics::write_pack(report, *selected_initializer);
    report << "\nTarget accepts: "_view;
    Language::Diagnostics::write_layout(report, type->get().get_layout());
    report.get_hint()
        << "Change the initializer or declare the exact Type it produces."_view;
    return False;
  }

  BAIL_IF(!link_constant(cursor));
  initializer_linked = True;
  return True;
}

auto Language::Flow::Local::resolve() const -> const Abstract& {
  if (!type || !initializer_linked) {
    return Invalid::get_invalid();
  }

  return *this;
}

auto Language::Flow::Local::resolve_access(
    const Abstract& access_host,
    Core::View::Bytes route) const -> const Abstract& {
  return type.visit(
      []() -> const Abstract& { return Invalid::get_invalid(); },
      [&](const Reference<const Language::Model::Type>& selected)
          -> const Abstract& {
        return selected.get().resolve_type_access(
            access_host, route, Language::Model::Type::Access::Self);
      });
}

auto Language::Flow::Local::resolve_call(
    const Abstract& access_host,
    Core::View::Bytes route) const -> const Abstract& {
  return type.visit(
      []() -> const Abstract& { return Invalid::get_invalid(); },
      [&](const Reference<const Language::Model::Type>& selected)
          -> const Abstract& {
        return selected.get().resolve_type_call(
            access_host, route, Language::Model::Type::Access::Self);
      });
}

auto Language::Flow::Local::get_documentation() const -> const Documentation& {
  for (const Language::Statement& statement : host.get_statements()) {
    if (&statement.get_root() == this) {
      return statement.get_documentation();
    }
  }

  return Documentation::get_empty();
}

auto Language::Flow::Local::finalize(Cursor& cursor) -> void {
  initializer.visit(
      []() {}, [&](Model::Pack& selected) { selected.finalize(cursor); });
}

auto Language::Flow::Local::lower(Llvm::Builder& body) const -> Bool {
  if (writability == Writability::Constant) {
    if (!body.has_full_debug()) {
      return True;
    }

    auto value = get_constant();
    return value && value->lower(body) &&
           body.constant_local(*this, *value, anchor);
  }

  Bool type_ready = get_type().reserve_value(body.get_program()) &&
                    get_type().complete_value(body.get_program());
  if (!type_ready) {
    return False;
  }

  Core::Option<const Model::Pack&> value = get_initializer();
  if (!value) {
    auto created = get_type().create_default(body.get_program().get_arena());
    if (!created) {
      return False;
    }

    value = *created;
  }

  Bool lowered = value->lower(body);
  if (!lowered) {
    Perimortem::Core::Diagnostics::Log::error(
        "Library LLVM lowering could not emit one Local initializer."_view);
    return False;
  }

  Bool bound = body.bind_local(*this, *value);
  if (!bound) {
    Perimortem::Core::Diagnostics::Log::error(
        "Library LLVM lowering could not bind one Local value."_view);
    return False;
  }

  return body.local(*this, anchor);
}

auto Language::Flow::Local::get_constant() const -> Core::Option<Model::Pack&> {
  if (writability != Writability::Constant || !cache_constant()) {
    return {};
  }

  return constant.visit(
      []() -> Core::Option<Model::Pack&> { return {}; },
      [](const Reference<Model::Pack>& selected) -> Core::Option<Model::Pack&> {
        return selected.get();
      });
}

auto Language::Flow::Local::link_constant(Cursor& cursor) const -> Bool {
  if (writability != Writability::Constant || cache_constant()) {
    return True;
  }

  cursor.create_expression_error(
      anchor,
      "Const Local initializer did not resolve to a compile-time value."_view,
      "Use only previously linked constant Expressions in a const Local."_view);
  return False;
}

auto Language::Flow::Local::cache_constant() const -> Bool {
  // A const Local may recurse through another const query while its Expression
  // folds. State records that cycle without manufacturing a partial value.
  if (constant_state == ConstantState::Folded) {
    return True;
  }
  if (constant_state == ConstantState::Folding ||
      constant_state == ConstantState::Failed) {
    return False;
  }

  constant_state = ConstantState::Folding;
  // Target owned fitting runs before ordinary folding because the receiving
  // Type may construct a value whose Layout differs from the authored source.
  auto fitted = initializer.visit(
      []() -> Core::Option<Model::Pack&> { return {}; },
      [&](Model::Pack& source) {
        return get_type().create_fitted(domain, source);
      });
  if (fitted) {
    constant = Reference<Model::Pack>(*fitted);
    constant_state = ConstantState::Folded;
    return True;
  }

  auto expression = initializer.visit(
      []() -> Core::Option<Expression&> { return {}; },
      [](Model::Pack& selected) { return selected.select<Expression>(); });
  if (!expression) {
    constant_state = ConstantState::Failed;
    return False;
  }

  expression->fold().visit(
      [&](const Core::Option<Model::Pack&>& folded) {
        // Dynamic absence may become constant after another declaration closes,
        // so it returns to Unresolved rather than poisoning future attempts.
        if (!folded) {
          constant_state = ConstantState::Unresolved;
          return;
        }
        constant = Reference<Model::Pack>(*folded);
        constant_state = ConstantState::Folded;
      },
      [&](const Expression::Error&) {
        constant_state = ConstantState::Failed;
      });
  return constant_state == ConstantState::Folded;
}
