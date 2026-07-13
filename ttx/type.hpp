// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/null_terminated.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"

#include "ttx/attribute.hpp"
#include "ttx/documentation.hpp"
#include "ttx/function.hpp"
#include "ttx/layout.hpp"
#include "ttx/member.hpp"

namespace Ttx {

// Type is the identity-bearing half of the TTX model and represents a named
// source concept that can answer identity and dispatch questions.
//
// It owns the facts that require a stable authored name. Those facts are the
// public name, nested types reached through `::`, callable functions reached
// through `->`, members that can project into a Layout, aliases, and
// documentation attached to the authored name. In short, Type answers:
//
// * Who is this?
// * What named types does this contain?
// * What functions does this dispatch?
// * What layout shape does this present when projected?
//
// Type deliberately does not own ABI storage facts such as size, alignment, or
// offsets. Those facts need target lowering context. Keeping them out of Type
// prevents the language model from pretending that every layout already knows
// how it will be packed on every backend.
//
// It also does not own expression trees, package loading, dialect registration,
// or construction policy. Those concepts have their own owners. A Type can
// project to a Layout, but it does not become the owner of all layout
// operations.
//
// Use Type when the question needs identity or dispatch. Examples are asking
// whether `Graphics::Sprite` resolves to a nested type, whether `Sprite->draw`
// exists, whether an alias reaches the same canonical object as another type,
// or which documentation belongs to the authored name.
//
// Shape fitting is left to Layout. A layout literal such as `[.x = 2, .y = 3]`
// has no type identity and cannot dispatch a function. When the question is
// whether shaped data can construct something, first project the target type
// with `Layout(type)` and ask the layout question there.
//
// An empty name is reserved for the hidden invalid bottom value used by queries
// such as `canonical()` when a type chain cannot resolve to a real identity.
// Callers ask `is_invalid()` instead of naming that sentinel directly.
//
// Type equivalence is canonical address identity. Layout shape is queried by
// constructing `Layout(type)`, which keeps the tree from owning a second
// semantic model of the same program.
//
// Dialect kinds, ABI categories, and package classifications stay with the
// extensible registry, resolver, and lowering layer that own those facts. Type
// stays an identity object instead of becoming a shadow IR that duplicates the
// rest of the toolchain.
class Type {
 public:
  static constexpr Perimortem::Core::View::Bytes display_name_attribute =
      "display_name"_view;

  explicit constexpr Type(
      Perimortem::Core::View::Bytes name,
      Documentation documentation = Documentation())
      : name(name), documentation(documentation) {}
  constexpr Type(
      Perimortem::Core::View::Bytes name,
      Perimortem::Core::View::Vector<Attribute> attributes,
      Documentation documentation = Documentation())
      : name(name), attributes(attributes), documentation(documentation) {}
  constexpr Type(
      Perimortem::Core::View::Bytes name,
      Layout layout,
      Perimortem::Core::View::Vector<const Type*> types =
          Perimortem::Core::View::Vector<const Type*>(),
      Perimortem::Core::View::Vector<Function> functions =
          Perimortem::Core::View::Vector<Function>(),
      Documentation documentation = Documentation(),
      Perimortem::Core::View::Vector<Attribute> attributes =
          Perimortem::Core::View::Vector<Attribute>())
      : name(name),
        layout(layout),
        types(types),
        functions(functions),
        attributes(attributes),
        documentation(documentation) {}
  constexpr Type(
      Perimortem::Core::View::Bytes name,
      Perimortem::Core::View::Vector<Member> members,
      Perimortem::Core::View::Vector<const Type*> types =
          Perimortem::Core::View::Vector<const Type*>(),
      Perimortem::Core::View::Vector<Function> functions =
          Perimortem::Core::View::Vector<Function>(),
      Documentation documentation = Documentation(),
      Perimortem::Core::View::Vector<Attribute> attributes =
          Perimortem::Core::View::Vector<Attribute>())
      : Type(
            name,
            Layout(members),
            types,
            functions,
            documentation,
            attributes) {}

  static constexpr auto alias(
      Perimortem::Core::View::Bytes name,
      const Type& parent,
      Documentation documentation = Documentation(),
      Perimortem::Core::View::Vector<Attribute> attributes =
          Perimortem::Core::View::Vector<Attribute>()) -> Type {
    // An alias is a new authored name for an existing canonical type. Its
    // documentation belongs to the alias, not to the parent, so tools can show
    // the alias context directly or canonicalize when they want root prose.
    Type type(name, attributes, documentation);
    type.alias_parent = &parent;
    return type;
  }

  constexpr auto get_name() const -> Perimortem::Core::View::Bytes {
    return name;
  }

  constexpr auto get_documentation() const -> Documentation {
    return documentation;
  }

  constexpr auto get_attributes() const
      -> Perimortem::Core::View::Vector<Attribute> {
    return attributes;
  }

  operator Layout() const { return Layout(*this); }
  constexpr auto get_layout() const -> Layout { return layout; }

  // Enumerates the member entries authored on this type by projecting through
  // the owned layout. Use the Layout object itself for shape questions.
  constexpr auto get_members() const -> Perimortem::Core::View::Vector<Member> {
    return layout.get_members();
  }

  // Enumerates nested type entries for export and registry-style inspection.
  // Use `find_type()` when implementing `::` lookup.
  constexpr auto get_types() const
      -> Perimortem::Core::View::Vector<const Type*> {
    return types;
  }

  // Enumerates callable entries for export and tooling. Use `find_function()`
  // when implementing `->` dispatch.
  constexpr auto get_functions() const
      -> Perimortem::Core::View::Vector<Function> {
    return functions;
  }

  // Follows alias parents and returns the root type.
  //
  // Cycles return Invalid rather than looping forever so resolution can report
  // the alias cycle at the source location that created it. Documentation is
  // not folded while canonicalizing. The alias keeps its contextual prose, and
  // the root type keeps its own documentation.
  auto canonical() const -> const Type&;

  // Builds a short, diagnostic-facing name for this type.
  //
  // The type's authored name is enough for local facts. Producers that know a
  // public path, such as a package export, may attach `display_name` so
  // diagnostics can say `Graphics::Size2D alias of Math::Geometry::Size2D`
  // without asking the resolver to carry a side table. The attribute is a
  // presentation hint only; canonical address identity is still the source of
  // truth for equivalence.
  auto describe() const -> Perimortem::Memory::Dynamic::Bytes;

  // Type equivalence is deliberately narrow.
  //
  // Two types are equivalent when their canonical addresses are the same. This
  // is the right query for identity checks such as alias equality and overload
  // ownership. It is the wrong query for layout construction because two
  // different types can still project to layouts that fit the same source
  // shape.
  auto equivalent_to(const Type& other) const -> Bool;

  // Lookup dispatches through canonical aliases.
  //
  // `::` asks the type for nested type information such as
  // `Graphics::Shaders::Default2D`. `->` asks for functions owned by the type.
  // `.` is exposed here so a type can degrade to its layout shape and find a
  // member by name. The distinction keeps the three access modes visible even
  // when they start from the same authored type name. Name lookup returns the
  // first authored match. More precise function overload selection should be a
  // separate argument-layout-aware query rather than changing this name probe.
  auto find_member(Perimortem::Core::View::Bytes name) const -> const Member*;
  auto find_type(Perimortem::Core::View::Bytes name) const -> const Type*;
  auto find_function(Perimortem::Core::View::Bytes name) const
      -> const Function*;

  // Attribute lookup has two useful modes.
  //
  // `find_attribute` is local authored metadata. It answers what was written on
  // this exact name, so an alias can still say `isa = Alias` without hiding its
  // presentation facts.
  //
  // `resolve_attribute` is the lowering/dispatch query. It walks the alias
  // chain in authored order, keeping the first value found. Use it for
  // value-producing facts such as a target `cpp` type name.
  //
  // `attribute_equals` tests the whole resolved identity for one exact value.
  // This is useful for facts such as ABI categories that participate in alias
  // identity without replacing the value selected by `resolve_attribute`.
  auto find_attribute(Perimortem::Core::View::Bytes key) const
      -> const Attribute*;
  auto resolve_attribute(Perimortem::Core::View::Bytes key) const
      -> const Attribute*;
  auto attribute_equals(
      Perimortem::Core::View::Bytes key,
      Perimortem::Core::View::Bytes value) const -> Bool;
  auto is_invalid() const -> Bool;
  constexpr auto is_alias() const -> Bool { return alias_parent != nullptr; }
  constexpr auto get_alias_parent() const -> const Type* {
    return alias_parent;
  }

 private:
  auto display_name() const -> Perimortem::Core::View::Bytes;
  auto has_display_name() const -> Bool;

  Perimortem::Core::View::Bytes name;
  Layout layout;
  Perimortem::Core::View::Vector<const Type*> types;
  Perimortem::Core::View::Vector<Function> functions;
  Perimortem::Core::View::Vector<Attribute> attributes;
  const Type* alias_parent = nullptr;
  Documentation documentation;
};

}  // namespace Ttx
