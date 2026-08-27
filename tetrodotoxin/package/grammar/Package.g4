// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors
//
// Canonical Package source shape. The common source envelope consumes external
// Import Types first; Package then accepts only Library Type definitions that
// form its named export surface.

parser grammar Package;

options {
  tokenVocab = TTXLexer;
}

import Library;

packageSource
    : documentation DIALECT DEFINE PACKAGE_DIALECT END_STATEMENT
      (sourceImport | packageImport)* (definition typeDefinition)* EOF
    ;
