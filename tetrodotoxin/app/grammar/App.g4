// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors
//
// Canonical App source shape. App owns startup and lifecycle policy while
// selected Types, Callables, Scenes, and Resources keep their real identities.

parser grammar App;

options {
  tokenVocab = TTXLexer;
}

import Tetrodotoxin;

appSource
    : documentation DIALECT DEFINE APP_DIALECT END_STATEMENT
      (sourceImport | packageImport)* documentedAppDeclaration* EOF
    ;

documentedAppDeclaration
    : documentation? appDeclaration
    ;

appDeclaration
    : runtimeDeclaration
    | lifecycleDeclaration
    ;

runtimeDeclaration
    : RUNTIME ASSIGN startupProfile (runtimeBody | END_STATEMENT)
    ;

startupProfile
    : WINDOWED_PROFILE
    | TERMINAL_PROFILE
    | HEADLESS_PROFILE
    ;

runtimeBody
    : SCOPE_START runtimeSetting* SCOPE_END
    ;

runtimeSetting
    : ADDRESS addressableName ASSIGN appValue (PACK | END_STATEMENT)
    ;

appValue
    : literal
    | typeReference
    | appPack
    ;

appPack
    : PACKING_START runtimeSetting* PACKING_END
    ;

lifecycleDeclaration
    : LIFECYCLE ASSIGN
      (PROGRAM_LIFETIME programLifecycle | SCENE_DIALECT sceneLifecycle)
    ;

programLifecycle
    : SCOPE_START START callableSelection (PACK | END_STATEMENT) SCOPE_END
    ;

callableSelection
    : typeRoute CALL addressableName
    ;

sceneLifecycle
    : SCOPE_START initialScene transitionMapping* SCOPE_END
    ;

initialScene
    : INITIAL typeRoute END_STATEMENT
    ;

transitionMapping
    : ON signalSelection transition END_STATEMENT
    ;

signalSelection
    : typeRoute ADDRESS addressableName
    ;

transition
    : REPLACE typeRoute
    | PUSH typeRoute
    | POP
    | EXIT
    ;
