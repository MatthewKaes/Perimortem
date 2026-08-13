// Perimortem Engine
// Copyright © Matt Kaes
//
// Canonical Library source shape. Every ordinary member begins with the shared
// Definition prefix, then the stateless Member parser selects the concrete
// owner for the remaining qualifier form.
// Source alone adds import and Foreign declarations around that same grammar.

parser grammar Library;

options {
  tokenVocab = TTXLexer;
}

import Tetrodotoxin, Foreign;

librarySource
    : documentation DIALECT DEFINE LIBRARY_DIALECT END_STATEMENT
      documentedSourceDeclaration* EOF
    ;

documentedSourceDeclaration
    : definition libraryDefinition
    | documentation? usingDeclaration
    | foreignBlock
    ;

usingDeclaration
    : USING typeRoute END_STATEMENT
    ;

libraryDefinition
    : fieldDefinition
    | typeDefinition
    | functionDefinition
    ;

typeDefinition
    : ALIAS ASSIGN typeReference END_STATEMENT
    | ENUM BRACKET_START typeReference BRACKET_END
      SCOPE_START enumerationCase* SCOPE_END
    | (STRUCT | OBJECT) structureBody
    ;

enumerationCase
    : documentation? addressableName ASSIGN signedInteger
      (END_STATEMENT | PACK)
    ;

structureBody
    : SCOPE_START (definition libraryDefinition)* SCOPE_END
    ;

fieldDefinition
    : (typeReference (ASSIGN declarationInitializer)?
      | ASSIGN declarationInitializer)
      END_STATEMENT
    ;

declarationInitializer
    : expression
    | objectInitializer
    ;

objectInitializer
    : NEW parenthesizedPack?
    ;

functionDefinition
    : FUNC ASSIGN functionSignature block
    ;

block
    : SCOPE_START statement* SCOPE_END
    ;

statement
    : documentation
    | conditionalStatement
    | forStatement
    | whileStatement
    | matchStatement
    | returnStatement
    | continueStatement
    | breakStatement
    | localDeclaration
    | assignmentStatement
    | invocationStatement
    ;

conditionalStatement
    : IF pack block
      (ELSE (conditionalStatement | block))?
    ;

forStatement
    : FOR parameterLayout IN expression block
    ;

whileStatement
    : WHILE pack block
    ;

matchStatement
    : MATCH expression SCOPE_START matchCase* SCOPE_END
    ;

matchCase
    : CASE (DISCARD | optionBindingPattern | expression) DEFINE block
    ;

// `some` and `empty` remain contextual Library spellings carried by
// ADDRESSABLE Tokens. The semantic Match owner admits only `some(name)`,
// unbound `some`, and `empty` when the input has exact Type Option[T].
optionBindingPattern
    : addressableName PACKING_START addressableName PACKING_END
    ;

returnStatement
    : RETURN pack? END_STATEMENT
    ;

continueStatement
    : CONTINUE END_STATEMENT
    ;

breakStatement
    : BREAK END_STATEMENT
    ;

localDeclaration
    : fieldWritability addressableName DEFINE
      (typeReference (ASSIGN declarationInitializer)?
      | ASSIGN declarationInitializer)
      END_STATEMENT
    ;

assignmentStatement
    : assignmentTarget assignmentOperator expression END_STATEMENT
    ;

assignmentOperator
    : ASSIGN
    | ADD_ASSIGN
    | SUB_ASSIGN
    ;

assignmentTarget
    : (addressableName | SELF) assignmentSuffix*
    ;

assignmentSuffix
    : ADDRESS addressableName
    | BRACKET_START expression BRACKET_END
    ;

invocationStatement
    : expression END_STATEMENT
    ;

expression
    : rangeExpression
    ;

rangeExpression
    : orExpression (RANGE orExpression)?
    ;

orExpression
    : andExpression (OR andExpression)*
    ;

andExpression
    : equalityExpression (AND equalityExpression)*
    ;

equalityExpression
    : comparisonExpression ((EQUAL | NOT_EQUAL) comparisonExpression)*
    ;

comparisonExpression
    : additiveExpression
      ((LESS | GREATER | LESS_EQUAL | GREATER_EQUAL) additiveExpression)*
    ;

additiveExpression
    : multiplicativeExpression ((ADD | SUBTRACT) multiplicativeExpression)*
    ;

multiplicativeExpression
    : unaryExpression ((MULTIPLY | DIVIDE | MODULO) unaryExpression)*
    ;

unaryExpression
    : (NOT | SUBTRACT) unaryExpression
    | postfixExpression
    ;

postfixExpression
    : primaryExpression postfixSuffix*
    ;

postfixSuffix
    : ADDRESS addressableName
    | CALL addressableName parenthesizedPack
    | TYPE_ACCESS typeName
    | BRACKET_START expression BRACKET_END
    | SWIZZLE swizzleSelection? BRACKET_END
    | VALUE_ACCESS expression (PACK expression)? BRACKET_END
    | QUESTION
    ;

swizzleSelection
    : addressableName (PACK addressableName)* PACK?
    ;

primaryExpression
    : literal
    | typeName
    | addressableName
    | SELF
    | parenthesizedPack
    ;

pack
    : expression
    | parenthesizedPack
    ;

// Parentheses supply Pack flow. `=` names a produced value and remains distinct
// from the `:` used by descriptor Layout slots in Tetrodotoxin.g4.
parenthesizedPack
    : PACKING_START (namedPackEntries | positionalPackEntries)? PACK?
      PACKING_END
    ;

namedPackEntries
    : namedPackEntry (PACK namedPackEntry)*
    ;

namedPackEntry
    : ADDRESS addressableName ASSIGN expression
    ;

positionalPackEntries
    : expression (PACK expression)*
    ;
