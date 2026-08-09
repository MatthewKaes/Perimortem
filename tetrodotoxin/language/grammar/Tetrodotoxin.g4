// Perimortem Engine
// Copyright © Matt Kaes
//
// Parser fragments shared by concrete Tetrodotoxin dialect prototypes.
// Semantic terminal categories remain the responsibility of the consuming
// dialect even when two dialects reuse the same source shape.

parser grammar Tetrodotoxin;

options {
  tokenVocab = TTXLexer;
}

documentation
    : COMMENT+
    ;

attribute
    : ATTRIBUTE attributeArguments?
    ;

attributeArguments
    : PACKING_START attributeArgument (PACK attributeArgument)* PACK? PACKING_END
    ;

attributeArgument
    : literal
    | typeReference
    | addressableName
    ;

visibility
    : PUBLIC
    | PRIVATE
    ;

fieldPublication
    : PUBLIC
    | PRIVATE
    | EXPOSE
    ;

fieldWritability
    : STATE
    | CONST
    ;

typeReference
    : typeRoute typeArguments?
    ;

typeRoute
    : typeName (TYPE_ACCESS typeName)*
    ;

typeArguments
    : BRACKET_START genericArgument (PACK genericArgument)* PACK? BRACKET_END
    ;

genericArgument
    : typeReference
    | literal
    ;

typeName
    : TYPE
    | PACKAGE_DIALECT
    | LIBRARY_DIALECT
    | APP_DIALECT
    | SCENE_DIALECT
    | RENDER_DIALECT
    | SHADER_DIALECT
    | WINDOWED_PROFILE
    | TERMINAL_PROFILE
    | HEADLESS_PROFILE
    | PROGRAM_LIFETIME
    ;

addressableName
    : ADDRESSABLE
    | USING
    | ENUM
    | STRUCT
    | OBJECT
    | FOREIGN
    | FROM
    | RUNTIME
    | LIFECYCLE
    | START
    | INITIAL
    | ON
    | REPLACE
    | PUSH
    | POP
    | EXIT
    | CHILD
    | SIGNAL
    | PREPARE
    | PAUSE
    | RESUME
    | UPDATE
    | RELEASE
    | STAGE
    | RESOURCE
    | SHADER
    ;

functionSignature
    : parameterLayout CALL resultLayout
    ;

parameterLayout
    : typeReference
    | BRACKET_START parameterEntries? PACK? BRACKET_END
    ;

parameterEntries
    : SELF (PACK namedLayoutSlot)*
    | namedLayoutSlot (PACK namedLayoutSlot)*
    | typeReference (PACK typeReference)*
    ;

resultLayout
    : typeReference
    | BRACKET_START resultEntries? PACK? BRACKET_END
    ;

resultEntries
    : namedLayoutSlot (PACK namedLayoutSlot)*
    | typeReference (PACK typeReference)*
    ;

namedLayoutSlot
    : ADDRESS addressableName DEFINE typeReference
    ;

literal
    : NUMERIC
    | HEX
    | FLOAT
    | STRING
    | BYTES
    | EMBEDDED
    | TRUE
    | FALSE
    ;

signedInteger
    : SUBTRACT? (NUMERIC | HEX)
    ;
