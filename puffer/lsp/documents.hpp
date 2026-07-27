// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include <pthread.h>

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/static/vector.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"

#include "puffer/lsp/document.hpp"

namespace Puffer::Lsp {

class Documents {
 public:
  auto upsert(
      Perimortem::Core::View::Bytes uri,
      Perimortem::Core::View::Bytes source) -> void;
  auto erase(Perimortem::Core::View::Bytes uri) -> void;
  auto get_text(Perimortem::Core::View::Bytes uri) const
      -> Perimortem::Memory::Dynamic::Bytes;

 private:
  auto find(Perimortem::Core::View::Bytes uri) const -> Count;

  mutable pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
  Perimortem::Core::Static::Vector<Document, 64> records;
};

}  // namespace Puffer::Lsp
