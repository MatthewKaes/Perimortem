// Perimortem Engine
// Copyright © Matt Kaes
//
// Foreign is an embedded grammar fragment, not an installed source Dialect.
// The standalone entry lets the fragment be read without first choosing a CPU
// capable parent grammar.

parser grammar Foreign;

options {
  tokenVocab = TTXLexer;
}

import Tetrodotoxin;

foreignFragment
    : foreignBlock EOF
    ;

foreignBlock
    : documentation? FOREIGN STRING SCOPE_START
      documentedForeignDeclaration* SCOPE_END
    ;

documentedForeignDeclaration
    : documentation? foreignDeclaration
    ;

foreignDeclaration
    : foreignFieldDeclaration
    | foreignFunctionDeclaration
    ;

foreignFieldDeclaration
    : visibility (CONST | STATE) addressableName DEFINE typeReference
      END_STATEMENT
    ;

foreignFunctionDeclaration
    : visibility FUNC addressableName functionSignature END_STATEMENT
    ;
