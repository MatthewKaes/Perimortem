// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "ttx/core/types.hpp"
#include "ttx/type.hpp"

namespace Tetrodotoxin::Isa {

// Evaluation context for one body ISA.
//
// ISAs publish TTX facts into this context instead of returning private result
// objects. Resolution owns the record and import lifetime. The ISA owns the
// source body it evaluates and publishes the root Type that represents that
// body to export for TTX `::`, `.`, and `->` queries.
//
// Any memory associated with a single Virtual Machine cluster should use the
// clusters shared memory stored in `arena` for object generation if the VM
// depends on the results being persistant. The shared cluster memory is also
// only cleaned up on a reboot of the cluster so ISAs should use the standard
// Bibliotheca for complex temporary objects that are memory intensive.
class Context {
 public:
  class Import {
   public:
    Import() = default;
    constexpr Import(Perimortem::Core::View::Bytes name, const Ttx::Type& type)
        : name(name), type(&type) {}

    constexpr auto get_name() const -> Perimortem::Core::View::Bytes {
      return name;
    }
    constexpr auto get_type() const -> const Ttx::Type* { return type; }

   private:
    Perimortem::Core::View::Bytes name;
    const Ttx::Type* type = nullptr;
  };

  Context(
      Perimortem::Memory::Allocator::Arena& arena,
      Perimortem::Core::View::Bytes source_name,
      Perimortem::Core::View::Vector<Import> imports =
          Perimortem::Core::View::Vector<Import>())
      : arena(arena), source_name(source_name), imports(imports) {}

  constexpr auto get_arena() const -> Perimortem::Memory::Allocator::Arena& {
    return arena;
  }
  constexpr auto get_source_name() const -> Perimortem::Core::View::Bytes {
    return source_name;
  }
  constexpr auto get_import_name() const -> Perimortem::Core::View::Bytes {
    return import_name;
  }
  constexpr auto get_type() const -> Ttx::Type* { return type; }

  auto publish(
      Ttx::Type& type,
      Perimortem::Core::View::Bytes import_name =
          Perimortem::Core::View::Bytes()) -> void {
    this->type = &type;
    this->import_name = import_name;
  }

  auto resolve_type(Perimortem::Core::View::Bytes name) const
      -> const Ttx::Type* {
    Count segment_start = 0;
    const Ttx::Type* type = nullptr;
    for (Count name_index = 0; name_index <= name.get_size(); name_index++) {
      Bool at_end = name_index == name.get_size();
      Bool at_type_access = !at_end && name_index + 1 < name.get_size() &&
                            name[name_index] == ':' &&
                            name[name_index + 1] == ':';
      if (!at_end && !at_type_access) {
        continue;
      }

      auto segment = name.slice(segment_start, name_index - segment_start);
      if (segment.is_empty()) {
        return nullptr;
      }

      type = type == nullptr ? resolve_root_type(segment)
                             : type->find_type(segment);
      if (type == nullptr) {
        return nullptr;
      }

      segment_start = name_index + (at_type_access ? 2 : 1);
      if (at_type_access) {
        name_index++;
      }
    }

    return type;
  }

 private:
  auto resolve_root_type(Perimortem::Core::View::Bytes name) const
      -> const Ttx::Type* {
    for (Count import_index = 0; import_index < imports.get_size();
         import_index++) {
      if (imports[import_index].get_name() == name) {
        return imports[import_index].get_type();
      }
    }

    return Ttx::Core::Types::find_type(name);
  }

  Perimortem::Memory::Allocator::Arena& arena;
  Perimortem::Core::View::Bytes source_name;
  Perimortem::Core::View::Bytes import_name;
  Perimortem::Core::View::Vector<Import> imports;
  Ttx::Type* type = nullptr;
};

}  // namespace Tetrodotoxin::Isa
