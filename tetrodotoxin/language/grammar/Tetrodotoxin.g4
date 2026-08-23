// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors
//
// Parser fragments shared by concrete Tetrodotoxin dialects. Semantic result
// categories remain the responsibility of the consuming dialect even when two
// dialects reuse the same authored shape.

parser grammar Tetrodotoxin;

options {
  tokenVocab = TTXLexer;
}

documentation
    : COMMENT+
    ;

attribute
    : ATTRIBUTE (PACKING_START attributeValue PACKING_END)?
    ;

attributeValue
    : NUMERIC
    | HEX
    | FLOAT
    | STRING
    | TRUE
    | FALSE
    | SUBTRACT (NUMERIC | HEX | FLOAT)
    ;

definition
    : documentation? attribute* visibility definitionModifier* definitionName
      DEFINE
    ;

definitionModifier
    : STATE
    | CONST
    ;

definitionName
    : addressableName
    | typeName
    ;

visibility
    : PUBLIC
    | PRIVATE
    | EXPOSE
    ;

fieldWritability
    : STATE
    | CONST
    ;

typeReference
    : typeRoute genericArgumentLayout?
    ;

typeRoute
    : typeName (TYPE_ACCESS typeName)*
    ;

genericArgumentLayout
    : BRACKET_START
      (genericArgument (PACK genericArgument)* PACK?)?
      BRACKET_END
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
    | FOREIGN
    | RUNTIME
    | LIFECYCLE
    | START
    | INITIAL
    | ON
    | REPLACE
    | PUSH
    | POP
    | EXIT
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

// Parameters are an empty or Named descriptor Layout. Results admit the full
// descriptor grammar. `:` promises a slot Type; it never supplies Pack flow.
parameterLayout
    : BRACKET_START parameterEntries? PACK? BRACKET_END
    ;

parameterEntries
    : SELF (PACK namedLayoutSlot)*
    | namedLayoutSlot (PACK namedLayoutSlot)*
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
