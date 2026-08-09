// Perimortem Engine
// Copyright © Matt Kaes
//
// Render dialect grammar prototype. Attributes carry bindings, locations,
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
    : documentation? DISABLED? attribute* renderDeclaration
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
    : visibility STAGE typeName functionSignature END_STATEMENT
    ;

renderStructureDeclaration
    : visibility typeName DEFINE STRUCT SCOPE_START
      documentedRenderDeclaration* SCOPE_END
    ;
