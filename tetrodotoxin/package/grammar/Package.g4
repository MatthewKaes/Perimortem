// Perimortem Engine
// Copyright © Matt Kaes
//
// Canonical Package source shape. Package owns manifest coordinates and
// confined source paths while semantic routes keep the shared Type shape.

parser grammar Package;

options {
  tokenVocab = TTXLexer;
}

import Tetrodotoxin;

packageSource
    : documentation DIALECT DEFINE PACKAGE_DIALECT END_STATEMENT
      documentedDependency* documentedSource+ EOF
    ;

documentedDependency
    : documentation? dependencyDeclaration
    ;

dependencyDeclaration
    : RESOLVE typeRoute DEFINE externalPackageName ASSIGN STRING END_STATEMENT
    ;

externalPackageName
    : typeName (ADDRESS typeName)*
    ;

documentedSource
    : documentation? sourceDeclaration
    ;

sourceDeclaration
    : SOURCE typeRoute FROM STRING END_STATEMENT
    ;
