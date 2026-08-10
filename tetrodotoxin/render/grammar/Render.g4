// Perimortem Engine
// Copyright © Matt Kaes
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
    : documentation? attribute* renderDeclaration
    ;

renderDeclaration
    : renderAliasDeclaration
    | renderValueDeclaration
    | renderResourceDeclaration
    | renderStageDeclaration
    | renderStructureDeclaration
    ;

renderAliasDeclaration
    : visibility typeName DEFINE ALIAS ASSIGN typeReference END_STATEMENT
    ;

renderValueDeclaration
    : visibility (CONST | PUSH)? addressableName DEFINE typeReference
      END_STATEMENT
    ;

renderResourceDeclaration
    : visibility RESOURCE addressableName DEFINE typeReference END_STATEMENT
    ;

renderStageDeclaration
    : visibility STAGE typeName renderStageSignature END_STATEMENT
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
    : visibility typeName DEFINE STRUCT SCOPE_START
      documentedRenderDeclaration* SCOPE_END
    ;
