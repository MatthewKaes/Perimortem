// Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/static/vector.hpp"

#include "tetrodotoxin/library/language/model/callable.hpp"
#include "tetrodotoxin/library/language/parameter.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/model/documentations/comment.hpp"
#include "ttx/model/layouts/named.hpp"
#include "ttx/model/layouts/ranged.hpp"

namespace Tetrodotoxin::Library::Builtin::Object {

class Reserve : public Language::Model::Callable {
 public:
  static constexpr Perimortem::Core::View::Bytes name = "reserve"_view;

  TTX_CONTRACT(Reserve, Language::Model::Callable);

  static auto create(
      Perimortem::Memory::Allocator::Arena& domain,
      const Language::Model::Type& receiver,
      const Language::Model::Type& count,
      const Language::Model::Type& result) -> Reserve&;

  TTX_NAME(name);
  TTX_DOCUMENTATION(documentation);
  TTX_CONSTEXPR_INVALID_CONTEXT;

  constexpr auto get_parameters() const
      -> const Ttx::Concept::Layout& override {
    return parameters;
  }

  constexpr auto get_results() const -> const Ttx::Concept::Layout& override {
    return results;
  }

  auto accepts_receiver(
      const Ttx::Concept::Abstract& receiver,
      const Ttx::Concept::Abstract& host) const -> Bool override;

 private:
  Reserve(
      Language::Parameter& self,
      Language::Parameter& count,
      const Language::Model::Type& result);

  Perimortem::Core::Static::
      Vector<Ttx::Concept::Reference<const Ttx::Concept::Abstract>, 2>
          parameter_entries;
  Ttx::Model::Layouts::Named parameters;
  Ttx::Model::Layouts::Ranged results;
  static constexpr Ttx::Model::Documentations::Comment documentation{
    "Reserves at least count initialized elements and returns writable access."_view,
  };
};

}  // namespace Tetrodotoxin::Library::Builtin::Object
