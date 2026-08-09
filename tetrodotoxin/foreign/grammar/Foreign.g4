// Perimortem Engine
// Copyright © Matt Kaes
//
// Foreign is an embedded grammar fragment, not an installed source Dialect.
// The standalone entry exists only so this owner can be checked and discussed
// without first choosing a CPU capable parent grammar.

parser grammar Foreign;

options {
  tokenVocab = TTXLexer;
}

import Tetrodotoxin;

foreignPrototype
    : foreignBlock EOF
    ;

foreignBlock
    : documentation? attribute* FOREIGN STRING SCOPE_START
      documentedForeignDeclaration* SCOPE_END
    ;

documentedForeignDeclaration
    : documentation? DISABLED? attribute* foreignDeclaration
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
