// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors
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
      (sourceImport | packageImport)* documentedSourceDeclaration* EOF
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
    | NAMESPACE structureBody
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
    : NEW BRACKET_START typeReference BRACKET_END objectInitializerArguments?
    ;

objectInitializerArguments
    : PACKING_START (namedPackEntries | positionalPackEntries) PACK? PACKING_END
    ;

functionDefinition
    : FUNC ASSIGN functionSignature block
    ;

block
    : SCOPE_START documentedStatement* SCOPE_END
    ;

// Documentation decorates membership in the authored sequence. The selected
// Local, Pack, control owner, or nested Block remains the semantic identity.
documentedStatement
    : documentation? statement
    ;

statement
    : conditionalStatement
    | forStatement
    | whileStatement
    | matchStatement
    | returnStatement
    | continueStatement
    | breakStatement
    | localDeclaration
    | block
    | expressionStatement
    ;

conditionalStatement
    : IF pack block
      (ELSE documentation? (conditionalStatement | block))?
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
    : CASE (DISCARD | expression) DEFINE block
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

assignmentOperator
    // TTX tokenizes each complete spelling before Library selects its exact
    // semantic operator. Library never reconstructs `+=` or `-=` from parts.
    : ASSIGN
    | ADD_ASSIGN
    | SUB_ASSIGN
    ;

expressionStatement
    : expression END_STATEMENT
    ;

expression
    : assignmentExpression
    ;

// Assignment consumes the complete tighter expression on its left and parses
// its right at the same precedence. Its empty result keeps it in expression
// grammar while preventing assignment from becoming reusable value flow.
assignmentExpression
    : rangeExpression (assignmentOperator assignmentExpression)?
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
    | BRACKET_START expression (PACK expression)? BRACKET_END
    | SWIZZLE swizzleSelection? BRACKET_END
    | VALUE_ACCESS expression (PACK expression)? BRACKET_END
    | NOT
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
