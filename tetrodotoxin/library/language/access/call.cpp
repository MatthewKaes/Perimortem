// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/access/call.hpp"

#include "tetrodotoxin/library/language/model/parser/pack.hpp"
#include "tetrodotoxin/library/language/types/composite.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/model/addressable.hpp"

using namespace Perimortem;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Ttx::Model;
using namespace Tetrodotoxin::Library;

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

// The Callable result Layout describes what one invocation produces, while
// this Layout preserves which invocation produced it. Fitting therefore
// delegates to the immutable signature shape, but a successful fitted query
// returns the Call rather than laundering value flow into a result Type. This
// is Call's canonical output Layout, not a shadow inventory: it borrows the
// Callable and retains no copied entries, names, or Types.
static auto create_layout(
    Memory::Allocator::Arena& domain,
    const Language::Access::Call& call,
    const Callable& callable) -> const Ttx::Concept::Layout& {
  class Layout final : public Ttx::Concept::Layout {
   public:
    constexpr Layout(
        const Language::Access::Call& call,
        const Callable& callable)
        : call(call), callable(callable) {}

    constexpr auto get_size() const -> Count override {
      return callable.get_results().get_size();
    }

    constexpr auto get_abstract(Count index) const
        -> Core::Option<const Abstract&> override {
      BAIL_IF(index >= get_size());
      return call;
    }

    constexpr auto get_name(Count index) const
        -> Core::Option<Core::View::Bytes> override {
      return callable.get_results().get_name(index);
    }

    auto fits_entry(
        const Ttx::Concept::Layout& target,
        Count source_index,
        Count target_index) const -> Bool override {
      return callable.get_results().fits_entry(
          target, source_index, target_index);
    }

    auto fits_at(const Ttx::Concept::Layout& target, Count target_offset) const
        -> Bool override {
      return callable.get_results().fits_at(target, target_offset);
    }

    auto get_fitted_at(
        const Ttx::Concept::Layout& target,
        Count target_offset,
        Count target_index) const
        -> Utility::Result<const Abstract&, Errors> override {
      if (target_index >= get_size()) {
        return Errors::IndexOutOfBounds;
      }
      if (!has_target_segment(target, target_offset)) {
        return Errors::SizeMismatch;
      }
      if (!fits_at(target, target_offset)) {
        return Errors::IncompatibleFit;
      }
      return call;
    }

   private:
    const Language::Access::Call& call;
    const Callable& callable;
  };

  return domain.construct<Layout>(call, callable);
}

// Self input reflection composes the real receiver with the authored argument
// Pack without creating another producer or copying either Layout. Static Calls
// need no composition because their Type receiver contributes no runtime value.
static auto create_inputs(
    Memory::Allocator::Arena& domain,
    const Language::Expression& receiver,
    const Language::Model::Pack& arguments) -> const Ttx::Concept::Layout& {
  class Inputs final : public Ttx::Concept::Layout {
   public:
    constexpr Inputs(
        const Language::Expression& receiver,
        const Language::Model::Pack& arguments)
        : receiver(receiver), arguments(arguments) {}

    auto get_size() const -> Count override {
      return 1 + arguments.get_layout().get_size();
    }

    auto get_abstract(Count index) const
        -> Core::Option<const Abstract&> override {
      if (index == 0) {
        return receiver;
      }
      return arguments.get_layout().get_abstract(index - 1);
    }

    auto get_name(Count index) const
        -> Core::Option<Core::View::Bytes> override {
      return index == 0 ? Core::Option<Core::View::Bytes>()
                        : arguments.get_layout().get_name(index - 1);
    }

    auto fits_entry(
        const Ttx::Concept::Layout& target,
        Count source,
        Count target_index) const -> Bool override {
      BAIL_IF(source >= get_size() || target_index >= target.get_size());
      if (source != 0) {
        return arguments.fits_entry(target, source - 1, target_index);
      }

      auto target_entry = target.get_abstract(target_index);
      BAIL_IF(!target_entry);
      return select_result_type(*target_entry)
          .visit(
              []() { return False; },
              [&](const Ttx::Model::Type& type) {
                return receiver.fits(type);
              });
    }

    auto fits_at(const Ttx::Concept::Layout& target, Count target_offset) const
        -> Bool override {
      BAIL_IF(
          target_offset > target.get_size() ||
          get_size() > target.get_size() - target_offset);
      return fits_entry(target, 0, target_offset) &&
             arguments.fits_at(target, target_offset + 1);
    }

    auto get_fitted_at(
        const Ttx::Concept::Layout& target,
        Count target_offset,
        Count target_index) const
        -> Utility::Result<const Abstract&, Errors> override {
      if (target_index >= get_size()) {
        return Errors::IndexOutOfBounds;
      }
      if (target_offset > target.get_size() ||
          get_size() > target.get_size() - target_offset) {
        return Errors::SizeMismatch;
      }
      if (!fits_at(target, target_offset)) {
        return Errors::IncompatibleFit;
      }
      return target_index == 0
                 ? Utility::Result<const Abstract&, Errors>(receiver)
                 : arguments.get_fitted_at(
                       target, target_offset + 1, target_index - 1);
    }

   private:
    const Language::Expression& receiver;
    const Language::Model::Pack& arguments;
  };

  return domain.construct<Inputs>(receiver, arguments);
}

auto Language::Access::Call::parse(
    Memory::Allocator::Arena& domain,
    Language::Monograph& source,
    Cursor& cursor,
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

  Token arguments_opening = transaction.current();
  auto arguments =
      Language::Model::Parser::Pack::parse(domain, source, transaction, True);
  BAIL_IF(!arguments);
  Token closing = transaction.peek(-1);

  auto receiver_anchor = receiver.get_anchor();
  if (!receiver_anchor) {
    transaction.create_expression_error(
        Anchor::create(name_token, Span(operation, closing)),
        "Library invocation requires an authored receiver Anchor."_view);
    return {};
  }

  // The Token remains the canonical authored name fact. The spelling copied
  // into the Arena supports delayed lookup because Token coordinates require
  // source bytes after the parser Cursor leaves this transaction.
  Core::View::Bytes name =
      domain.proxy(name_token.caculate_text(transaction.get_source_text()));
  Anchor anchor = Anchor::create(
      name_token, receiver_anchor->get_span(),
      Span(arguments_opening, closing));
  Call& call = Expression::create_authored<Call>(
      domain, anchor, [&](Core::Option<Anchor> source) -> Call {
        return Call(domain, receiver, name_token, name, *arguments, source);
      });
  cursor.join(transaction);
  return call;
}

auto Language::Access::Call::link(
    Tetrodotoxin::Language::Monograph& source,
    const Abstract& lexical_context,
    Core::Option<const Ttx::Model::Type&> access_scope) -> Bool {
  // Every access first completes its receiver. Static and Self are outcomes of
  // that result, not parser modes or retained role flags.
  BAIL_IF(!receiver.link(source, lexical_context, access_scope));
  BAIL_IF(!arguments.link(source, lexical_context, access_scope));

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

  const Ttx::Concept::Layout& parameters = selected->get_parameters();
  Bool arguments_fit = arguments.fits(parameters);
  if (!static_type) {
    if (!inputs) {
      inputs = create_inputs(domain, receiver, arguments);
    }
    arguments_fit = inputs->fits(parameters);
  }
  if (!arguments_fit) {
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

  if (callable) {
    // Re-linking may revalidate the surrounding graph, but this invocation's
    // successfully published producer Layout remains the original object.
    return True;
  }

  const Ttx::Concept::Layout& retained_output =
      create_layout(domain, *this, *selected);
  callable = Reference<const Callable>(*selected);
  output = retained_output;

  // Call deliberately does not delegate to Expression::link. Invocations with
  // empty or multiple result Layouts are complete even though scalar get_type()
  // is Invalid. The selected Callable remains the exact result Layout owner.
  return True;
}

auto Language::Access::Call::get_documentation() const -> const Documentation& {
  return callable.visit(
      []() -> const Documentation& { return Documentation::get_empty(); },
      [](const Reference<const Callable>& selected) -> const Documentation& {
        return selected.get().get_documentation();
      });
}

auto Language::Access::Call::get_type() const -> const Abstract& {
  if (!callable) {
    return Invalid::get_invalid();
  }
  const Ttx::Concept::Layout& results = callable->get().get_results();
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

auto Language::Access::Call::get_layout() const -> const Ttx::Concept::Layout& {
  return *output;
}

auto Language::Access::Call::resolve() const -> const Abstract& {
  if (!callable || !output) {
    return Invalid::get_invalid();
  }

  return static_cast<const Language::Model::Pack&>(*this);
}

auto Language::Access::Call::finalize() -> void {
  // Receiver and argument Pack are the complete evaluation inputs owned by
  // this invocation. A Call retained directly by a Block is the effect itself;
  // discarded result flow must not turn that effectful Call into a fold
  // request.
  receiver.finalize();
  arguments.finalize();
}

auto Language::Access::Call::get_callable() const
    -> Core::Option<const Callable&> {
  return callable.visit(
      []() -> Core::Option<const Callable&> { return {}; },
      [](const Reference<const Callable>& selected)
          -> Core::Option<const Callable&> { return selected.get(); });
}
