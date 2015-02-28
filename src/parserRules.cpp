#include "parserRules.h"

std::string parserRules [] = {
	"Goal CompilationUnit",
	"CompilationUnit PackageDeclaration ImportDeclarations* TypeDeclaration $",
	"TypeDeclaration ClassDeclaration",
	"PackageDeclaration package Name ;",
	"PackageDeclaration",
	"ImportDeclarations* ImportDeclarations",
	"ImportDeclarations*",
	"ImportDeclarations ImportDeclaration",
	"ImportDeclarations ImportDeclarations ImportDeclaration",
	"ImportDeclaration SingleTypeImportDeclaration",
	"ImportDeclaration TypeImportOnDemandDeclaration",
	"SingleTypeImportDeclaration import Name ;",
	"TypeImportOnDemandDeclaration import Name . * ;",
	"Name SimpleName",
	"Name QualifiedName",
	"SimpleName Identifier",
	"QualifiedName Name . Identifier",
	"Identifier ID",
	"ClassDeclaration Modifiers class Identifier Super Interfaces ClassBody",
	"Modifiers Modifier",
	"Modifiers Modifiers Modifier",
	"Modifier public",
	"Modifier abstract",
	"Modifier final",
	"Super extends ClassType",
	"Super",
	"ClassType ClassOrInterfaceType",
	"ClassOrInterfaceType Name",
	"Interfaces implements InterfaceTypeList",
	"Interfaces",
	"InterfaceTypeList InterfaceType",
	"InterfaceTypeList InterfaceTypeList , InterfaceType",
	"InterfaceType ClassOrInterfaceType",
	"ClassBody { ClassBodyDeclarations* }",
	"ClassBodyDeclarations* ClassBodyDeclarations",
	"ClassBodyDeclarations*",
	"ClassBodyDeclarations ClassBodyDeclaration",
	"ClassBodyDeclarations ClassBodyDeclarations ClassBodyDeclaration",
	"ClassBodyDeclaration ClassMemberDeclaration",
	"ClassBodyDeclaration ConstructorDeclaration",
	"ClassMemberDeclaration FieldDeclaration",
	"ClassMemberDeclaration MethodDeclaration",
	"FieldDeclaration MemberModifiers Type VariableDeclarator ;",
	"MemberModifiers MemberModifier",
	"MemberModifiers MemberModifiers MemberModifier",
	"MemberModifier public",
	"MemberModifier abstract",
	"MemberModifier static",
	"MemberModifier native",
	"MemberModifier protected",
	"MemberModifier final",
	"Type PrimitiveType",
	"Type ReferenceType",
	"PrimitiveType byte",
	"PrimitiveType short",
	"PrimitiveType int",
	"PrimitiveType char",
	"PrimitiveType boolean",
	"ReferenceType ClassOrInterfaceType",
	"ReferenceType ArrayType",
	"ArrayType PrimitiveType [ ]",
	"ArrayType Name [ ]",
	"VariableDeclarator Identifier",
	"VariableDeclarator Identifier = Expression",
	"Expression ConditionalExpression",
	"Expression Assignment",
	"Assignment LeftHandSide = AssignmentExpression",
	"LeftHandSide Name",
	"LeftHandSide FieldAccess",
	"LeftHandSide ArrayAccess",
	"FieldAccess Primary . Identifier",
	"Primary PrimaryNoNewArray",
	"Primary ArrayCreationExpression",
	"PrimaryNoNewArray Literal",
	"PrimaryNoNewArray this",
	"PrimaryNoNewArray ( Expression )",
	"PrimaryNoNewArray ClassInstanceCreationExpression",
	"PrimaryNoNewArray FieldAccess",
	"PrimaryNoNewArray MethodInvocation",
	"PrimaryNoNewArray ArrayAccess",
	"Literal NUMBER",
	"Literal true",
	"Literal false",
	"Literal CHAR",
	"Literal STRING",
	"Literal null",
	"ClassInstanceCreationExpression new ClassType ( ArgumentList* )",
	"ArgumentList* ArgumentList",
	"ArgumentList*",
	"ArgumentList Expression",
	"ArgumentList ArgumentList , Expression",
	"MethodInvocation Name ( ArgumentList* )",
	"MethodInvocation FieldAccess ( ArgumentList* )",
	"ArrayAccess Name [ Expression ]",
	"ArrayAccess PrimaryNoNewArray [ Expression ]",
	"ArrayCreationExpression new PrimitiveType DimExpr",
	"ArrayCreationExpression new ClassOrInterfaceType DimExpr",
	"DimExpr [ Expression ]",
	"AssignmentExpression ConditionalExpression",
	"AssignmentExpression Assignment",
	"ConditionalExpression ConditionalOrExpression",
	"ConditionalOrExpression ConditionalAndExpression",
	"ConditionalOrExpression ConditionalOrExpression || ConditionalAndExpression",
	"ConditionalAndExpression InclusiveOrExpression",
	"ConditionalAndExpression ConditionalAndExpression && InclusiveOrExpression",
	"InclusiveOrExpression AndExpression",
	"InclusiveOrExpression InclusiveOrExpression | AndExpression",
	"AndExpression EqualityExpression",
	"AndExpression AndExpression & EqualityExpression",
	"EqualityExpression RelationalExpression",
	"EqualityExpression EqualityExpression == RelationalExpression",
	"EqualityExpression EqualityExpression != RelationalExpression",
	"RelationalExpression RelationalExpression < AdditiveExpression",
	"RelationalExpression RelationalExpression > AdditiveExpression",
	"RelationalExpression RelationalExpression <= AdditiveExpression",
	"RelationalExpression RelationalExpression >= AdditiveExpression",
	"RelationalExpression RelationalExpression instanceof ReferenceType",
	"AdditiveExpression MultiplicativeExpression",
	"AdditiveExpression AdditiveExpression + MultiplicativeExpression",
	"AdditiveExpression AdditiveExpression - MultiplicativeExpression",
	"MultiplicativeExpression UnaryExpression",
	"MultiplicativeExpression MultiplicativeExpression * UnaryExpression",
	"MultiplicativeExpression MultiplicativeExpression / UnaryExpression",
	"MultiplicativeExpression MultiplicativeExpression % UnaryExpression",
	"UnaryExpression - UnaryExpression",
	"UnaryExpression UnaryExpressionNotMinus",
	"UnaryExpressionNotMinus ! UnaryExpression",
	"UnaryExpressionNotMinus CastExpression",
	"UnaryExpressionNotMinus Primary",
	"UnaryExpressionNotMinus Name",
	"CastExpression ( PrimitiveType Dims ) UnaryExpression",
	"CastExpression ( Name [ ] ) UnaryExpressionNotMinus",
	"Dims [ ]",
	"Dims",
	"MethodDeclaration MethodHeader MethodBody",
	"MethodHeader MemberModifiers Type MethodDeclarator",
	"MethodHeader MemberModifiers void MethodDeclarator",
	"MethodDeclarator Identifier ( FormalParameterList* )",
	"FormalParameterList* FormalParameterList",
	"FormalParameterList*",
	"FormalParameterList FormalParameter",
	"FormalParameterList FormalParameterList , FormalParameter",
	"FormalParameter Type Identifier",
	"MethodBody Block",
	"MethodBody ;",
	"Block { BlockStatements* }",
	"BlockStatements* BlockStatements",
	"BlockStatements*",
	"BlockStatements BlockStatement",
	"BlockStatements BlockStatements BlockStatement",
	"BlockStatement LocalVariableDeclarationStatement",
	"BlockStatement Statement",
	"LocalVariableDeclarationStatement LocalVariableDeclaration ;",
	"LocalVariableDeclaration Type Identifier = Expression",
	"Statement StatementWithoutTrailingSubstatement",
	"Statement IfThenStatement",
	"Statement IfThenElseStatement",
	"Statement WhileStatement",
	"Statement ForStatement",
	"StatementWithoutTrailingSubstatement Block",
	"StatementWithoutTrailingSubstatement EmptyStatement",
	"StatementWithoutTrailingSubstatement ExpressionStatement",
	"StatementWithoutTrailingSubstatement ReturnStatement",
	"EmptyStatement ;",
	"ExpressionStatement StatementExpression ;",
	"StatementExpression Assignment",
	"StatementExpression MethodInvocation",
	"StatementExpression ClassInstanceCreationExpression",
	"IfThenStatement if ( Expression ) Statement",
	"IfThenElseStatement if ( Expression ) StatementNoShortIf else Statement",
	"StatementNoShortIf StatementWithoutTrailingSubstatement",
	"StatementNoShortIf IfThenElseStatementNoShortIf",
	"StatementNoShortIf WhileStatementNoShortIf",
	"StatementNoShortIf ForStatementNoShortIf",
	"IfThenElseStatementNoShortIf if ( Expression ) StatementNoShortIf else StatementNoShortIf",
	"WhileStatementNoShortIf while ( Expression ) StatementNoShortIf",
	"ForStatementNoShortIf for ( ForInit ; Expression* ; ForUpdate ) StatementNoShortIf",
	"Expression* Expression",
	"Expression*",
	"WhileStatement while ( Expression ) Statement",
	"ForStatement for ( ForInit ; Expression* ; ForUpdate ) Statement",
	"ForInit StatementExpression",
	"ForInit LocalVariableDeclaration",
	"ForInit",
	"ForUpdate StatementExpression",
	"ForUpdate",
	"ReturnStatement return Expression* ;",
	"ConstructorDeclaration MemberModifiers ConstructorDeclarator ConstructorBody",
	"ConstructorDeclarator SimpleName ( FormalParameterList* )",
	"ConstructorBody { BlockStatements* }",
	"InterfaceDeclaration Modifiers interface Identifier ExtendsInterfaces* InterfaceBody",
	"ExtendsInterfaces InterfaceType",
	"ExtendsInterfaces ExtendsInterfaces , InterfaceType",
	"InterfaceBody { InterfaceMemberDeclarations* }",
	"InterfaceMemberDeclarations* InterfaceMemberDeclarations",
	"InterfaceMemberDeclarations*",
	"InterfaceMemberDeclarations InterfaceMemberDeclaration",
	"InterfaceMemberDeclarations InterfaceMemberDeclarations InterfaceMemberDeclaration",
	"InterfaceMemberDeclaration AbstractMethodDeclaration",
	"AbstractMethodDeclaration AbstractMethodHeader ;",
	"RelationalExpression AdditiveExpression",
	"CastExpression ( Expression ) UnaryExpressionNotMinus",
	"ExtendsInterfaces* extends ExtendsInterfaces",
	"ExtendsInterfaces*",
	"TypeDeclaration InterfaceDeclaration",
	"TypeDeclaration",
	"MemberModifiers* MemberModifiers",
	"MemberModifiers*",
	"AbstractMethodHeader MemberModifiers* Type MethodDeclarator",
	"AbstractMethodHeader MemberModifiers* void MethodDeclarator",
	"PrimaryNoNewArray Name . this"
};