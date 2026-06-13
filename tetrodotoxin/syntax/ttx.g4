// Perimortem Engine
// Copyright © Matt Kaes
//
// ANTLR reference grammar for TTX.
//
// The shipped parser is hand-written. This file is intentionally a compact
// external spec for syntax drift checks, not the production parser.

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
    : Library
    | Shader
    | Entity
    ;

importDecl
    : Import Type Define packageKind Assign importSource EndStatement
    ;

importSource
    : importFileSource
    | typeRefPath
    ;

importFileSource
    : GroupStart AddressOp Addressable Assign String GroupEnd
    ;

member
    : commentBlock? Disabled? attribute* memberDecl
    ;

memberDecl
    : External sigil? Func Addressable function
    | sigil? Func Addressable function
    | sigil? definitionName TypedAssign expr EndStatement
    | sigil? definitionName Define Enum pack EndStatement
    | sigil? definitionName Define scopeBuiltin scopeBody
    | sigil? definitionName Define typeRef (Assign expr)? EndStatement
    ;

definitionName
    : Type
    | Addressable
    ;

scopeBuiltin
    : Shader
    | Object
    | Struct
    | Foreign
    ;

scopeBody
    : ScopeStart member* ScopeEnd
    ;

sigil
    : Public
    | ConstPublic
    | Dynamic
    | ConstDynamic
    | Hidden
    | ConstHidden
    | Temporary
    | ConstTemporary
    ;

attribute
    : Attribute pack?
    ;

function
    : layout CallOp layout (block | EndStatement)
    ;

layout
    : typeRef
    | GroupStart layoutFields? PackingOp? GroupEnd
    ;

layoutFields
    : namedLayoutField (PackingOp namedLayoutField)*
    | typeRef (PackingOp typeRef)*
    ;

namedLayoutField
    : attribute* AddressOp Addressable Define typeRef
    ;

block
    : ScopeStart statement* ScopeEnd
    ;

statement
    : commentBlock
    | conditionalStmt
    | forStmt
    | whileStmt
    | matchStmt
    | returnStmt
    | declarationStmt
    | assignmentStmt
    | expressionStmt
    ;

conditionalStmt
    : (If | CompileIf) pack block (Else (conditionalStmt | block))?
    ;

forStmt
    : For layout In expr block
    ;

whileStmt
    : While pack block
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

returnStmt
    : Return expr? EndStatement
    ;

declarationStmt
    : sigil definitionName Define typeRef (Assign expr)? EndStatement
    | sigil definitionName TypedAssign expr EndStatement
    ;

assignmentStmt
    : assignChain assignOp expr EndStatement
    ;

assignOp
    : Assign
    | AddAssign
    | SubAssign
    ;

assignChain
    : Addressable assignSuffix*
    | Self assignSuffix+
    | Package assignSuffix+
    ;

assignSuffix
    : AddressOp (Addressable | Type)
    | IndexStart indexContent IndexEnd
    | SwizzleOp swizzleContent? IndexEnd
    ;

expressionStmt
    : expr EndStatement
    ;

expr
    : rangeExpr
    ;

rangeExpr
    : orExpr (RangeOp orExpr)?
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
    : (NotOp | SubOp) unaryExpr
    | postfixExpr
    ;

postfixExpr
    : primaryExpr postfixSuffix*
    ;

postfixSuffix
    : AddressOp (Addressable | Type)
    | CallOp methodName pack
    | IndexStart indexContent IndexEnd
    | SwizzleOp swizzleContent? IndexEnd
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
    | expr (PackingOp expr)* PackingOp?
    ;

primaryExpr
    : literal
    | Addressable
    | Self
    | Package
    | Discard
    | constructExpr
    | typeRef
    | pack
    | dataExpr
    ;

constructExpr
    : typeRefPath typeArgs? pack
    ;

pack
    : GroupStart (namedPackFields | exprList)? PackingOp? GroupEnd
    ;

namedPackFields
    : namedPackField (PackingOp namedPackField)*
    ;

namedPackField
    : AddressOp Addressable Assign expr
    ;

dataExpr
    : ScopeStart exprList? PackingOp? ScopeEnd
    ;

exprList
    : expr (PackingOp expr)*
    ;

typeRef
    : Numeric
    | typeRefPath typeArgs?
    ;

typeRefPath
    : typeRefSegment (AddressOp typeRefSegment)*
    ;

typeRefSegment
    : Type
    | Alias
    | Enum
    | Shader
    | Entity
    | Object
    | Struct
    | Foreign
    | Library
    ;

typeArgs
    : IndexStart typeRefList? PackingOp? IndexEnd
    ;

typeRefList
    : typeRef (PackingOp typeRef)*
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

Public          : 'public' ;
ConstPublic     : '@public' ;
Dynamic         : 'expose' ;
ConstDynamic    : '@expose' ;
Hidden          : 'hidden' ;
ConstHidden     : '@hidden' ;
Temporary       : 'stack' ;
ConstTemporary  : '@stack' ;
CompileIf       : '@if' ;
Import          : 'import' ;

And             : 'and' ;
Or              : 'or' ;
If              : 'if' ;
In              : 'in' ;
For             : 'for' ;
New             : 'new' ;
Case            : 'case' ;
Else            : 'else' ;
Enum            : 'Enum' ;
External        : 'external' ;
Func            : 'func' ;
Init            : 'init' ;
Self            : 'self' ;
True            : 'true' ;
Alias           : 'Alias' ;
False           : 'false' ;
Match           : 'match' ;
While           : 'while' ;
Shader          : 'Shader' ;
Entity          : 'Entity' ;
Object          : 'Object' ;
Return          : 'return' ;
Struct          : 'Struct' ;
Foreign         : 'Foreign' ;
Library         : 'Library' ;
Package         : 'package' ;

Comment         : '//' ~[\r\n]* ;
Disabled        : '/>' ;
Attribute       : '@' [a-zA-Z_] [a-zA-Z0-9_]* ;

SwizzleOp       : '.[' ;
SliceOp         : '+:' ;
RangeOp         : '...' ;
AddAssign       : '+=' ;
SubAssign       : '-=' ;
TypedAssign     : ':=' ;
LessEqOp        : '<=' ;
GreaterEqOp     : '>=' ;
CmpOp           : '==' ;
NotEqOp         : '!=' ;
CallOp          : '->' ;

ScopeStart      : '{' ;
ScopeEnd        : '}' ;
GroupStart      : '(' ;
GroupEnd        : ')' ;
IndexStart      : '[' ;
IndexEnd        : ']' ;
Assign          : '=' ;
Define          : ':' ;
EndStatement    : ';' ;

AddOp           : '+' ;
SubOp           : '-' ;
DivOp           : '/' ;
MulOp           : '*' ;
ModOp           : '%' ;
LessOp          : '<' ;
GreaterOp       : '>' ;
AddressOp       : '.' ;
PackingOp       : ',' ;
NotOp           : '!' ;
AndOp           : '&' ;
OrOp            : '|' ;

Bytes           : '0x[' (HEX_DIGIT | [ \t\r\n])* ']' ;
Embedded        : '$[' ~[\]\r\n]* ']' ;
Float           : DEC_DIGIT+ '.' DEC_DIGIT+ ;
Numeric         : '0x' HEX_DIGIT+ | DEC_DIGIT+ ;
String          : '"' (ESCAPE | ~["\\\r\n])* '"' ;
Discard         : '_' ;
Addressable     : [a-z] [a-zA-Z0-9_]* ;
Type            : [A-Z] [a-zA-Z0-9_]* ;

fragment ESCAPE     : '\\' . ;
fragment HEX_DIGIT  : [0-9a-fA-F] ;
fragment DEC_DIGIT  : [0-9] ;

WS              : [ \t\r\n]+ -> skip ;
Unknown         : . ;
