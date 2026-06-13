// Perimortem Engine
// Copyright © Matt Kaes
//
// TTX grammar written in ANTLR4 notation.
//
// This is NOT the parser we ship with Tetrodotoxin, but it is a formal version of
// the language so we have something boring and external to compare against when
// changing the builtin Tetrodotoxin parser.
//
// To regenerate the ANTLR reference parser use:
//    antlr4 -o /tmp/ttx-antlr tetrodotoxin/syntax/ttx.g4
//    javac -cp /usr/share/java/antlr-4.13.2-complete.jar /tmp/ttx-antlr/tetrodotoxin/syntax/ttx*.java

// To run against sample TTX in the code base use:
//    java -cp /usr/share/java/antlr-4.13.2-complete.jar:/tmp/ttx-antlr/tetrodotoxin/syntax \
//      org.antlr.v4.gui.TestRig ttx program validation/data/ttx/png.ttx
//
// The ANTLR version is useful because it's used as a stable reference. 
// If this file can't parse any checked in TTX then either the grammar is
// stale or the source syntax has drifted. That doesn't mean we should use
// the generated parser in production though. TTX was intentionally designed
// to be fast to parse and tokenize by hand literally:
//
// - PascalCase and snake_case split type and value names in the lexer.
// - `.[`, `+:`, `0x[`, and `...` are real tokens instead of parser tricks.
// - Packs, attributes, and layouts map directly to the AST.
// - The lexer NEVER errors and any stream produces exactly 1 correct tokenization
//   in a single pass, leaving the parser more room for reporting user errors.
//
// With those rules a custom parser can skip ANTLR's parse tree, report errors
// in TTX terms, and avoid a pile of generated code we don't really want to
// maintain around.
//
// Places where this file is intentionally a little ahead of the C++ parser:
// - Positional function parameters (`Type`) are documented here, but the
//   current parser accepts only named parameters (`.name : Type`).
// - `@comptime if` is documented here. The parser already handles the lower
//   level `@if(...) { ... }` compile conditional.
//
// A few lexer details are load bearing:
// - Package kinds like `Library` and `Shader` are Type tokens. Validation
//   happens after lexing.
// - `9.` is avoided in this grammar even if the tokenizer can read it. It is
//   too easy to confuse with ranges and member access.
// - `_` is the discard token. `_name` is not a valid identifier.
// - `&` and `|` are reserved. Use `and` / `or` for logical operators and method
//   calls for bitwise operations.
// - Keywords are mapped to unique token types during the lexer meaning the parser
//   will never see an identifier or type that aliases a keyword.

grammar ttx;

program
    : commentBlock? packageDecl importDecl* member* EOF
    ;

commentBlock
    : Comment+
    ;

packageDecl
    : Package Define packageKind EndStatement
    ;

packageKind
    : Type
    ;

importDecl
    : Import Type Define packageKind Assign importSource EndStatement
    ;

importSource
    : importFileSource
    | qualifiedTypePath
    ;

importFileSource
    : GroupStart AddressOp Addressable Assign String GroupEnd
    ;

qualifiedTypePath
    : Type (AddressOp Type)*
    ;

member
    : commentBlock? Disabled? attribute* definition
    ;

definition
    : enumVariant
    | sigil? definitionName Define typeRef definitionRhs
    ;

definitionName
    : Addressable
    | Type
    ;

enumVariant
    : Type Assign expr EndStatement
    ;

sigil
    : Constant
    | Detail
    | Public
    | Dynamic
    | Hidden
    | Temporary
    ;

definitionRhs
    : EndStatement
    | Assign scopeBody
    | Assign funcInit
    | Assign generatorExpr EndStatement
    | Assign expr EndStatement
    ;

scopeBody
    : ScopeStart scopeItem* ScopeEnd
    ;

scopeItem
    : member
    ;

attribute
    : Attribute attributeArgs?
    ;

attributeArgs
    : GroupStart packFields? PackingOp? GroupEnd
    ;

typeRef
    : Type (AddressOp Type)* typeArgs?
    ;

typeArgs
    : IndexStart typeArgList? PackingOp? IndexEnd
    ;

typeArgList
    : typeArg (PackingOp typeArg)*
    ;

typeArg
    : genericTypeParam
    | expr
    ;

genericTypeParam
    : Addressable Define typeRef
    ;

funcInit
    : paramPack CallOp layoutDef (block | EndStatement)
    ;

paramPack
    : GroupStart paramList? PackingOp? GroupEnd
    ;

paramList
    : param (PackingOp param)*
    ;

param
    : attribute* AddressOp Addressable Define typeRef
    | typeRef
    ;

layoutDef
    : typeRef
    | GroupStart GroupEnd
    | GroupStart namedLayoutList PackingOp? GroupEnd
    | GroupStart unnamedLayoutList PackingOp? GroupEnd
    ;

namedLayoutList
    : namedLayoutField (PackingOp namedLayoutField)*
    ;

namedLayoutField
    : attribute* AddressOp Addressable Define typeRef
    ;

unnamedLayoutList
    : typeRef (PackingOp typeRef)*
    ;

block
    : ScopeStart statement* ScopeEnd
    ;

statement
    : commentBlock
    | ifStmt
    | forStmt
    | whileStmt
    | matchStmt
    | compileConditional
    | returnStmt
    | varDecl
    | assignStmt
    | exprStmt
    ;

ifStmt
    : If GroupStart expr GroupEnd block (Else (ifStmt | block))?
    ;

forStmt
    : For GroupStart loopBinding GroupEnd In expr block
    ;

loopBinding
    : (Addressable | Discard) Define typeRef
    ;

whileStmt
    : While GroupStart expr GroupEnd block
    ;

matchStmt
    : Match expr ScopeStart matchCase* ScopeEnd
    ;

matchCase
    : Case matchPattern Define block
    ;

matchPattern
    : Discard
    | expr
    ;

compileConditional
    : Attribute attributeArgs block
    | Attribute If expr block (Else block)?
    ;

returnStmt
    : Return expr? EndStatement
    ;

varDecl
    : sigil definitionName Define typeRef (Assign expr)? EndStatement
    | sigil definitionName TypedAssign expr EndStatement
    | sigil splatPattern Assign expr EndStatement
    ;

splatPattern
    : GroupStart splatField (PackingOp splatField)* PackingOp? GroupEnd
    ;

splatField
    : Addressable
    | Discard
    ;

assignStmt
    : expr assignOp expr EndStatement
    ;

assignOp
    : Assign
    | AddAssign
    | SubAssign
    ;

exprStmt
    : expr EndStatement
    ;

expr
    : lambdaExpr
    | rangeExpr
    ;

lambdaExpr
    : paramPack GeneratorOp (expr | block)
    ;

rangeExpr
    : orExpr (RangeOp Assign? orExpr)?
    ;

orExpr
    : andExpr (Or andExpr)*
    ;

andExpr
    : cmpExpr (And cmpExpr)*
    ;

cmpExpr
    : addExpr (cmpOp addExpr)?
    ;

cmpOp
    : CmpOp
    | NotEqOp
    | LessOp
    | GreaterOp
    | LessEqOp
    | GreaterEqOp
    ;

addExpr
    : mulExpr ((AddOp | SubOp) mulExpr)*
    ;

mulExpr
    : unaryExpr ((MulOp | DivOp | ModOp) unaryExpr)*
    ;

unaryExpr
    : NotOp unaryExpr
    | SubOp unaryExpr
    | postfixExpr
    ;

postfixExpr
    : primaryExpr postfixSuffix*
    ;

postfixSuffix
    : AddressOp (Addressable | Type)
    | CallOp methodName callArgs
    | IndexStart indexContent? IndexEnd
    | SwizzleOp swizzleContent? IndexEnd
    | callArgs
    | NotOp
    ;

methodName
    : Addressable
    | Type
    | New
    ;

indexContent
    : expr (SliceOp expr)?
    ;

swizzleContent
    : expr SliceOp expr
    | swizzleComponent (PackingOp swizzleComponent)* PackingOp?
    ;

swizzleComponent
    : Addressable
    | Numeric
    ;

primaryExpr
    : literal
    | Addressable
    | Self
    | Package
    | Discard
    | typeRef
    | packExpr
    | arrayExpr
    | generatorExpr
    ;

packExpr
    : GroupStart packFields? PackingOp? GroupEnd
    ;

callArgs
    : GroupStart packFields? PackingOp? GroupEnd
    ;

packFields
    : packField (PackingOp packField)*
    ;

packField
    : AddressOp Addressable Assign expr
    | expr
    ;

arrayExpr
    : ScopeStart exprList? PackingOp? ScopeEnd
    ;

exprList
    : expr (PackingOp expr)*
    ;

generatorExpr
    : IndexStart Addressable Define typeRef GeneratorOp block IndexEnd
    ;

literal
    : Numeric
    | Float
    | String
    | Bytes
    | Embedded
    | True
    | False
    ;

Constant     : 'frozen' ;
Detail       : 'detail' ;
Public       : 'public' ;
Dynamic      : 'expose' ;
Hidden       : 'hidden' ;
Temporary    : 'stack' ;
Import       : 'import' ;

As           : 'as' ;
And          : 'and' ;
Or           : 'or' ;
If           : 'if' ;
In           : 'in' ;
For          : 'for' ;
New          : 'new' ;
Case         : 'case' ;
Else         : 'else' ;
Enum         : 'enum' ;
Func         : 'func' ;
Init         : 'init' ;
Self         : 'self' ;
True         : 'true' ;
Alias        : 'alias' ;
False        : 'false' ;
Match        : 'match' ;
Using        : 'using' ;
While        : 'while' ;
Shader       : 'shader' ;
Entity       : 'entity' ;
Object       : 'object' ;
Return       : 'return' ;
Struct       : 'struct' ;
Library      : 'library' ;
Package      : 'package' ;

Comment      : '//' ~[\r\n]* ;
Disabled     : '/>' ;
Attribute    : '@' [a-zA-Z_] [a-zA-Z0-9_]* ;

SwizzleOp    : '.[' ;
SliceOp      : '+:' ;
RangeOp      : '...' ;
AddAssign    : '+=' ;
SubAssign    : '-=' ;
TypedAssign  : ':=' ;
LessEqOp     : '<=' ;
GreaterEqOp  : '>=' ;
CmpOp        : '==' ;
NotEqOp      : '!=' ;
CallOp       : '->' ;
GeneratorOp  : '=>' ;

ScopeStart   : '{' ;
ScopeEnd     : '}' ;
GroupStart   : '(' ;
GroupEnd     : ')' ;
IndexStart   : '[' ;
IndexEnd     : ']' ;
Assign       : '=' ;
Define       : ':' ;
EndStatement : ';' ;

AddOp        : '+' ;
SubOp        : '-' ;
DivOp        : '/' ;
MulOp        : '*' ;
ModOp        : '%' ;
LessOp       : '<' ;
GreaterOp    : '>' ;
AddressOp    : '.' ;
PackingOp    : ',' ;
NotOp        : '!' ;
AndOp        : '&' ;
OrOp         : '|' ;

Bytes
    : '0x[' (HEX_DIGIT | [ \t\r\n])* ']'
    ;

Embedded
    : '$[' ~[\]\r\n]* ']'
    ;

Float
    : DEC_DIGIT+ '.' DEC_DIGIT+
    ;

Numeric
    : '0x' HEX_DIGIT+
    | DEC_DIGIT+
    ;

String
    : '"' (ESCAPE | ~["\\\r\n])* '"'
    ;

Discard
    : '_'
    ;

Addressable
    : [a-z] [a-zA-Z0-9_]*
    ;

Type
    : [A-Z] [a-zA-Z0-9_]*
    ;

fragment ESCAPE
    : '\\' .
    ;

fragment HEX_DIGIT
    : [0-9a-fA-F]
    ;

fragment DEC_DIGIT
    : [0-9]
    ;

WS
    : [ \t\r\n]+ -> skip
    ;

Unknown
    : .
    ;
