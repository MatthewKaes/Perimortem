// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include <pthread.h>

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/static/vector.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"

namespace Tetrodotoxin::Lsp::Server {

class Documents {
 public:
  auto upsert(
      Perimortem::Core::View::Bytes uri,
      Perimortem::Core::View::Bytes source) -> void;
  auto erase(Perimortem::Core::View::Bytes uri) -> void;
  auto get_text(Perimortem::Core::View::Bytes uri) const
      -> Perimortem::Memory::Dynamic::Bytes;

 private:
  class Record {
   public:
    Bool active = False;
    Perimortem::Memory::Dynamic::Bytes uri;
    Perimortem::Memory::Dynamic::Bytes text;
  };

  auto find(Perimortem::Core::View::Bytes uri) const -> Count;

  mutable pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
  Perimortem::Core::Static::Vector<Record, 64> records;
};

}  // namespace Tetrodotoxin::Lsp::Server
