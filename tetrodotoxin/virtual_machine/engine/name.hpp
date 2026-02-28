// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "types/abstract.hpp"

#include "types/empty.hpp"

namespace Tetrodotoxin::Types {

class Name : public Abstract {
 public:
  static constexpr uint32_t uuid = 0x61D5666f;
  constexpr auto get_name() const
      -> const Perimortem::Memory::View::Bytes override {
    return name;
  };
  constexpr auto get_doc() const
      -> const Perimortem::Memory::View::Bytes override {
    return "A TTX program including any required libraries.";
  };
  constexpr auto get_uuid() const -> uint32_t override { return uuid; }
  constexpr auto get_usage() const -> Usage override { return usage; }
  auto get_size() const -> uint32_t override { return resolve().get_size(); }
  auto resolve() const -> const Abstract& override { return type.resolve(); }

  auto resolve_context(const Perimortem::Memory::View::Bytes name) const
      -> const Abstract& override {
    return resolve().resolve_context(name);
  }

  // Names are only context and do not own a scope.
  auto resolve_scope(const Perimortem::Memory::View::Bytes name) const
      -> const Abstract& override {
    return Empty::instance();
  }

  // TODO: We could have names know about their host context but for now this
  // doesn't seem useful.
  auto resolve_host() const -> const Abstract& override {
    return Empty::instance();
  }

  virtual auto expand_scope(
      const std::function<void(const Perimortem::Memory::View::Bytes,
                               const Abstract* const&)>& fn) const
      -> void override {
    resolve().expand_context(fn);
  }

  Name(const Perimortem::Memory::View::Bytes doc,
       const Perimortem::Memory::View::Bytes name,
       Abstract& type,
       Usage usage)
      : doc(doc), name(name), type(type), usage(usage) {};

  const Perimortem::Memory::View::Bytes doc;
  const Perimortem::Memory::View::Bytes name;
  const Abstract& type;
  const Usage usage;
};

}  // namespace Tetrodotoxin::Types