// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/construction.hpp"

#include "perimortem/core/diagnostics/log.hpp"

#include "tetrodotoxin/library/language/access/call.hpp"
#include "tetrodotoxin/library/language/expressions/identifier.hpp"
#include "tetrodotoxin/library/language/expressions/initializer.hpp"
#include "tetrodotoxin/library/language/field.hpp"
#include "tetrodotoxin/library/language/generic.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "tetrodotoxin/library/language/types/option.hpp"
#include "tetrodotoxin/library/llvm/body.hpp"
#include "tetrodotoxin/library/llvm/builder.hpp"
#include "tetrodotoxin/library/llvm/functions.hpp"
#include "ttx/concept/invalid.hpp"

using namespace Perimortem;
using namespace Ttx::Concept;
using namespace Tetrodotoxin::Library;

class ConstructionInput final : public Language::Expression {
 public:
  TTX_CONTRACT(ConstructionInput, Language::Expression);

  static auto create(
      Memory::Allocator::Arena& arena,
      Language::Expressions::Identifier& option,
      const Language::Types::Option& option_type,
      const Language::Model::Type& element,
      Language::Model::Pack& fallback) -> ConstructionInput& {
    return Expression::create_synthetic<ConstructionInput>(
        arena, [&](auto anchor) -> ConstructionInput {
          return ConstructionInput(
              option, option_type, element, fallback, anchor);
        });
  }

  TTX_NAME("ConstructionInput"_view);
  TTX_EMPTY_DOCUMENTATION();
  TTX_INVALID_CONTEXT;

  constexpr auto get_type() const -> const Abstract& override {
    return element.get();
  }

  auto lower(Llvm::Builder& body) const -> Bool override {
    BAIL_IF(!option.get().lower(body));
    auto selected =
        body.begin_unwrap(option_type.get(), element.get(), option.get());
    BAIL_IF(!selected || !fallback.get().lower(body));
    return body.end_unwrap(*selected, element.get(), *this, fallback.get());
  }

 private:
  constexpr ConstructionInput(
      Language::Expressions::Identifier& option,
      const Language::Types::Option& option_type,
      const Language::Model::Type& element,
      Language::Model::Pack& fallback,
      Core::Option<Ttx::Lexical::Anchor> anchor)
      : Expression(anchor),
        option(option),
        option_type(option_type),
        element(element),
        fallback(fallback) {}

  Reference<Language::Expressions::Identifier> option;
  Reference<const Language::Types::Option> option_type;
  Reference<const Language::Model::Type> element;
  Reference<Language::Model::Pack> fallback;
};

class ConstructionOption final : public Language::Expression {
 public:
  TTX_CONTRACT(ConstructionOption, Language::Expression);

  static auto create(
      Memory::Allocator::Arena& arena,
      const Language::Types::Option& type,
      Core::Option<Language::Model::Pack&> payload) -> ConstructionOption& {
    Core::Option<Reference<Language::Model::Pack>> retained;
    if (payload) {
      retained = Reference<Language::Model::Pack>(*payload);
    }
    return Expression::create_synthetic<ConstructionOption>(
        arena, [&](auto anchor) -> ConstructionOption {
          return ConstructionOption(type, retained, anchor);
        });
  }

  TTX_NAME("ConstructionOption"_view);
  TTX_EMPTY_DOCUMENTATION();
  TTX_INVALID_CONTEXT;

  constexpr auto get_type() const -> const Abstract& override {
    return type.get();
  }

  auto lower(Llvm::Builder& body) const -> Bool override {
    if (!payload) {
      return body.absent(type.get(), *this);
    }

    return payload->get().lower(body) &&
           body.present(
               type.get(), type.get().get_element_type(), *this,
               payload->get());
  }

 private:
  constexpr ConstructionOption(
      const Language::Types::Option& type,
      Core::Option<Reference<Language::Model::Pack>> payload,
      Core::Option<Ttx::Lexical::Anchor> anchor)
      : Expression(anchor), type(type), payload(payload) {}

  Reference<const Language::Types::Option> type;
  Core::Option<Reference<Language::Model::Pack>> payload;
};

static auto get_root(const Language::Types::Composite& owner)
    -> Core::Option<const Language::Monograph&> {
  Reference<const Abstract> selected(owner);
  while (true) {
    auto monograph = selected.get().select<Language::Monograph>();
    if (monograph) {
      return *monograph;
    }

    auto composite = selected.get().select<Language::Types::Composite>();
    BAIL_IF(!composite);
    selected = Reference<const Abstract>(composite->get_host());
  }
}

static auto get_option(
    const Language::Monograph& root,
    const Language::Model::Type& element)
    -> Core::Option<const Language::Types::Option&> {
  auto generic =
      root.resolve_context("Option"_view).resolve().select<Language::Generic>();
  BAIL_IF(!generic);

  Core::Static::Vector<Language::Generic::Argument, 1> arguments = {{
    Language::Generic::Argument(element),
  }};
  return generic->materialize(arguments.get_view())
      .visit(
          [](const Language::Model::Type& selected)
              -> Core::Option<const Language::Types::Option&> {
            return selected.select<Language::Types::Option>();
          },
          [](const Language::Generic::Failure&)
              -> Core::Option<const Language::Types::Option&> { return {}; });
}

auto Language::Construction::create(
    Memory::Allocator::Arena& arena,
    Types::Composite& owner,
    Bool provider) -> Core::Option<Construction&> {
  BAIL_IF(owner.get_layout().is_empty());
  auto root = get_root(owner);
  BAIL_IF(!root);

  Memory::Managed::Vector<Reference<const Abstract>> parameters(arena);
  for (const Reference<Abstract>& candidate : owner.get_addressables()) {
    auto field = candidate.get().select<Field>();
    if (!field || field->get_writability() != Writability::Internal ||
        !field->get_definition().is_published()) {
      continue;
    }

    auto option = get_option(*root, field->get_type());
    BAIL_IF(!option);
    Parameter& parameter =
        Parameter::create_synthetic(arena, field->get_name(), *option);
    parameters.insert(parameter);
  }

  Memory::Managed::Vector<Reference<const Abstract>> results(arena);
  results.insert(owner);
  Construction& construction = arena.construct_from<Construction>([&]() {
    return Construction(arena, owner, parameters, results, provider);
  });
  BAIL_IF(provider && !construction.complete_body());
  return construction;
}

auto Language::Construction::complete_body() -> Bool {
  for (const Reference<Abstract>& candidate : owner.get_addressables()) {
    auto field = candidate.get().select<Field>();
    if (!field || field->get_writability() != Writability::Internal) {
      continue;
    }

    Core::Option<Model::Pack&> fallback;
    auto authored = field->get_initializer();
    if (authored) {
      fallback = const_cast<Model::Pack&>(*authored);
    } else {
      fallback = field->get_type().create_default(arena);
    }
    BAIL_IF(!fallback);

    if (!field->get_definition().is_published()) {
      values.insert(*fallback);
      continue;
    }

    auto parameter = resolve_context(field->get_name()).select<Parameter>();
    BAIL_IF(!parameter);
    auto option_type = parameter->get_type().select<Types::Option>();
    BAIL_IF(!option_type);
    auto& identifier =
        Expressions::Identifier::create_synthetic(arena, parameter->get_name());
    BAIL_IF(!identifier.link_restored(*this));
    values.insert(
        ConstructionInput::create(
            arena, identifier, *option_type, field->get_type(), *fallback));
  }

  initializer = Expressions::Initializer::create_synthetic(
      arena, owner, values.get_view());
  return True;
}

auto Language::Construction::create_call(
    Memory::Allocator::Arena& domain,
    Core::Option<Model::Pack&> supplied) const -> Core::Option<Model::Pack&> {
  Memory::Managed::Vector<Reference<Model::Pack>> entries(domain);
  Memory::Managed::Vector<Core::View::Bytes> names(domain);
  Core::Option<const Ttx::Concept::Layout&> source =
      supplied
          ? Core::Option<const Ttx::Concept::Layout&>(supplied->get_layout())
          : Core::Option<const Ttx::Concept::Layout&>();
  Count matched = 0;
  for (Count parameter_index = 0; parameter_index < parameters.get_size();
       parameter_index++) {
    auto parameter = parameters.get_abstract(parameter_index);
    BAIL_IF(!parameter || parameter->get_name().is_empty());
    Core::View::Bytes name = parameter->get_name();
    auto selected_parameter = parameter->select<Model::Addressable>();
    auto option = selected_parameter
                      ? selected_parameter->get_type().select<Types::Option>()
                      : Core::Option<const Types::Option&>();
    BAIL_IF(!option);

    Core::Option<Model::Pack&> selected;
    if (source) {
      for (Count source_index = 0; source_index < source->get_size();
           source_index++) {
        auto source_name = source->get_name(source_index);
        if (!source_name || *source_name != name) {
          continue;
        }

        auto produced = supplied->get_produced(source_index);
        BAIL_IF(selected || !produced);
        selected = const_cast<Ttx::Model::Pack&>(produced->producer)
                       .select<Model::Pack>();
        BAIL_IF(!selected);
        matched++;
      }
    }

    entries.insert(ConstructionOption::create(domain, *option, selected));
    names.insert(name);
  }
  BAIL_IF(source && matched != source->get_size());

  Model::Pack& arguments = Model::Pack::create_completed(
      domain, entries.get_view(), names.get_view());
  BAIL_IF(!arguments.fits(parameters));
  auto& receiver =
      Expressions::Identifier::create_synthetic(domain, owner.get_name());
  if (!receiver.link_restored(owner.get_host())) {
    Core::Diagnostics::Log::error(
        "Provider construction could not select its owner Type."_view);
    return {};
  }
  auto& call =
      Access::Call::create_synthetic(domain, receiver, get_name(), arguments);
  if (!call.link_restored(owner.get_host())) {
    Core::Diagnostics::Log::error(
        "Provider construction arguments do not fit its generated Callable."_view);
    return {};
  }
  return static_cast<Model::Pack&>(call);
}

auto Language::Construction::resolve_context(Core::View::Bytes name) const
    -> const Abstract& {
  for (const Reference<const Abstract>& parameter :
       parameter_entries.get_view()) {
    if (parameter.get().get_name() == name) {
      return parameter.get();
    }
  }
  return Invalid::get_invalid();
}

auto Language::Construction::reserve_declaration(Llvm::Program& program) const
    -> Bool {
  auto reserved =
      program.get_functions().reserve_construction(program, *this, owner);
  BAIL_IF(!reserved);
  return !*reserved || Model::Callable::reserve_declaration(program);
}

auto Language::Construction::complete_declaration(Llvm::Program& program) const
    -> Bool {
  return Model::Callable::complete_declaration(program) &&
         program.get_functions().complete(program, *this);
}

auto Language::Construction::lower_declaration(Llvm::Program& program) const
    -> Bool {
  if (!provider) {
    return True;
  }

  auto lowering = program.get_functions().begin_body(program, *this);
  BAIL_IF(!lowering || !initializer);
  Llvm::Body native_body(
      program, *this, lowering->get_function(), lowering->get_callable(),
      lowering->get_sret(), lowering->get_sret_type());
  BAIL_IF(!program.get_functions().bind_parameters(native_body, *this));

  Llvm::Builder body(native_body);
  BAIL_IF(!initializer->lower(body) || !body.return_values(*initializer));
  return program.get_functions().end_body(native_body, *this);
}
