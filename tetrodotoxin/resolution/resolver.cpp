// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/resolution/resolver.hpp"

#include "perimortem/core/bibliotheca.hpp"
#include "perimortem/core/data.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin;

Resolution::Resolver::~Resolver() {
  for (Count source_index = 0; source_index < sources.get_size();
       source_index++) {
    if (!sources[source_index]) {
      continue;
    }

    sources[source_index]->~Source();
    Bibliotheca::remit(Data::cast<Bits_8>(sources[source_index]));
  }
}

auto Resolution::Resolver::find(View::Bytes source_path) const -> Source* {
  for (Count source_index = 0; source_index < sources.get_size();
       source_index++) {
    if (sources[source_index] &&
        sources[source_index]->get_path() == source_path) {
      return sources[source_index];
    }
  }

  return nullptr;
}

auto Resolution::Resolver::load(View::Bytes source_path) -> Source* {
  if (Source* source = find(source_path)) {
    return source;
  }

  auto* source = new (Bibliotheca::check_out(sizeof(Source)).ptr)
      Source(source_path);
  sources.insert(source);
  return source;
}

auto Resolution::Resolver::resolve(View::Bytes source_path) -> Bool {
  return load(source_path) != nullptr;
}
