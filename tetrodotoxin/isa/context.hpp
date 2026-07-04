// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/dynamic/map.hpp"

#include "ttx/core/types.hpp"
#include "ttx/lexical/cursor.hpp"
#include "ttx/type.hpp"

namespace Tetrodotoxin::Isa {

// Evaluation context for one body ISA.
//
// ISAs publish visible working types into this context while evaluating token
// bytecode. The evaluator returns the root Type for the body once it has enough
// information to actualize the source result.
//
// Any memory associated with a single Virtual Machine cluster should use the
// clusters shared memory stored in `arena` for object generation if the VM
// depends on the results being persistant. The shared cluster memory is also
// only cleaned up on a reboot of the cluster so ISAs should use the standard
// Bibliotheca for complex temporary objects that are memory intensive.
class Context {
 public:
  explicit Context(Perimortem::Memory::Allocator::Arena& arena)
      : arena(arena) {}

  constexpr auto get_arena() const -> Perimortem::Memory::Allocator::Arena& {
    return arena;
  }
  auto define_type(Perimortem::Core::View::Bytes name, const Ttx::Type& type)
      -> Bool {
    if (types.find(name) != nullptr || Ttx::Core::Types::find_type(name)) {
      return False;
    }

    types.insert(name, &type);
    return True;
  }

  auto define_type(const Ttx::Type& type) -> Bool {
    return define_type(type.get_name(), type);
  }

  auto find_type(Perimortem::Core::View::Bytes name) const
      -> const Ttx::Type* {
    const auto* type = types.find(name);
    return type == nullptr ? nullptr : type->value;
  }

  auto resolve_type(Ttx::Lexical::Cursor& cursor) const -> const Ttx::Type* {
    const Ttx::Lexical::Token* root = cursor.require(
        Ttx::Lexical::Class::Type::Type, "Expected Type name."_view);
    if (root == nullptr) {
      return nullptr;
    }

    return resolve_type(cursor, root->get_text());
  }

  auto resolve_type(
      Ttx::Lexical::Cursor& cursor,
      Perimortem::Core::View::Bytes root_name) const -> const Ttx::Type* {
    const Ttx::Type* type = find_type(root_name);
    if (type == nullptr) {
      type = Ttx::Core::Types::find_type(root_name);
    }

    return resolve_type(cursor, type);
  }

  auto resolve_type(Ttx::Lexical::Cursor& cursor, const Ttx::Type* type) const
      -> const Ttx::Type* {
    while (cursor.matches(Ttx::Lexical::Class::Type::TypeAccessOp)) {
      cursor.consume();

      const Ttx::Lexical::Token* segment = cursor.require(
          Ttx::Lexical::Class::Type::Type,
          "Expected Type name after `::`."_view);
      if (segment == nullptr) {
        return nullptr;
      }

      if (type != nullptr) {
        type = type->find_type(segment->get_text());
      }
    }

    return resolve_type_arguments(cursor, type);
  }

 private:
  auto resolve_type_arguments(
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Type* type) const -> const Ttx::Type* {
    if (!cursor.matches(Ttx::Lexical::Class::Type::IndexStart)) {
      return type;
    }

    if (type != nullptr && type->get_name() == "View"_view) {
      const Count start = cursor.get_token_index();
      cursor.consume();
      if (cursor.matches(Ttx::Lexical::Class::Type::Type) &&
          cursor.current().get_text() == "Bytes"_view) {
        cursor.consume();
        if (cursor.matches(Ttx::Lexical::Class::Type::IndexEnd)) {
          cursor.consume();
          return Ttx::Core::Types::find_type("View[Bytes]"_view);
        }
      }
      cursor.seek_token(start);
    }

    return consume_type_arguments(cursor) ? type : nullptr;
  }

  auto consume_type_arguments(Ttx::Lexical::Cursor& cursor) const -> Bool {
    if (!cursor.matches(Ttx::Lexical::Class::Type::IndexStart)) {
      return True;
    }

    Count depth = 0;
    while (!cursor.matches(Ttx::Lexical::Class::Type::EndOfStream)) {
      if (cursor.matches(Ttx::Lexical::Class::Type::IndexStart)) {
        depth++;
        cursor.consume();
        continue;
      }

      if (cursor.matches(Ttx::Lexical::Class::Type::IndexEnd)) {
        cursor.consume();
        depth--;
        if (depth == 0) {
          return True;
        }
        continue;
      }

      cursor.consume();
    }

    cursor.token_error("Expected `]` after type arguments."_view);
    return False;
  }

  Perimortem::Memory::Allocator::Arena& arena;
  Perimortem::Memory::Dynamic::Map<
      Perimortem::Core::View::Bytes,
      const Ttx::Type*>
      types;
};

}  // namespace Tetrodotoxin::Isa
