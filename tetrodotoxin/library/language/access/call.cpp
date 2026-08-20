// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/access/call.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/library/language/diagnostics.hpp"
#include "tetrodotoxin/library/language/model/parser/pack.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "tetrodotoxin/library/llvm/builder.hpp"
#include "ttx/concept/invalid.hpp"

using namespace Perimortem;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Ttx::Model;
using namespace Tetrodotoxin::Library;

static auto select_type(const Abstract& candidate)
    -> Core::Option<const Language::Model::Type&> {
  auto direct = candidate.select<Language::Model::Type>();
  if (direct) {
    return *direct;
  }

  return candidate.resolve().select<Language::Model::Type>();
}

auto Language::Access::Call::create_synthetic(
    Memory::Allocator::Arena& arena,
    Expression& receiver,
    Core::View::Bytes name,
    Language::Model::Pack& arguments) -> Call& {
  return Expression::create_synthetic<Call>(arena, [&](auto source) -> Call {
    return Call(arena, receiver, {}, name, arguments, source);
  });
}

static auto select_result_type(const Abstract& result)
    -> Core::Option<const Language::Model::Type&> {
  auto addressable = result.select<Language::Model::Addressable>();
  if (addressable) {
    return select_type(addressable->get_type());
  }

  auto direct = result.select<Language::Model::Type>();
  if (direct) {
    return *direct;
  }

  const Abstract& resolved = result.resolve();
  addressable = resolved.select<Language::Model::Addressable>();
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
    const Language::Model::Callable& callable) -> const Ttx::Concept::Layout& {
  class Layout final : public Ttx::Concept::Layout {
   public:
    constexpr Layout(
        const Language::Access::Call& call,
        const Language::Model::Callable& callable)
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
    const Language::Model::Callable& callable;
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
              [&](const Language::Model::Type& type) {
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
    const Abstract& context,
    Cursor& cursor,
    Expression& receiver) -> Core::Option<Expression&> {
  Memory::Allocator::Arena& domain = cursor.get_arena();
  Token operation = cursor.require(
      Code::Type::CallOp,
      "Library invocation requires `->` before its Callable name."_view);
  BAIL_IF(!operation);

  Token name_token = cursor.require(
      Code::Type::Addressable,
      "Library invocation requires one Callable name after `->`."_view);
  BAIL_IF(!name_token);

  Token arguments_opening = cursor.current();
  auto arguments = Language::Model::Parser::Pack::parse(context, cursor, True);
  BAIL_IF(!arguments);
  Token closing = cursor.peek(-1);

  auto receiver_anchor = receiver.get_anchor();
  if (!receiver_anchor) {
    cursor.create_expression_error(
        Anchor::create(name_token, Span(operation, closing)),
        "Library invocation requires an authored receiver Anchor."_view);
    return {};
  }

  // The Token and its source spelling share the retained source lifetime,
  // so delayed lookup can borrow the authored bytes directly.
  Core::View::Bytes name = name_token.caculate_text(cursor.get_source_text());
  Anchor anchor = Anchor::create(
      name_token, receiver_anchor->get_span(),
      Span(arguments_opening, closing));
  Call& call = Expression::create_authored<Call>(
      domain, anchor, [&](Core::Option<Anchor> source) -> Call {
        return Call(domain, receiver, name_token, name, *arguments, source);
      });
  return call;
}

auto Language::Access::Call::link(
    Ttx::Lexical::Cursor& cursor,
    const Abstract& lexical_context,
    Core::Option<const Abstract&> access_scope) -> Bool {
  if (!get_anchor() && callable && output) {
    return True;
  }

  // Every access first completes its receiver. Static and Self are outcomes of
  // that result, not parser modes or retained role flags.
  BAIL_IF(!receiver.link(cursor, lexical_context, access_scope));
  BAIL_IF(!arguments.link(cursor, lexical_context, access_scope));
  // A Type receiver proves Static lookup but argument positions are value
  // flow. Preserve that split before Callable fitting reads the Pack Layout.
  if (&arguments.resolve() != &arguments) {
    cursor.create_expression_error(
        get_anchor(),
        "Library invocation arguments did not produce value flow."_view,
        "Use Type results only as access receivers."_view);
    return False;
  }

  const Abstract& receiver_result = receiver.get_result();
  const Abstract& host = access_scope.visit(
      [&]() -> const Abstract& { return lexical_context; },
      [](const Abstract& selected) -> const Abstract& { return selected; });
  const Abstract& candidate = receiver_result.visit<Language::Model::Type>(
      [&](const Language::Model::Type& type) -> const Abstract& {
        return type.resolve_type_call(
            host, name, Language::Model::Type::Access::Static);
      },
      [&](const Abstract& receiver) -> const Abstract& {
        return receiver.resolve_call(host, name);
      });
  auto selected = candidate.resolve().select<Language::Model::Callable>();
  if (!selected) {
    auto report = cursor.create_report(get_anchor());
    report << "Receiver '"_view << receiver_result.get_name()
           << "' has no accessible Callable named '"_view << name << "'. "_view
           << "Receiver type: "_view;
    Language::Diagnostics::write_type(report, receiver_result);
    report << "."_view;
    report.get_hint()
        << "Correct the Callable spelling or invoke it through the required "
           "Static or Self receiver."_view;
    return False;
  }

  if (!selected->accepts_receiver(receiver_result, host)) {
    auto report = cursor.create_report(get_anchor());
    report
        << "Callable '"_view << selected->get_name()
        << "' cannot use receiver '"_view << receiver_result.get_name()
        << "' because its required storage or authority is unavailable."_view;
    report.get_hint()
        << "Invoke through an Addressable whose lifetime and write authority "
           "satisfy this Callable."_view;
    return False;
  }

  const Ttx::Concept::Layout& parameters = selected->get_parameters();
  Bool arguments_fit = arguments.fits(parameters);
  if (selected->is_type_bound()) {
    if (!input_layout) {
      input_layout = create_inputs(domain, receiver, arguments);
    }
    arguments_fit = input_layout->fits(parameters);
  }

  if (!arguments_fit) {
    auto report = cursor.create_report(get_anchor());
    report << "Arguments do not fit Callable '"_view << selected->get_name()
           << "'.\nSource produces: "_view;
    if (selected->is_type_bound()) {
      Language::Diagnostics::write_layout(report, *input_layout);
    } else {
      Language::Diagnostics::write_pack(report, arguments);
    }
    report << "\nParameters accept: "_view;
    Language::Diagnostics::write_layout(report, parameters);
    report.get_hint()
        << "Supply the exact positional or named parameter Types shown above."_view;
    return False;
  }

  if (callable && &callable->get() != &*selected) {
    auto report = cursor.create_report(get_anchor());
    report << "Internal semantic error: invocation '"_view << name
           << "' changed Callable identity from '"_view
           << callable->get().get_name() << "' to '"_view
           << selected->get_name() << "'."_view;
    report.get_hint()
        << "The source is valid; report this unstable linking result."_view;
    return False;
  }

  if (callable) {
    // Repeated linking may revalidate the surrounding graph, but this
    // invocation keeps its successfully published producer Layout.
    return True;
  }

  fitted_inputs.clear();
  if (!fit_inputs(*selected, input_layout)) {
    cursor.create_expression_error(
        get_anchor(),
        "Internal semantic error: Callable fitting did not retain one stable "
        "input mapping."_view,
        "The source is valid; report this invocation fitting failure."_view);
    return False;
  }

  const Ttx::Concept::Layout& retained_output =
      create_layout(domain, *this, *selected);
  callable = Reference<const Language::Model::Callable>(*selected);
  output = retained_output;

  // Call deliberately does not delegate to Expression::link. Invocations with
  // empty or multiple result Layouts are complete even though scalar get_type()
  // is Invalid. The selected Callable remains the exact result Layout owner.
  return True;
}

auto Language::Access::Call::link_restored(
    const Abstract& lexical_context,
    Core::Option<const Abstract&> access_scope) -> Bool {
  BAIL_IF(
      !receiver.link_restored(lexical_context, access_scope) ||
      !arguments.link_restored(lexical_context, access_scope) ||
      &arguments.resolve() != &arguments);

  const Abstract& receiver_result = receiver.get_result();
  const Abstract& host = access_scope.visit(
      [&]() -> const Abstract& { return lexical_context; },
      [](const Abstract& selected) -> const Abstract& { return selected; });
  const Abstract& candidate = receiver_result.visit<Language::Model::Type>(
      [&](const Language::Model::Type& type) -> const Abstract& {
        return type.resolve_type_call(
            host, name, Language::Model::Type::Access::Static);
      },
      [&](const Abstract& selected) -> const Abstract& {
        return selected.resolve_call(host, name);
      });
  auto selected = candidate.resolve().select<Language::Model::Callable>();
  BAIL_IF(!selected || !selected->accepts_receiver(receiver_result, host));

  const Layout& parameters = selected->get_parameters();
  Bool fits = arguments.fits(parameters);
  if (selected->is_type_bound()) {
    input_layout = create_inputs(domain, receiver, arguments);
    fits = input_layout->fits(parameters);
  }
  BAIL_IF(!fits || !fit_inputs(*selected, input_layout));

  callable = Reference<const Language::Model::Callable>(*selected);
  output = create_layout(domain, *this, *selected);
  return True;
}

auto Language::Access::Call::get_documentation() const -> const Documentation& {
  return callable.visit(
      []() -> const Documentation& { return Documentation::get_empty(); },
      [](const Reference<const Language::Model::Callable>& selected)
          -> const Documentation& {
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
            [](const Language::Model::Type& type) -> const Abstract& {
              return type;
            });
      });
}

auto Language::Access::Call::get_value_type(Count index) const
    -> const Abstract& {
  if (!callable) {
    return Invalid::get_invalid();
  }
  const Ttx::Concept::Layout& results = callable->get().get_results();
  auto result = results.get_abstract(index);
  if (!result) {
    return Invalid::get_invalid();
  }
  return select_result_type(*result).visit(
      []() -> const Abstract& { return Invalid::get_invalid(); },
      [](const Language::Model::Type& type) -> const Abstract& {
        return type;
      });
}

auto Language::Access::Call::get_produced(Count index) const
    -> Core::Option<Ttx::Model::Pack::Produced> {
  if (!output || index >= output->get_size() || &resolve() != this) {
    return {};
  }

  // Every result belongs to this invocation even though its immutable
  // signature Layout supplies the descriptor shape.
  return Ttx::Model::Pack::Produced{*this, index};
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

auto Language::Access::Call::finalize(Cursor& cursor) -> void {
  // Receiver and argument Pack are the complete evaluation inputs owned by
  // this invocation. A Call retained directly by a Block is the effect itself.
  // Discarded result flow must not turn that effectful Call into a fold
  // request.
  receiver.finalize(cursor);
  arguments.finalize(cursor);
}

static auto select_parameter(
    const Language::Model::Callable& callable,
    Count index) -> Core::Option<const Ttx::Model::Addressable&> {
  auto entry = callable.get_parameters().get_abstract(index);
  return entry ? entry->select<Ttx::Model::Addressable>()
               : Core::Option<const Ttx::Model::Addressable&>();
}

auto Language::Access::Call::fit_inputs(
    const Language::Model::Callable& callable,
    Core::Option<const Ttx::Concept::Layout&> input_layout) -> Bool {
  const Ttx::Concept::Layout& parameters = callable.get_parameters();
  Count receiver_offset = callable.declares_self() ? 1 : 0;
  Count source_size = receiver_offset + arguments.get_layout().get_size();
  if (source_size == parameters.get_size()) {
    const Ttx::Concept::Layout& source =
        input_layout ? *input_layout : arguments.get_layout();
    for (Count target_index = 0; target_index < parameters.get_size();
         target_index++) {
      Count selected = 0;
      Count matches = 0;
      for (Count source_index = 0; source_index < source_size; source_index++) {
        if (!source.get_name(source_index) && source_index != target_index) {
          continue;
        }

        Bool fits = input_layout ? input_layout->fits_entry(
                                       parameters, source_index, target_index)
                                 : arguments.fits_entry(
                                       parameters, source_index, target_index);
        if (fits) {
          selected = source_index;
          matches++;
        }
      }

      auto parameter = select_parameter(callable, target_index);
      BAIL_IF(matches != 1 || !parameter);
      auto produced = selected < receiver_offset
                          ? receiver.get_produced(0)
                          : arguments.get_produced(selected - receiver_offset);
      BAIL_IF(!produced);
      fitted_inputs.insert(
          Input(*parameter, produced->producer, produced->local_index, 1));
    }

    return True;
  }

  BAIL_IF(parameters.get_size() != receiver_offset + 1);
  if (receiver_offset != 0) {
    auto parameter = select_parameter(callable, 0);
    auto produced = receiver.get_produced(0);
    BAIL_IF(!parameter || !produced);
    fitted_inputs.insert(
        Input(*parameter, produced->producer, produced->local_index, 1));
  }

  auto parameter = select_parameter(callable, receiver_offset);
  BAIL_IF(!parameter);
  fitted_inputs.insert(
      Input(*parameter, arguments, 0, arguments.get_layout().get_size()));
  return True;
}

auto Language::Access::Call::lower(Llvm::Builder& body) const -> Bool {
  auto selected = get_callable();
  if (!selected) {
    return False;
  }

  Llvm::Program& program = body.get_program();
  if (!selected->reserve_declaration(program) ||
      !selected->complete_declaration(program)) {
    return False;
  }

  if (selected->declares_self()) {
    Bool receiver_lowered = receiver.lower(body);
    if (!receiver_lowered) {
      return False;
    }
  }

  Bool arguments_lowered = arguments.lower(body);
  if (!arguments_lowered) {
    return False;
  }

  Memory::Managed::Vector<LLVMValueRef> native_inputs(domain);
  for (const Input& input : fitted_inputs.get_view()) {
    auto value = body.fit_input(
        input.get_parameter(), input.get_source(), input.get_offset(),
        input.get_size());
    if (!value) {
      return False;
    }

    native_inputs.insert(*value);
  }

  Core::Option<const Ttx::Model::Pack&> receiver_source;
  if (selected->declares_self() && !fitted_inputs.is_empty()) {
    const Input& input = fitted_inputs.at(0);
    if (input.get_offset() == 0 && input.get_size() == 1) {
      receiver_source = input.get_source();
    }
  }

  return selected->lower_call(
      body, *this, native_inputs.get_view(), receiver_source);
}

auto Language::Access::Call::evaluate()
    -> Utility::Result<Core::Option<Language::Model::Pack&>, Error> {
  auto selected = get_callable();
  if (!selected) {
    return Core::Option<Language::Model::Pack&>();
  }

  Core::Option<const Language::Model::Pack&> folded_receiver;
  if (!selected->declares_self()) {
    return selected->fold_call(domain, folded_receiver, arguments);
  }

  return receiver.fold().visit(
      [&](const Core::Option<Language::Model::Pack&>& value)
          -> Utility::Result<Core::Option<Language::Model::Pack&>, Error> {
        if (!value) {
          return Core::Option<Language::Model::Pack&>();
        }

        folded_receiver = *value;
        return selected->fold_call(domain, folded_receiver, arguments);
      },
      [](const Error& error)
          -> Utility::Result<Core::Option<Language::Model::Pack&>, Error> {
        return error;
      });
}

auto Language::Access::Call::get_callable() const
    -> Core::Option<const Language::Model::Callable&> {
  return callable.visit(
      []() -> Core::Option<const Language::Model::Callable&> { return {}; },
      [](const Reference<const Language::Model::Callable>& selected)
          -> Core::Option<const Language::Model::Callable&> {
        return selected.get();
      });
}
