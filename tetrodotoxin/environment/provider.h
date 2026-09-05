// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TETRODOTOXIN_ENVIRONMENT_PROVIDER_H
#define TETRODOTOXIN_ENVIRONMENT_PROVIDER_H

#include "tetrodotoxin/terminal/provider.h"
#include "ttx/abi.h"

// Environment lends this view only while the retained source production keeps
// the child Workspace alive. A Terminal follows root() for semantic queries and
// records the revisions it actually consumes in its ProductionClosure.
struct tetrodotoxin_workspace_view_ops {
  ttx_abi_header header;
  ttx_abstract(TTX_CALL* root)(tetrodotoxin_workspace_view_self*);
  void(TTX_CALL* visit_revisions)(
      tetrodotoxin_workspace_view_self*,
      tetrodotoxin_closure_authority_sink);
};

// A Terminal proves this relationship before treating its graph input as one
// Environment-owned semantic island. The requirement exposes no native
// Workspace representation; richer operations remain separate typed provider
// contracts.
TTX_EXTERN_C TTX_EXPORT ttx_abstract TTX_CALL
    tetrodotoxin_workspace_requirement(void);

#endif
