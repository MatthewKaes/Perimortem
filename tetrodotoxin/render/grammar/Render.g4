// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors
//
// Canonical Render source shape. Attributes carry bindings, locations,
// builtins, and capabilities as separate facts instead of one opaque policy.

parser grammar Render;

options {
  tokenVocab = TTXLexer;
}

import Tetrodotoxin;

renderSource
    : documentation DIALECT DEFINE RENDER_DIALECT END_STATEMENT
      documentedRenderDeclaration* EOF
    ;

documentedRenderDeclaration
    : definition renderDeclaration
    ;

renderDeclaration
    : renderAliasDeclaration
    | renderValueDeclaration
    | renderResourceDeclaration
    | renderStageDeclaration
    | renderStructureDeclaration
    ;

renderAliasDeclaration
    : ALIAS ASSIGN typeReference END_STATEMENT
    ;

renderValueDeclaration
    : typeReference END_STATEMENT
    ;

renderResourceDeclaration
    : (PUSH | RESOURCE) typeReference END_STATEMENT
    ;

renderStageDeclaration
    : STAGE renderStageSignature END_STATEMENT
    ;

renderStageSignature
    : renderParameterLayout CALL renderResultLayout
    ;

renderParameterLayout
    : typeReference
    | BRACKET_START renderParameterEntries? PACK? BRACKET_END
    ;

renderParameterEntries
    : renderNamedLayoutSlot (PACK renderNamedLayoutSlot)*
    | typeReference (PACK typeReference)*
    ;

renderResultLayout
    : typeReference
    | BRACKET_START renderResultEntries? PACK? BRACKET_END
    ;

renderResultEntries
    : renderNamedLayoutSlot (PACK renderNamedLayoutSlot)*
    | typeReference (PACK typeReference)*
    ;

renderNamedLayoutSlot
    : attribute* ADDRESS addressableName DEFINE typeReference
    ;

renderStructureDeclaration
    : STRUCT SCOPE_START
      documentedRenderDeclaration* SCOPE_END
    ;
