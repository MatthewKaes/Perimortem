// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/flow/local.hpp"

#include "tetrodotoxin/library/language/expressions/initializer.hpp"
#include "tetrodotoxin/library/language/model/parser/pack.hpp"
#include "tetrodotoxin/library/language/types/composite.hpp"
#include "ttx/concept/invalid.hpp"

using namespace Perimortem;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Ttx::Model;
using namespace Tetrodotoxin::Library;

auto Language::Flow::Local::interpret(
    Memory::Allocator::Arena& domain,
    Monograph& source,
    Cursor& cursor,
    Block& host) -> Core::Option<Local&> {
  auto transaction = cursor.branch();
  Token evaluation = transaction.current();
  Writability writability = Writability::Full;
  switch (evaluation.get_code().get_type()) {
  case Code::Type::State:
    transaction.consume();
    break;
  case Code::Type::Const:
    transaction.consume();
    writability = Writability::Constant;
    break;
  default:
    return {};
  }

  Token name = transaction.require(
      Code::Type::Addressable,
      "Library Local declarations require one addressable name."_view);
  BAIL_IF(!name);
  BAIL_IF(!transaction.require(
      Code::Type::Define,
      "Library Local declarations require `:` after their name."_view));

  Core::Option<TypeReference> type_reference;
  Core::Option<Model::Pack&> initializer;
  if (transaction.matches(Code::Type::Assign)) {
    transaction.consume();
    if (Expressions::Initializer::is_next(transaction)) {
      transaction.create_token_error(
          "An inferred Library Local cannot use `new`."_view,
          "Name one exact Object Type before initialization begins."_view);
      return {};
    }

    initializer = Model::Parser::Pack::parse(domain, source, transaction);
    BAIL_IF(!initializer);
  } else {
    auto declared_type = TypeReference::parse(source, transaction);
    BAIL_IF(!declared_type);
    type_reference = *declared_type;

    if (transaction.matches(Code::Type::Assign)) {
      transaction.consume();
      if (Expressions::Initializer::is_next(transaction)) {
        auto object_initializer =
            Expressions::Initializer::parse(domain, source, transaction);
        BAIL_IF(!object_initializer);
        initializer = *object_initializer;
      } else {
        initializer = Model::Parser::Pack::parse(domain, source, transaction);
        BAIL_IF(!initializer);
      }
    }
  }

  if (writability == Writability::Constant && !initializer) {
    transaction.create_token_error(
        "Library const Locals require one compile-time initializer."_view);
    return {};
  }

  Token terminator = transaction.require(
      Code::Type::EndStatement,
      "Library Local declarations require one terminating `;`."_view);
  BAIL_IF(!terminator);

  Core::View::Bytes spelling =
      domain.proxy(name.caculate_text(transaction.get_source_text()));
  Anchor anchor = Anchor::create(name, Span(evaluation, terminator));
  Local& local = domain.construct_from<Local>([&]() -> Local {
    return Local(
        host, name, spelling, writability, type_reference, initializer, anchor);
  });
  cursor.join(transaction);
  return local;
}

auto Language::Flow::Local::link(
    Tetrodotoxin::Language::Monograph& source,
    const Type& access_scope) -> Bool {
  if (type && initializer_linked) {
    return True;
  }

  if (type_reference) {
    auto composite = access_scope.select<Language::Types::Composite>();
    if (!composite) {
      source.report(
          anchor,
          "Explicit Local linking requires one Composite access scope."_view,
          "Retain the Function host Type while linking its Block."_view);
      return False;
    }

    const Abstract& selected = composite->resolve_type(*type_reference);
    const Abstract& resolved =
        selected.is<Type>() ? selected : selected.resolve();
    auto selected_type = resolved.select<Type>();
    if (!selected_type) {
      source.report(
          type_reference->get_anchor(),
          "Local Type route did not resolve to one stable Type."_view,
          "Publish the named Type before linking this Block."_view);
      return False;
    }
    if (selected_type->get_layout().is_empty()) {
      source.report(
          type_reference->get_anchor(),
          "Local cannot bind an empty Type Layout."_view,
          "Keep the empty Type as flow or choose a Type with one value leaf."_view);
      return False;
    }
    if (type && &type->get() != &*selected_type) {
      source.report(
          type_reference->get_anchor(),
          "Local Type linking selected a different semantic identity."_view,
          "Repeat completion with the same resolved Type edge."_view);
      return False;
    }

    type = Reference<const Type>(*selected_type);
  }

  auto selected_initializer = initializer.visit(
      []() -> Core::Option<Model::Pack&> { return {}; },
      [](Model::Pack& selected) -> Core::Option<Model::Pack&> {
        return selected;
      });
  if (!selected_initializer) {
    if (!type) {
      source.report(
          anchor, "Inferred Local requires one initializer Pack."_view,
          "Supply a value or name one explicit Type."_view);
      return False;
    }

    initializer_linked = True;
    return True;
  }

  // Local stays the receiving Addressable for typed construction while Block
  // supplies only declarations that precede this statement. The Function host
  // travels separately so lexical shadowing never grants member access.
  BAIL_IF(!selected_initializer->link(source, *this, access_scope));

  if (!type) {
    if (selected_initializer->get_layout().get_size() != 1) {
      source.report(
          anchor,
          "Inferred Local initializer must produce exactly one value."_view,
          "Name an explicit receiving Type for empty or multi-value flow."_view);
      return False;
    }

    const Abstract& output = selected_initializer->get_type();
    const Abstract& resolved = output.is<Type>() ? output : output.resolve();
    auto inferred_type = resolved.select<Type>();
    if (!inferred_type) {
      source.report(
          anchor,
          "Inferred Local initializer did not complete one stable Type."_view,
          "Use an initializer with one exact scalar output Type."_view);
      return False;
    }
    if (inferred_type->get_layout().is_empty()) {
      source.report(
          anchor, "Inferred Local cannot bind an empty Type Layout."_view,
          "Keep the empty result as flow or infer one value Type."_view);
      return False;
    }

    type = Reference<const Type>(*inferred_type);
    BAIL_IF(!link_constant(source));
    initializer_linked = True;
    return True;
  }

  if (!selected_initializer->fits(type->get())) {
    source.report(
        anchor,
        "Local initializer Pack does not fit the declared Type Layout."_view,
        "Supply the complete value flow accepted by the declared Local "
        "Type."_view);
    return False;
  }

  BAIL_IF(!link_constant(source));
  initializer_linked = True;
  return True;
}

auto Language::Flow::Local::resolve() const -> const Abstract& {
  if (!type || !initializer_linked) {
    return Invalid::get_invalid();
  }

  return *this;
}

auto Language::Flow::Local::finalize() -> void {
  initializer.visit(
      []() {}, [](Model::Pack& selected) { selected.finalize(); });
}

auto Language::Flow::Local::resolve_context(Core::View::Bytes route) const
    -> const Abstract& {
  return host.resolve_context(route);
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

auto Language::Flow::Local::link_constant(
    Tetrodotoxin::Language::Monograph& source) const -> Bool {
  if (writability != Writability::Constant || cache_constant()) {
    return True;
  }

  source.report(
      anchor,
      "Const Local initializer did not resolve to a compile-time value."_view,
      "Use only previously linked constant Expressions in a const Local."_view);
  return False;
}

auto Language::Flow::Local::cache_constant() const -> Bool {
  if (constant_state == ConstantState::Folded) {
    return True;
  }
  if (constant_state == ConstantState::Folding ||
      constant_state == ConstantState::Failed) {
    return False;
  }

  constant_state = ConstantState::Folding;
  auto expression = initializer.visit(
      []() -> Core::Option<Expression&> { return {}; },
      [](Model::Pack& selected) { return selected.select<Expression>(); });
  if (!expression) {
    constant_state = ConstantState::Failed;
    return False;
  }

  expression->fold().visit(
      [&](const Core::Option<Model::Pack&>& folded) {
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
