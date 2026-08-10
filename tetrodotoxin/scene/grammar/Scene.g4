// Perimortem Engine
// Copyright © Matt Kaes
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
    | documentation? foreignBlock
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

lifecycleRole
    : PREPARE
    | PAUSE
    | RESUME
    | UPDATE
    | RELEASE
    ;
