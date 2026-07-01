// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"

#include "ttx/documentation.hpp"

namespace Ttx {

class Type {
 public:
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
    constexpr auto is_empty() const -> Bool {
      return name.is_empty() && !type;
    }
    constexpr auto is_defaulted() const -> Bool { return defaulted; }
    auto equivalent_to(const Member& other) const -> Bool;

   private:
    Perimortem::Core::View::Bytes name;
    const Type* type = nullptr;
    Bool defaulted = False;
    Documentation documentation;
  };

  class Function {
   public:
    constexpr Function() = default;
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

  constexpr Type() = default;
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
    Type type;
    type.name = name;
    type.alias_parent = &parent;
    type.documentation = documentation;
    return type;
  }

  constexpr auto get_name() const -> Perimortem::Core::View::Bytes {
    return name;
  }
  constexpr auto get_documentation() const -> Documentation {
    return documentation;
  }
  constexpr auto get_members() const
      -> Perimortem::Core::View::Vector<Member> {
    return members;
  }
  constexpr auto get_types() const
      -> Perimortem::Core::View::Vector<const Type*> {
    return types;
  }
  constexpr auto get_functions() const
      -> Perimortem::Core::View::Vector<Function> {
    return functions;
  }

  auto canonical() const -> const Type*;
  auto equivalent_to(const Type& other) const -> Bool;
  auto find_member(Perimortem::Core::View::Bytes name) const -> const Member*;
  auto find_type(Perimortem::Core::View::Bytes name) const -> const Type*;
  auto find_function(Perimortem::Core::View::Bytes name) const
      -> const Function*;
  constexpr auto is_alias() const -> Bool { return alias_parent != nullptr; }
  constexpr auto is_valid() const -> Bool { return !name.is_empty(); }

 private:
  Perimortem::Core::View::Bytes name;
  Perimortem::Core::View::Vector<Member> members;
  Perimortem::Core::View::Vector<const Type*> types;
  Perimortem::Core::View::Vector<Function> functions;
  const Type* alias_parent = nullptr;
  Documentation documentation;
};

}  // namespace Ttx
