// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/lsp/server/documents.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Lsp;

auto Server::Documents::find(View::Bytes uri) const -> Count {
  for (Count i = 0; i < records.get_size(); i++) {
    if (records[i].active && records[i].uri == uri) {
      return i;
    }
  }

  return Count(-1);
}

auto Server::Documents::upsert(View::Bytes uri, View::Bytes source) -> void {
  if (uri.is_empty()) {
    return;
  }

  pthread_mutex_lock(&mutex);
  Count slot = find(uri);

  if (slot == Count(-1)) {
    for (Count i = 0; i < records.get_size(); i++) {
      if (!records[i].active) {
        slot = i;
        records[i].active = True;
        records[i].uri = uri;
        break;
      }
    }
  }

  if (slot != Count(-1)) {
    records[slot].text = source;
  }
  pthread_mutex_unlock(&mutex);
}

auto Server::Documents::erase(View::Bytes uri) -> void {
  pthread_mutex_lock(&mutex);
  Count slot = find(uri);

  if (slot != Count(-1)) {
    records[slot].active = False;
    records[slot].uri.clear();
    records[slot].text.clear();
  }
  pthread_mutex_unlock(&mutex);
}

auto Server::Documents::get_text(View::Bytes uri) const -> Dynamic::Bytes {
  pthread_mutex_lock(&mutex);
  Count slot = find(uri);

  if (slot == Count(-1)) {
    pthread_mutex_unlock(&mutex);
    return Dynamic::Bytes();
  }

  Dynamic::Bytes text = records[slot].text;
  pthread_mutex_unlock(&mutex);
  return text;
}
