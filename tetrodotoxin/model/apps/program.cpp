// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/model/apps/program.hpp"

#include "ttx/concept/invalid.hpp"
#include "ttx/model/types/flag.hpp"

using namespace Perimortem::Core;
using namespace Ttx::Concept;

auto Tetrodotoxin::Model::Apps::Program::LifecycleEdge::set(
    const Lifecycle& value) -> Bool {
  if (!is_empty()) {
    return False;
  }

  lifecycle = Reference<Lifecycle>(value);
  return True;
}

auto Tetrodotoxin::Model::Apps::Program::LifecycleEdge::is_empty() const
    -> Bool {
  return lifecycle.visit(
      []() -> Bool { __builtin_trap(); }, [](const Empty&) { return True; },
      [](const Reference<Lifecycle>&) { return False; });
}

auto Tetrodotoxin::Model::Apps::Program::LifecycleEdge::get() const
    -> const Lifecycle& {
  return lifecycle.visit(
      []() -> const Lifecycle& { __builtin_trap(); },
      [](const Empty&) -> const Lifecycle& { __builtin_trap(); },
      [](const Reference<Lifecycle>& selected) -> const Lifecycle& {
        return selected.get();
      });
}

Tetrodotoxin::Model::Apps::Program::Program(
    Perimortem::Memory::Allocator::Arena& arena,
    View::Bytes name,
    const Documentation& documentation)
    : name(arena.proxy(name)),
      documentation(documentation),
      members(arena),
      fields(arena),
      render_roots(arena),
      bindings(arena) {}

auto Tetrodotoxin::Model::Apps::Program::resolve() const -> const Abstract& {
  return state_sealed ? static_cast<const Abstract&>(*this)
                      : Invalid::get_invalid();
}

auto Tetrodotoxin::Model::Apps::Program::resolve_context(
    View::Bytes route) const -> const Abstract& {
  return completed ? members.resolve_context(route) : Invalid::get_invalid();
}

auto Tetrodotoxin::Model::Apps::Program::add_field(
    const Ttx::Model::Addressable& field) -> Bool {
  if (state_sealed) {
    return False;
  }
  Bool added = members.add(field, False);
  if (!added) {
    return False;
  }
  fields.insert(Reference<Ttx::Model::Addressable>(field));
  return True;
}

auto Tetrodotoxin::Model::Apps::Program::seal_state() -> Bool {
  if (state_sealed) {
    return False;
  }
  layout = Ttx::Model::Layouts::Structured(fields.get_view());
  state_sealed = True;
  return True;
}

auto Tetrodotoxin::Model::Apps::Program::add_lifecycle(
    const Lifecycle& lifecycle) -> Bool {
  if (!state_sealed || completed) {
    return False;
  }

  Bool added = members.add(lifecycle, False);
  return added;
}

auto Tetrodotoxin::Model::Apps::Program::set_role(
    Role role,
    const Lifecycle& lifecycle) -> Bool {
  if (completed || !lifecycle_fits(role, lifecycle)) {
    return False;
  }

  switch (role) {
  case Role::Start:
    return start.set(lifecycle);
  case Role::Frame:
    return frame.set(lifecycle);
  case Role::Stop:
    return stop.set(lifecycle);
  }

  __builtin_unreachable();
}

auto Tetrodotoxin::Model::Apps::Program::add_render_root(
    const Ttx::Model::Addressable& root) -> Bool {
  if (completed || !root.get_type().resolve().is<Model::Render>()) {
    return False;
  }

  for (Count i = 0; i < render_roots.get_size(); i++) {
    if (&render_roots[i].get() == &root) {
      return False;
    }
  }

  render_roots.insert(Reference<Ttx::Model::Addressable>(root));
  return True;
}

auto Tetrodotoxin::Model::Apps::Program::add_binding(
    const Model::Render& render,
    const Model::Shader& shader) -> Bool {
  if (completed || &shader.get_render() != &render) {
    return False;
  }

  for (Count i = 0; i < bindings.get_size(); i++) {
    if (&bindings[i].get_render() == &render) {
      return False;
    }
  }

  bindings.insert(Binding(render, shader));
  return True;
}

auto Tetrodotoxin::Model::Apps::Program::lifecycle_fits(
    Role role,
    const Lifecycle& lifecycle) const -> Bool {
  const Ttx::Concept::Layout& parameters = lifecycle.get_parameters();
  const Ttx::Concept::Layout& results = lifecycle.get_results();
  Count expected_parameters = role == Role::Frame ? 2 : 1;
  if (!lifecycle.get_body().is_valid() ||
      parameters.get_size() != expected_parameters ||
      !parameters.get_abstract(0).is<Ttx::Model::Addressable>() ||
      &parameters.get_abstract(0)
              .assume<Ttx::Model::Addressable>()
              .get_type() != this) {
    return False;
  }

  if (role != Role::Frame) {
    return results.is_empty();
  }

  return results.get_size() == 1 &&
         results.get_abstract(0).resolve().is<Ttx::Model::Types::Flag>();
}

auto Tetrodotoxin::Model::Apps::Program::complete() -> Bool {
  if (!state_sealed || completed || start.is_empty() || frame.is_empty() ||
      stop.is_empty() || render_roots.is_empty() || bindings.is_empty()) {
    return False;
  }

  for (Count root = 0; root < render_roots.get_size(); root++) {
    const Abstract& render = render_roots[root].get().get_type().resolve();
    Count matches = 0;
    for (Count binding = 0; binding < bindings.get_size(); binding++) {
      matches += &bindings[binding].get_render() == &render ? 1 : 0;
    }
    if (matches != 1) {
      return False;
    }
  }

  completed = True;
  return True;
}

auto Tetrodotoxin::Model::Apps::Program::get_render_root(Count index) const
    -> const Abstract& {
  if (index >= render_roots.get_size()) {
    return Invalid::get_invalid();
  }

  return render_roots.get_view()[index].get();
}
