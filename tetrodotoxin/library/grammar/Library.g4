// Perimortem Engine
// Copyright © Matt Kaes
//
// Library dialect grammar prototype. Source adds extension declarations around
// the same recursive declaration grammar used by every authored Structure.
// Alias declarations are Type declarations and never Field declarations.

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
    : documentation? DISABLED? attribute* librarySourceDeclaration
    ;

librarySourceDeclaration
    : usingDeclaration
    | libraryDeclaration
    | foreignBlock
    ;

usingDeclaration
    : USING typeRoute END_STATEMENT
    ;

documentedLibraryDeclaration
    : documentation? DISABLED? attribute* libraryDeclaration
    ;

libraryDeclaration
    : PUBLIC publicDeclaration
    | PRIVATE privateDeclaration
    | EXPOSE exposedFieldDeclaration
    ;

publicDeclaration
    : typeDeclaration
    | functionDeclaration
    | CONST? fieldDefinition
    ;

privateDeclaration
    : typeDeclaration
    | functionDeclaration
    | fieldWritability? fieldDefinition
    ;

exposedFieldDeclaration
    : STATE fieldDefinition
    ;

typeDeclaration
    : typeName DEFINE typeDefinition
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
    : SCOPE_START documentedLibraryDeclaration* SCOPE_END
    ;

fieldDefinition
    : addressableName DEFINE
      (typeReference (ASSIGN expression)? | ASSIGN expression) END_STATEMENT
    ;

functionDeclaration
    : FUNC addressableName functionSignature block
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
    | expressionStatement
    ;

conditionalStatement
    : IF argumentPack block (ELSE (conditionalStatement | block))?
    ;

forStatement
    : FOR parameterLayout IN expression block
    ;

whileStatement
    : WHILE argumentPack block
    ;

matchStatement
    : MATCH expression SCOPE_START matchCase* SCOPE_END
    ;

matchCase
    : CASE (DISCARD | expression) DEFINE block
    ;

returnStatement
    : RETURN expression? END_STATEMENT
    ;

continueStatement
    : CONTINUE END_STATEMENT
    ;

breakStatement
    : BREAK END_STATEMENT
    ;

localDeclaration
    : fieldWritability addressableName DEFINE
      (typeReference (ASSIGN expression)? | ASSIGN expression) END_STATEMENT
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
    : ADDRESS (addressableName | typeName)
    | BRACKET_START expression BRACKET_END
    | SWIZZLE swizzleSelection? BRACKET_END
    ;

expressionStatement
    : expression END_STATEMENT
    ;

expression
    : rangeExpression
    ;

rangeExpression
    : orExpression (RANGE orExpression)?
    ;

orExpression
    : andExpression ((OR | OR_OP) andExpression)*
    ;

andExpression
    : equalityExpression ((AND | AND_OP) equalityExpression)*
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
    : ADDRESS (addressableName | typeName)
    | CALL (addressableName | typeName) argumentPack
    | BRACKET_START expression BRACKET_END
    | SWIZZLE swizzleSelection? BRACKET_END
    | VALUE_ACCESS expression (PACK expression)? BRACKET_END
    ;

swizzleSelection
    : expression (PACK expression)* PACK?
    ;

primaryExpression
    : literal
    | addressableName
    | SELF
    | DISCARD
    | constructionExpression
    | typeReference
    | argumentPack
    | dataExpression
    ;

constructionExpression
    : typeReference argumentPack
    ;

argumentPack
    : PACKING_START (namedArguments | expressionList)? PACK? PACKING_END
    ;

namedArguments
    : namedArgument (PACK namedArgument)*
    ;

namedArgument
    : ADDRESS addressableName ASSIGN expression
    ;

dataExpression
    : SCOPE_START expressionList? PACK? SCOPE_END
    ;

expressionList
    : expression (PACK expression)*
    ;
