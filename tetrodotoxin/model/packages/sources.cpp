// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/model/packages/sources.hpp"

#include "perimortem/core/static/union.hpp"
#include "perimortem/core/algorithm/sort.hpp"

#include "tetrodotoxin/model/addressables/initialized.hpp"
#include "tetrodotoxin/model/app.hpp"
#include "tetrodotoxin/model/apps/lifecycle.hpp"
#include "tetrodotoxin/model/apps/program.hpp"
#include "tetrodotoxin/model/constants/aggregate.hpp"
#include "tetrodotoxin/model/expressions/call.hpp"
#include "tetrodotoxin/model/render.hpp"
#include "tetrodotoxin/model/renderables/constant.hpp"
#include "tetrodotoxin/model/renderables/push.hpp"
#include "tetrodotoxin/model/renderables/resource.hpp"
#include "tetrodotoxin/model/renderables/value.hpp"
#include "tetrodotoxin/model/renders/contract.hpp"
#include "tetrodotoxin/model/shader.hpp"
#include "tetrodotoxin/model/stages/implemented.hpp"
#include "tetrodotoxin/model/stages/required.hpp"
#include "tetrodotoxin/model/types/named_vector.hpp"
#include "tetrodotoxin/model/types/object.hpp"
#include "tetrodotoxin/model/types/represented.hpp"
#include "tetrodotoxin/model/types/structure.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/model/addressable.hpp"
#include "ttx/model/alias.hpp"
#include "ttx/model/callable.hpp"
#include "ttx/model/expression.hpp"
#include "ttx/model/generic.hpp"
#include "ttx/model/types/access.hpp"
#include "ttx/model/types/accesses/materialized.hpp"
#include "ttx/model/types/terminal.hpp"
#include "ttx/model/types/vector.hpp"
#include "ttx/model/types/vectors/materialized.hpp"
#include "ttx/model/types/view.hpp"
#include "ttx/model/types/views/materialized.hpp"

using namespace Perimortem::Core;
using namespace Ttx::Concept;

static auto precedes(View::Bytes left, View::Bytes right) -> Bool {
  Count shared = Math::min(left.get_size(), right.get_size());
  for (Count i = 0; i < shared; i++) {
    if (left[i] != right[i]) {
      return left[i] < right[i];
    }
  }

  return left.get_size() < right.get_size();
}

// DefinitionOrder provides deterministic name ordering over valid semantic
// references during package canonicalization.
class DefinitionOrder {
 private:
  class Empty {};

 public:
  DefinitionOrder() : definition(Empty()) {}
  explicit DefinitionOrder(const Abstract& definition)
      : definition(definition) {}

  auto operator>(const DefinitionOrder& other) const -> Bool {
    return precedes(other.get().get_name(), get().get_name());
  }

  constexpr auto get() const -> const Abstract& {
    return definition.visit(
        []() -> const Abstract& { __builtin_trap(); },
        [](const Empty&) -> const Abstract& { __builtin_trap(); },
        [](const Reference<Abstract>& selected) -> const Abstract& {
          return selected.get();
        });
  }

 private:
  Static::Union<Empty, Reference<Abstract>> definition;
};

auto Tetrodotoxin::Model::Packages::Sources::construct(
    Perimortem::Memory::Allocator::Arena& arena,
    View::Vector<Reference<Model::Source>> sources,
    const Model::Namespace& exports,
    View::Vector<Model::Terminal> terminals,
    View::Vector<Model::Shaders::Product> shader_products) -> const Abstract& {
  if (sources.is_empty()) {
    return Invalid::get_invalid();
  }

  Sources* package = arena.reserve<Sources>();
  new (package) Sources(arena, sources, exports, terminals, shader_products);
  if (!package->valid) {
    return Invalid::get_invalid();
  }

  return *package;
}

Tetrodotoxin::Model::Packages::Sources::Sources(
    Perimortem::Memory::Allocator::Arena& arena,
    View::Vector<Reference<Model::Source>> members,
    const Model::Namespace& exports,
    View::Vector<Model::Terminal> terminals,
    View::Vector<Model::Shaders::Product> shader_products)
    : exports(exports),
      sources(arena),
      terminals(arena),
      shader_products(arena),
      source_index(arena),
      dependencies(arena),
      definitions(arena),
      definition_index(arena),
      pending_namespaces(arena) {
  this->terminals.reset(terminals.get_size());
  for (Count i = 0; i < terminals.get_size(); i++) {
    this->terminals.insert(
        Model::Terminal(
            arena, terminals[i].get_path(), terminals[i].get_content()));
  }
  this->shader_products.reset(shader_products.get_size());
  for (Count i = 0; i < shader_products.get_size(); i++) {
    this->shader_products.insert(
        Model::Shaders::Product(
            arena, shader_products[i].get_shader(),
            shader_products[i].get_stages()));
  }

  const Model::Environment& environment = members[0].get().get_environment();
  sources.reset(members.get_size());
  source_index.ensure_capacity(members.get_size());
  for (Count i = 0; i < members.get_size(); i++) {
    const Model::Source& source = members[i].get();
    if (&source.get_environment() != &environment ||
        source_index.find(&source) != nullptr) {
      valid = False;
      return;
    }

    source_index.insert(&source, True);
    sources.insert(members[i]);
  }

  const View::Vector<Reference<Model::Package>> packages =
      environment.get_packages();
  dependencies.reset(packages.get_size());
  for (Count i = 0; i < packages.get_size(); i++) {
    dependencies.insert(packages[i]);
  }

  collect_namespace(exports);
  for (Count i = 0; i < pending_namespaces.get_size(); i++) {
    collect_namespace(pending_namespaces[i].get());
  }
  pending_namespaces.clear();
}

auto Tetrodotoxin::Model::Packages::Sources::collect_namespace(
    const Model::Namespace& namespace_object) -> void {
  // Definition IDs are canonical package coordinates, so authored definition
  // order cannot renumber an otherwise identical graph. Each retained scope is
  // traversed by name while the public Exports order remains untouched.
  Count root_count = namespace_object.get_root_count();
  auto* ordered = Data::cast<DefinitionOrder>(
      definitions.get_arena().allocate(sizeof(DefinitionOrder) * root_count));
  Count ordered_count = 0;
  for (Count i = 0; i < root_count; i++) {
    const Abstract& candidate = namespace_object.get_root(i);
    if (definition_index.find(&candidate) == nullptr) {
      new (ordered + ordered_count++) DefinitionOrder(candidate);
    }
  }

  auto sorted =
      Algorithm::sort(Access::Vector<DefinitionOrder>(ordered, ordered_count));
  for (Count i = 0; i < sorted.get_size(); i++) {
    collect_definition(sorted[i].get());
  }
}

auto Tetrodotoxin::Model::Packages::Sources::resolve_context(
    View::Bytes route) const -> const Abstract& {
  return exports.resolve_context(route);
}

auto Tetrodotoxin::Model::Packages::Sources::get_definition(Count id) const
    -> const Abstract& {
  if (id >= definitions.get_size()) {
    return Invalid::get_invalid();
  }

  return definitions.get_view()[id].get();
}

auto Tetrodotoxin::Model::Packages::Sources::get_definition_id(
    const Abstract& definition) const -> Count {
  const DefinitionIndex::Entry* selected = definition_index.find(&definition);
  return selected == nullptr ? Count(-1) : selected->value;
}

auto Tetrodotoxin::Model::Packages::Sources::collect_definition(
    const Abstract& definition) -> void {
  if (!valid || definition_index.find(&definition) != nullptr ||
      is_external(definition)) {
    return;
  }

  definition_index.insert(&definition, definitions.get_size());
  definitions.insert(Reference<Abstract>(definition));
  if (definition.is<Model::Namespace>()) {
    pending_namespaces.insert(
        Reference<Model::Namespace>(definition.assume<Model::Namespace>()));
    return;
  }

  if (definition.is<Ttx::Model::Alias>()) {
    collect_definition(definition.resolve());
    return;
  }

  if (definition.is<Model::Types::Structure>()) {
    const auto& structure = definition.assume<Model::Types::Structure>();
    for (Count i = 0; i < structure.get_member_count(); i++) {
      collect_definition(structure.get_member(i));
    }
    if (structure.is<Model::Types::Represented>()) {
      collect_definition(structure.get_shader_type());
    }
    return;
  }

  if (definition.is<Model::Types::Object>()) {
    const auto& object = definition.assume<Model::Types::Object>();
    for (Count i = 0; i < object.get_member_count(); i++) {
      collect_definition(object.get_member(i));
    }
    return;
  }

  if (definition.is<Model::Types::NamedVector>()) {
    const auto& vector = definition.assume<Model::Types::NamedVector>();
    collect_definition(vector.get_element_type());
    for (Count i = 0; i < vector.get_member_count(); i++) {
      collect_definition(vector.get_member(i));
    }
    return;
  }

  if (definition.is<Ttx::Model::Types::Vectors::Materialized>()) {
    const auto& vector =
        definition.assume<Ttx::Model::Types::Vectors::Materialized>();
    collect_definition(vector.get_generic());
    collect_layout(vector.get_arguments());
    return;
  }
  if (definition.is<Ttx::Model::Types::Accesses::Materialized>()) {
    const auto& access =
        definition.assume<Ttx::Model::Types::Accesses::Materialized>();
    collect_definition(access.get_generic());
    collect_layout(access.get_arguments());
    return;
  }
  if (definition.is<Ttx::Model::Types::Views::Materialized>()) {
    const auto& view =
        definition.assume<Ttx::Model::Types::Views::Materialized>();
    collect_definition(view.get_generic());
    collect_layout(view.get_arguments());
    return;
  }

  if (definition.is<Model::App>()) {
    const auto& app = definition.assume<Model::App>();
    collect_layout(app.get_layout());
    for (Count i = 0; i < app.get_member_count(); i++) {
      collect_definition(app.get_member(i));
    }
    collect_definition(app.get_start());
    collect_definition(app.get_frame());
    collect_definition(app.get_stop());
    for (Count i = 0; i < app.get_render_root_count(); i++) {
      collect_definition(app.get_render_root(i));
    }
    for (Count i = 0; i < app.get_binding_count(); i++) {
      collect_definition(app.get_binding(i).get_render());
      collect_definition(app.get_binding(i).get_shader());
    }
    return;
  }

  if (definition.is<Model::Render>()) {
    const auto& render = definition.assume<Model::Render>();
    collect_layout(render.get_layout());
    collect_definition(render.get_constants_namespace());
    collect_definition(render.get_pushes_namespace());
    collect_definition(render.get_resources_namespace());
    for (Count i = 0; i < render.get_constant_count(); i++) {
      collect_definition(render.get_constant(i));
    }
    for (Count i = 0; i < render.get_push_count(); i++) {
      collect_definition(render.get_push(i));
    }
    for (Count i = 0; i < render.get_resource_count(); i++) {
      collect_definition(render.get_resource(i));
    }
    for (Count i = 0; i < render.get_stage_count(); i++) {
      collect_definition(render.get_stage(i));
    }
    return;
  }

  if (definition.is<Model::Shader>()) {
    const auto& shader = definition.assume<Model::Shader>();
    collect_definition(shader.get_render());
    for (Count i = 0; i < shader.get_stage_count(); i++) {
      collect_definition(shader.get_stage(i));
    }
    return;
  }

  if (definition.is<Model::Stages::Implemented>()) {
    const auto& stage = definition.assume<Model::Stages::Implemented>();
    collect_definition(stage.get_required());
    collect_layout(stage.get_parameters());
    collect_layout(stage.get_results());
    collect_body(stage.get_body());
    return;
  }
  if (definition.is<Model::Stages::Required>()) {
    const auto& stage = definition.assume<Model::Stages::Required>();
    collect_layout(stage.get_parameters());
    collect_layout(stage.get_results());
    for (Count i = 0; i < stage.get_read_count(); i++) {
      collect_definition(stage.get_read(i));
    }
    return;
  }
  if (definition.is<Model::Apps::Lifecycle>()) {
    const auto& lifecycle = definition.assume<Model::Apps::Lifecycle>();
    collect_layout(lifecycle.get_parameters());
    collect_layout(lifecycle.get_results());
    collect_body(lifecycle.get_body());
    return;
  }

  if (definition.is<Ttx::Model::Callable>()) {
    const auto& callable = definition.assume<Ttx::Model::Callable>();
    collect_layout(callable.get_parameters());
    collect_layout(callable.get_results());
    return;
  }

  if (definition.is<Model::Addressables::Initialized>()) {
    const auto& initialized =
        definition.assume<Model::Addressables::Initialized>();
    collect_definition(initialized.get_type());
    collect_definition(initialized.get_initializer());
    return;
  }
  if (definition.is<Model::Renderables::Value>()) {
    const auto& value = definition.assume<Model::Renderables::Value>();
    collect_definition(value.get_type());
    collect_definition(value.get_initializer());
    return;
  }
  if (definition.is<Model::Renderables::Constant>()) {
    const auto& constant = definition.assume<Model::Renderables::Constant>();
    collect_definition(constant.get_value());
    return;
  }
  if (definition.is<Model::Renderables::Push>()) {
    const auto& push = definition.assume<Model::Renderables::Push>();
    collect_definition(push.get_source());
    return;
  }
  if (definition.is<Model::Renderables::Resource>()) {
    const auto& resource = definition.assume<Model::Renderables::Resource>();
    collect_definition(resource.get_source());
    return;
  }
  if (definition.is<Ttx::Model::Addressable>()) {
    collect_definition(definition.assume<Ttx::Model::Addressable>().get_type());
    return;
  }

  if (definition.is<Model::Constants::Aggregate>()) {
    const auto& aggregate = definition.assume<Model::Constants::Aggregate>();
    collect_definition(aggregate.get_type());
    for (Count i = 0; i < aggregate.get_size(); i++) {
      collect_definition(aggregate.get_value(i));
    }
    return;
  }
  if (definition.is<Ttx::Model::Constant>()) {
    collect_definition(definition.assume<Ttx::Model::Constant>().get_type());
    return;
  }
  if (definition.is<Model::Expressions::Call>()) {
    const auto& call = definition.assume<Model::Expressions::Call>();
    collect_definition(call.get_callable());
    collect_definition(call.get_type());
    collect_layout(call.get_inputs());
    return;
  }
  if (definition.is<Ttx::Model::Expression>()) {
    const auto& expression = definition.assume<Ttx::Model::Expression>();
    collect_definition(expression.get_type());
    collect_layout(expression.get_inputs());
    return;
  }

  if (definition.is<Ttx::Model::Types::Vector>()) {
    const auto& vector = definition.assume<Ttx::Model::Types::Vector>();
    collect_definition(vector.get_element_type());
    collect_layout(vector.get_layout());
    return;
  }
  if (definition.is<Ttx::Model::Types::Access>()) {
    collect_definition(
        definition.assume<Ttx::Model::Types::Access>().get_element_type());
    return;
  }
  if (definition.is<Ttx::Model::Types::View>()) {
    collect_definition(
        definition.assume<Ttx::Model::Types::View>().get_element_type());
    return;
  }
  if (definition.is<Ttx::Model::Type>()) {
    collect_layout(definition.assume<Ttx::Model::Type>().get_layout());
    return;
  }
}

auto Tetrodotoxin::Model::Packages::Sources::collect_layout(
    const Ttx::Concept::Layout& layout) -> void {
  for (Count i = 0; i < layout.get_size(); i++) {
    collect_definition(layout.get_abstract(i));
  }
}

auto Tetrodotoxin::Model::Packages::Sources::collect_body(
    const Ttx::Model::Body& body) -> void {
  const auto values = body.get_values();
  for (Count i = 0; i < values.get_size(); i++) {
    collect_definition(values[i].get_type());
  }

  // A Body operation is a closed value union. Visit it directly so semantic
  // references are collected once without manufacturing nullable candidates
  // for alternatives which are not present.
  const auto operations = body.get_operations();
  for (Count i = 0; i < operations.get_size(); i++) {
    operations[i].visit(
        []() -> void { __builtin_trap(); },
        [&](const Ttx::Model::Bodies::Operations::Constant& value) -> void {
          collect_definition(value.get_value());
        },
        [](const Ttx::Model::Bodies::Operations::Aggregate&) -> void {},
        [&](const Ttx::Model::Bodies::Operations::Projection& value) -> void {
          collect_definition(value.get_addressable());
        },
        [&](const Ttx::Model::Bodies::Operations::Call& value) -> void {
          collect_definition(value.get_callable());
        },
        [&](const Ttx::Model::Bodies::Operations::Load& value) -> void {
          collect_definition(value.get_addressable());
        },
        [&](const Ttx::Model::Bodies::Operations::Store& value) -> void {
          collect_definition(value.get_addressable());
        },
        [](const Ttx::Model::Bodies::Operations::Binary&) -> void {},
        [](const Ttx::Model::Bodies::Operations::Convert&) -> void {},
        [](const Ttx::Model::Bodies::Operations::IndexedRead&) -> void {},
        [](const Ttx::Model::Bodies::Operations::IndexedWrite&) -> void {},
        [](const Ttx::Model::Bodies::Operations::Branch&) -> void {},
        [](const Ttx::Model::Bodies::Operations::Jump&) -> void {},
        [](const Ttx::Model::Bodies::Operations::Return&) -> void {});
  }
}

auto Tetrodotoxin::Model::Packages::Sources::is_external(
    const Abstract& definition) -> Bool {
  Count matches = 0;
  for (Count i = 0; i < dependencies.get_size(); i++) {
    const Model::Package& package = dependencies[i].get();
    if (&definition == &package ||
        package.get_definition_id(definition) != Count(-1)) {
      matches++;
    }
  }
  if (matches > 1) {
    // Builtins are process-shared evaluator inputs rather than package-owned
    // singletons. A package with exactly one owning dependency uses that
    // dependency's identity. If multiple independent dependencies happen to
    // expose the same evaluator identity, this package assigns its own local
    // coordinate. A non builtin semantic owner remains ambiguous and invalid.
    if (definition.is<Ttx::Model::Types::Terminal>() ||
        definition.is<Ttx::Model::Generic>()) {
      return False;
    }
    valid = False;
  }
  return matches != 0;
}
