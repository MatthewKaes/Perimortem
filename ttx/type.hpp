// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"

#include "ttx/documentation.hpp"

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
  // Member is the shared entry view used by every shaped list.
  //
  // Aggregate members, layout fields, function parameters, function returns,
  // and repack results all reduce to ordered entries that may reference types,
  // defaults, documentation, and sometimes local names. Names are present when
  // authored syntax or a receiving boundary supplies them. Swizzle and slice
  // results normally use the same entry view without carrying names forward.
  // Sharing this view keeps the model from inventing separate node kinds for
  // the same layout question.
  //
  // A member references a type but does not own it. The referenced type answers
  // identity and function dispatch questions. The member answers local shape
  // questions such as whether the entry is named, whether it can be omitted
  // during construction, and which documentation was written for this use.
  class Member {
   public:
    constexpr Member() = default;
    constexpr Member(
        Perimortem::Core::View::Bytes name,
        const Type& type,
        Bool defaulted = False,
        Documentation documentation = Documentation())
        : name(name),
          type(&type),
          defaulted(defaulted),
          documentation(documentation) {}
    constexpr Member(
        Perimortem::Core::View::Bytes name,
        const Type& type,
        Documentation documentation,
        Bool defaulted = False)
        : Member(name, type, defaulted, documentation) {}

    constexpr auto get_name() const -> Perimortem::Core::View::Bytes {
      return name;
    }
    constexpr auto get_type() const -> const Type* { return type; }
    constexpr auto get_documentation() const -> Documentation {
      return documentation;
    }
    constexpr auto is_named() const -> Bool { return !name.is_empty(); }

    // Empty members are sentinels for absent layout slots and default
    // constructed parse results. A real shaped entry needs a referenced type.
    constexpr auto is_empty() const -> Bool {
      return name.is_empty() && !type;
    }
    constexpr auto is_defaulted() const -> Bool { return defaulted; }

    // Member equivalence is the reusable type-slot check used by layout
    // algorithms.
    //
    // It ignores documentation, defaults, and member names. Documentation is
    // prose. Defaults are construction policy owned by the target layout. Names
    // are interpreted by Layout before it asks two members whether their
    // referenced types are equivalent.
    //
    // That split lets a named layout compare `.x` to `.x`, while a positional
    // layout can still compare entry 0 to entry 0 without inventing a separate
    // member kind.
    auto equivalent_to(const Member& other) const -> Bool;

   private:
    Perimortem::Core::View::Bytes name;
    const Type* type = nullptr;
    Bool defaulted = False;
    Documentation documentation;
  };

  // Function is callable behavior attached to a type.
  //
  // Calls require a type because pure layouts have no function table to
  // dispatch through. `Sprite->draw(...)` can ask the Sprite type for a
  // function. `(.x = 2, .y = 3)->format()` is invalid because the layout has no
  // identity and therefore no owner for `format`.
  //
  // Parameters and results are represented as member views so call shapes use
  // the same layout fitting rules as aggregates and returns. This keeps
  // function calls from growing their own argument model when a named layout
  // already describes the exact same data.
  class Function {
   public:
    constexpr Function(
        Perimortem::Core::View::Bytes name,
        Perimortem::Core::View::Vector<Member> parameters,
        Perimortem::Core::View::Vector<Member> result,
        Documentation documentation = Documentation())
        : name(name),
          parameters(parameters),
          result(result),
          documentation(documentation) {}

    constexpr auto get_name() const -> Perimortem::Core::View::Bytes {
      return name;
    }
    constexpr auto get_parameters() const
        -> Perimortem::Core::View::Vector<Member> {
      return parameters;
    }
    constexpr auto get_result() const -> Perimortem::Core::View::Vector<Member> {
      return result;
    }
    constexpr auto get_documentation() const -> Documentation {
      return documentation;
    }

   private:
    Perimortem::Core::View::Bytes name;
    Perimortem::Core::View::Vector<Member> parameters;
    Perimortem::Core::View::Vector<Member> result;
    Documentation documentation;
  };

  explicit constexpr Type(
      Perimortem::Core::View::Bytes name,
      Documentation documentation = Documentation())
      : name(name), documentation(documentation) {}
  constexpr Type(
      Perimortem::Core::View::Bytes name,
      Perimortem::Core::View::Vector<Member> members,
      Perimortem::Core::View::Vector<const Type*> types =
          Perimortem::Core::View::Vector<const Type*>(),
      Perimortem::Core::View::Vector<Function> functions =
          Perimortem::Core::View::Vector<Function>(),
      Documentation documentation = Documentation())
      : name(name),
        members(members),
        types(types),
        functions(functions),
        documentation(documentation) {}

  static constexpr auto alias(
      Perimortem::Core::View::Bytes name,
      const Type& parent,
      Documentation documentation = Documentation()) -> Type {
    // An alias is a new authored name for an existing canonical type. Its
    // documentation belongs to the alias, not to the parent, so tools can show
    // the alias context directly or canonicalize when they want root prose.
    Type type(name, documentation);
    type.alias_parent = &parent;
    return type;
  }

  constexpr auto get_name() const -> Perimortem::Core::View::Bytes {
    return name;
  }
  constexpr auto get_documentation() const -> Documentation {
    return documentation;
  }

  // Enumerates the member entries authored on this type.
  //
  // Use `Layout(type)` for shape questions such as construction and layout
  // equivalence. This raw view exists for package export and introspection, not
  // for rebuilding layout policy outside Layout.
  constexpr auto get_members() const
      -> Perimortem::Core::View::Vector<Member> {
    return members;
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
  // Cycles return null rather than looping forever so resolution can report the
  // alias cycle at the source location that created it. Documentation is not
  // folded while canonicalizing. The alias keeps its contextual prose, and the
  // root type keeps its own documentation.
  auto canonical() const -> const Type*;

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
  constexpr auto is_alias() const -> Bool { return alias_parent != nullptr; }

 private:
  Perimortem::Core::View::Bytes name;
  Perimortem::Core::View::Vector<Member> members;
  Perimortem::Core::View::Vector<const Type*> types;
  Perimortem::Core::View::Vector<Function> functions;
  const Type* alias_parent = nullptr;
  Documentation documentation;
};

}  // namespace Ttx
