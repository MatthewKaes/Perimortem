// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"

namespace Tetrodotoxin::Ttx {

class Layout {
 public:
  enum class Kind : Bits_8 {
    Invalid,
    Fluid,
    Concrete,
  };

  class Entry {
   public:
    constexpr Entry() = default;
    constexpr Entry(
        Perimortem::Core::View::Bytes name,
        Perimortem::Core::View::Bytes type_name,
        const Layout* layout,
        Count offset)
        : name(name), type_name(type_name), layout(layout), offset(offset) {}

    constexpr auto get_name() const -> Perimortem::Core::View::Bytes {
      return name;
    }

    constexpr auto get_type_name() const -> Perimortem::Core::View::Bytes {
      return type_name;
    }

    constexpr auto get_layout() const -> const Layout* { return layout; }
    constexpr auto get_offset() const -> Count { return offset; }
    constexpr auto is_named() const -> Bool { return !name.is_empty(); }

    constexpr auto operator==(const Entry& other) const -> Bool {
      return name == other.name && type_name == other.type_name &&
             offset == other.offset;
    }

   private:
    Perimortem::Core::View::Bytes name;
    Perimortem::Core::View::Bytes type_name;
    const Layout* layout = nullptr;
    Count offset = 0;
  };

  constexpr Layout() = default;
  constexpr Layout(
      Kind kind,
      Count byte_size,
      Count alignment,
      Perimortem::Core::View::Vector<Entry> entries =
          Perimortem::Core::View::Vector<Entry>())
      : kind(kind),
        byte_size(byte_size),
        alignment(alignment),
        entries(entries) {}

  constexpr auto get_kind() const -> Kind { return kind; }
  constexpr auto get_byte_size() const -> Count { return byte_size; }
  constexpr auto get_alignment() const -> Count { return alignment; }
  constexpr auto get_entries() const -> Perimortem::Core::View::Vector<Entry> {
    return entries;
  }

  constexpr auto is_valid() const -> Bool {
    return kind != Kind::Invalid && alignment != 0;
  }

  constexpr auto is_concrete() const -> Bool { return kind == Kind::Concrete; }

  constexpr auto is_fluid() const -> Bool { return kind == Kind::Fluid; }
  constexpr auto is_terminal() const -> Bool { return entries.is_empty(); }
  constexpr auto is_scalar() const -> Bool {
    return is_valid() && is_concrete() && is_terminal();
  }
  constexpr auto is_aggregate() const -> Bool {
    return is_valid() && !entries.is_empty();
  }

  constexpr auto get_field_count() const -> Count { return entries.get_size(); }

  constexpr auto field_at(Count index) const -> const Entry* {
    return index < entries.get_size() ? &entries[index] : nullptr;
  }

  constexpr auto find_field(Perimortem::Core::View::Bytes name) const
      -> const Entry* {
    for (Count i = 0; i < entries.get_size(); i++) {
      if (entries[i].get_name() == name) {
        return &entries[i];
      }
    }

    return nullptr;
  }

  constexpr auto operator==(const Layout& other) const -> Bool {
    if (kind != other.kind || byte_size != other.byte_size ||
        alignment != other.alignment ||
        entries.get_size() != other.entries.get_size()) {
      return False;
    }

    for (Count i = 0; i < entries.get_size(); i++) {
      if (!(entries[i] == other.entries[i])) {
        return False;
      }
    }

    return True;
  }

  constexpr auto operator!=(const Layout& other) const -> Bool {
    return !(*this == other);
  }

 private:
  Kind kind = Kind::Invalid;
  Count byte_size = 0;
  Count alignment = 0;
  Perimortem::Core::View::Vector<Entry> entries;
};

class Type {
 public:
  enum class Class : Bits_8 {
    Unknown,
    Void,
    Bool,
    Integer,
    Real,
    Bytes,
    String,
    Vector,
    Color,
    Sampler2D,
    View,
    Access,
    List,
    Dict,
    Action,
  };

  class Method {
   public:
    constexpr Method() = default;
    constexpr Method(
        Perimortem::Core::View::Bytes name,
        const Layout* args,
        Perimortem::Core::View::Bytes result_type_name,
        const Layout* result)
        : name(name),
          args(args),
          result_type_name(result_type_name),
          result(result) {}

    constexpr auto get_name() const -> Perimortem::Core::View::Bytes {
      return name;
    }
    constexpr auto get_args() const -> const Layout* { return args; }
    constexpr auto get_result_type_name() const
        -> Perimortem::Core::View::Bytes {
      return result_type_name;
    }
    constexpr auto get_result() const -> const Layout* { return result; }

   private:
    Perimortem::Core::View::Bytes name;
    const Layout* args = nullptr;
    Perimortem::Core::View::Bytes result_type_name;
    const Layout* result = nullptr;
  };

  constexpr Type() = default;
  constexpr Type(
      Perimortem::Core::View::Bytes name,
      Class klass,
      const Layout* layout = nullptr,
      Perimortem::Core::View::Vector<Method> methods =
          Perimortem::Core::View::Vector<Method>())
      : name(name), klass(klass), layout(layout), methods(methods) {}

  constexpr auto get_name() const -> Perimortem::Core::View::Bytes {
    return name;
  }

  constexpr auto get_class() const -> Class { return klass; }
  constexpr auto get_layout() const -> const Layout* { return layout; }
  constexpr auto get_fields() const
      -> Perimortem::Core::View::Vector<Layout::Entry> {
    return layout ? layout->get_entries()
                  : Perimortem::Core::View::Vector<Layout::Entry>();
  }
  constexpr auto get_methods() const -> Perimortem::Core::View::Vector<Method> {
    return methods;
  }

  constexpr auto is_valid() const -> Bool {
    return !name.is_empty() && klass != Class::Unknown;
  }

  constexpr auto is_numeric() const -> Bool { return klass == Class::Integer; }

  constexpr auto is_real() const -> Bool { return klass == Class::Real; }
  constexpr auto is_scalar() const -> Bool {
    return layout && layout->is_scalar();
  }
  constexpr auto is_aggregate() const -> Bool {
    return layout && layout->is_aggregate();
  }
  constexpr auto get_field_count() const -> Count {
    return layout ? layout->get_field_count() : 0;
  }
  constexpr auto get_method_count() const -> Count {
    return methods.get_size();
  }
  constexpr auto find_field(Perimortem::Core::View::Bytes field_name) const
      -> const Layout::Entry* {
    return layout ? layout->find_field(field_name) : nullptr;
  }
  constexpr auto find_method(Perimortem::Core::View::Bytes method_name) const
      -> const Method* {
    for (Count i = 0; i < methods.get_size(); i++) {
      if (methods[i].get_name() == method_name) {
        return &methods[i];
      }
    }

    return nullptr;
  }

 private:
  Perimortem::Core::View::Bytes name;
  Class klass = Class::Unknown;
  const Layout* layout = nullptr;
  Perimortem::Core::View::Vector<Method> methods;
};

using Scalar = Type;
using Field = Layout::Entry;
using Method = Type::Method;

}  // namespace Tetrodotoxin::Ttx
