// Perimortem Engine
// Copyright © Matt Kaes
//
// Scene dialect grammar prototype. Scene reuses Library declaration shapes but
// owns child identity, signals, lifecycle roles, and the meaning of every body.

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
    | documentation? DISABLED? attribute* usingDeclaration
    | documentation? DISABLED? attribute* foreignBlock
    ;

documentedSceneExtension
    : documentation? DISABLED? attribute* sceneExtension
    ;

sceneExtension
    : childDeclaration
    | signalDeclaration
    | lifecycleRoleDeclaration
    ;

childDeclaration
    : CHILD addressableName DEFINE typeReference END_STATEMENT
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
