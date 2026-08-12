// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/access/call.hpp"

#include "tetrodotoxin/library/language/types/composite.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/model/addressable.hpp"

using namespace Perimortem;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Ttx::Model;
using namespace Tetrodotoxin::Library;

static const Layouts::Fluid empty_results;

static auto select_type(const Abstract& candidate)
    -> Core::Option<const Ttx::Model::Type&> {
  auto direct = candidate.select<Ttx::Model::Type>();
  if (direct) {
    return *direct;
  }

  return candidate.resolve().select<Ttx::Model::Type>();
}

static auto select_result_type(const Abstract& result)
    -> Core::Option<const Ttx::Model::Type&> {
  auto addressable = result.select<Addressable>();
  if (addressable) {
    return select_type(addressable->get_type());
  }

  auto direct = result.select<Ttx::Model::Type>();
  if (direct) {
    return *direct;
  }

  const Abstract& resolved = result.resolve();
  addressable = resolved.select<Addressable>();
  return addressable ? select_type(addressable->get_type())
                     : select_type(resolved);
}

constexpr auto Language::Access::Call::ReceiverLayout::get_abstract(
    Count index) const -> Core::Option<const Abstract&> {
  BAIL_IF(index != 0);

  return receiver;
}

auto Language::Access::Call::ReceiverLayout::fits_at(
    const Layout& target,
    Count target_offset) const -> Bool {
  BAIL_IF(!has_target_segment(target, target_offset));

  return target.get_abstract(target_offset)
      .visit(
          []() { return False; },
          [&](const Abstract& parameter) {
            return select_result_type(parameter).visit(
                []() { return False; },
                [&](const Ttx::Model::Type& type) {
                  return receiver.fits(type);
                });
          });
}

auto Language::Access::Call::ReceiverLayout::get_fitted_at(
    const Layout& target,
    Count target_offset,
    Count target_index) const -> Utility::Result<const Abstract&, Errors> {
  if (target_index != 0) {
    return Errors::IndexOutOfBounds;
  }
  if (!has_target_segment(target, target_offset)) {
    return Errors::SizeMismatch;
  }
  if (!fits_at(target, target_offset)) {
    return Errors::IncompatibleFit;
  }

  return receiver;
}

auto Language::Access::Call::parse(
    Memory::Allocator::Arena& domain,
    Materializations& materializations,
    Cursor& cursor,
    const Abstract& source_context,
    Expression& receiver) -> Core::Option<Expression&> {
  auto transaction = cursor.branch();
  Token operation = transaction.require(
      Code::Type::CallOp,
      "Library invocation requires `->` before its Callable name."_view);
  BAIL_IF(!operation);

  Token name_token = transaction.require(
      Code::Type::Addressable,
      "Library invocation requires one Callable name after `->`."_view);
  BAIL_IF(!name_token);

  auto arguments = ArgumentPack::parse(
      domain, materializations, transaction, source_context);
  BAIL_IF(!arguments);

  auto receiver_anchor = receiver.get_anchor();
  if (!receiver_anchor) {
    transaction.create_expression_error(
        Anchor::create(
            name_token,
            Span(operation, arguments->get_anchor().get_span().get_end())),
        "Library invocation requires an authored receiver Anchor."_view);
    return {};
  }

  // The Token remains the canonical authored name fact. The Arena-stable
  // spelling exists only because Token coordinates need source bytes after the
  // parser Cursor has left this transaction.
  Core::View::Bytes name =
      domain.proxy(name_token.caculate_text(transaction.get_source_text()));
  Anchor anchor = Anchor::create(
      name_token, receiver_anchor->get_span(),
      arguments->get_anchor().get_span());
  Call& call = Expression::create_authored<Call>(
      domain, anchor, [&](Core::Option<Anchor> source) -> Call {
        return Call(receiver, name_token, name, *arguments, source);
      });
  cursor.join(transaction);
  return call;
}

auto Language::Access::Call::link(
    Tetrodotoxin::Language::Monograph& source,
    const Abstract& lexical_context,
    Materializations& materializations,
    Core::Option<const Ttx::Model::Type&> access_scope) -> Bool {
  // Every access first completes its receiver. Static and Self are outcomes of
  // that result, not parser modes or retained role flags.
  BAIL_IF(
      !receiver.link(source, lexical_context, materializations, access_scope));
  BAIL_IF(
      !arguments.link(source, lexical_context, materializations, access_scope));

  const Abstract& receiver_result = receiver.get_result();
  auto static_type = receiver_result.select<Ttx::Model::Type>();
  auto receiver_type =
      static_type ? static_type : select_type(receiver.get_type());
  auto target = receiver_type.visit(
      []() -> Core::Option<const Types::Composite&> { return {}; },
      [](const Ttx::Model::Type& type)
          -> Core::Option<const Types::Composite&> {
        return type.select<Types::Composite>();
      });
  if (!target) {
    source.report(
        get_anchor(),
        "Library invocation receiver did not produce one Composite Type."_view,
        "Invoke through a Type result for Static access or a typed value for "
        "Self access."_view);
    return False;
  }

  Tetrodotoxin::Language::Visibility visibility =
      access_scope && target->grants_private_access(*access_scope)
          ? Tetrodotoxin::Language::Visibility::Private
          : Tetrodotoxin::Language::Visibility::Public;

  Core::Option<const Callable&> selected;
  for (const Reference<Abstract>& binding : target->get_callables(visibility)) {
    // The inventory binding owns the local spelling. Alias is otherwise
    // opaque: only its ordinary resolution may reveal the registered Callable.
    if (binding.get().get_name() != name) {
      continue;
    }

    auto candidate = binding.get().resolve().select<Callable>();
    Bool admits_receiver =
        candidate && (static_type ? !candidate->is_type_bound()
                                  : candidate->is_type_bound(*target));
    if (admits_receiver) {
      selected = *candidate;
      break;
    }
  }

  if (!selected) {
    source.report(
        get_anchor(),
        "Library invocation did not find its registered Callable."_view,
        "Select an accessible Callable name with the receiver's Static or "
        "Self role."_view);
    return False;
  }

  const Layout& parameters = selected->get_parameters();
  Bool fits = static_type
                  ? arguments.fits(parameters)
                  : Bool(
                        parameters.get_size() == arguments.get_size() + 1 &&
                        arguments.fits_at(parameters, 1));
  if (!fits) {
    source.report(
        get_anchor(),
        "Library invocation arguments do not fit the registered Callable."_view,
        "Supply the exact positional or named parameter Layout."_view);
    return False;
  }

  if (callable && &callable->get() != &*selected) {
    source.report(
        get_anchor(),
        "Library invocation cannot change its Callable edge."_view,
        "Keep one exact Callable selected by this authored invocation."_view);
    return False;
  }

  callable = Reference<const Callable>(*selected);

  // Call deliberately does not delegate to Expression::link. Empty and
  // multi-result invocations are complete even though scalar get_type() is
  // Invalid; the selected Callable remains the exact result Layout owner.
  return True;
}

auto Language::Access::Call::get_documentation() const -> const Documentation& {
  return callable.visit(
      []() -> const Documentation& { return Documentation::get_empty(); },
      [](const Reference<const Callable>& selected) -> const Documentation& {
        return selected.get().get_documentation();
      });
}

auto Language::Access::Call::get_inputs() const -> const Layout& {
  // A Type-valued receiver participates in link selection but contributes no
  // runtime value. Self remains the leading evaluated input without storing a
  // second receiver-role fact.
  if (receiver.get_result().is<Ttx::Model::Type>()) {
    return arguments;
  }

  return self_inputs;
}

auto Language::Access::Call::get_results() const -> const Layout& {
  return callable.visit(
      []() -> const Layout& { return empty_results; },
      [](const Reference<const Callable>& selected) -> const Layout& {
        return selected.get().get_results();
      });
}

auto Language::Access::Call::get_type() const -> const Abstract& {
  const Layout& results = get_results();
  if (results.get_size() != 1) {
    return Invalid::get_invalid();
  }

  return results.get_abstract(0).visit(
      []() -> const Abstract& { return Invalid::get_invalid(); },
      [](const Abstract& result) -> const Abstract& {
        return select_result_type(result).visit(
            []() -> const Abstract& { return Invalid::get_invalid(); },
            [](const Ttx::Model::Type& type) -> const Abstract& {
              return type;
            });
      });
}

auto Language::Access::Call::get_callable() const
    -> Core::Option<const Callable&> {
  return callable.visit(
      []() -> Core::Option<const Callable&> { return {}; },
      [](const Reference<const Callable>& selected)
          -> Core::Option<const Callable&> { return selected.get(); });
}
