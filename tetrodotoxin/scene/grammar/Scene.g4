// Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors
//
// Canonical Scene source shape. Scene reuses Library declaration shapes but
// owns signals, lifecycle roles, hosted graphics state, and every body.

parser grammar Scene;

options {
  tokenVocab = TTXLexer;
}

import Library;

sceneSource
    : documentation DIALECT DEFINE SCENE_DIALECT END_STATEMENT
      sceneSourceDeclaration* EOF
    ;

sceneSourceDeclaration
    : documentedLibraryDeclaration
    | documentedSceneExtension
    | documentation? usingDeclaration
    | foreignBlock
    ;

documentedLibraryDeclaration
    : definition libraryDefinition
    ;

documentedSceneExtension
    : documentation? sceneExtension
    ;

sceneExtension
    : signalDeclaration
    | lifecycleRoleDeclaration
    ;

signalDeclaration
    : SIGNAL addressableName (DEFINE typeReference)? END_STATEMENT
    ;

lifecycleRoleDeclaration
    : SCENE_DIALECT lifecycleRole functionSignature block
    ;

// Scene overrides Library's Block composition so nested Library control flow
// can contain the one Scene-owned statement without teaching Library about
// Signals or a generic statement extension registry.
block
    : SCOPE_START sceneStatement* SCOPE_END
    ;

sceneStatement
    : emissionStatement
    | statement
    ;

emissionStatement
    : EMIT addressableName parenthesizedPack? END_STATEMENT
    ;

lifecycleRole
    : PREPARE
    | PAUSE
    | RESUME
    | UPDATE
    | RELEASE
    ;
