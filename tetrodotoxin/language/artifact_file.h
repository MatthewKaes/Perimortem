// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TETRODOTOXIN_LANGUAGE_ARTIFACT_FILE_H
#define TETRODOTOXIN_LANGUAGE_ARTIFACT_FILE_H

#include "ttx/abi.h"

// ArtifactFile is a Tetrodotoxin Language Interface rather than a TTX
// category. A successful witness keeps the exact candidate in the ordinary
// Interface prefix, then lends only the publication operations needed to write
// that candidate as one immutable file.
typedef struct {
  ttx_interface_ops interface;
  ttx_borrowed_bytes(TTX_CALL* route)(ttx_interface);
  uint64_t(TTX_CALL* size)(ttx_interface);
  void(TTX_CALL* visit_bytes)(ttx_interface, ttx_bytes_sink);
  uint8_t(TTX_CALL* executable)(ttx_interface);
} tetrodotoxin_artifact_file_ops;

TTX_EXTERN_C TTX_EXPORT ttx_abstract TTX_CALL
tetrodotoxin_artifact_file_requirement(void);

#endif
