// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/language/monograph.hpp"
#include "tetrodotoxin/language/provider.h"

namespace Tetrodotoxin::Language {

// A native Dialect supplies the same provider operations as a foreign frontend.
// Its Cursor and Arena remain private construction tools. The returned source
// graph owns those tools' lasting output, including partial source services.
class Dialect : public Ttx::Abstract {
 public:
  explicit Dialect(Perimortem::Core::View::Bytes name);
  virtual ~Dialect() = default;
  virtual auto interpret(
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Documentation& documentation,
      const Ttx::Lexical::Anchor& source_anchor,
      ttx_abstract context) -> Perimortem::Core::Option<Monograph&> = 0;
  auto get_name() const -> Perimortem::Core::View::Bytes override;
  auto get_provider() -> tetrodotoxin_dialect_provider;
  virtual void produce(
      const Monograph& monograph,
      ttx_context context,
      tetrodotoxin_production_result result) const;

 protected:
  auto negotiate(ttx_abstract requirement) const
      -> ttx_interface_relation override;

 private:
  const Perimortem::Core::View::Bytes name;
};

}  // namespace Tetrodotoxin::Language
