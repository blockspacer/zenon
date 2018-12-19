#pragma once

#include "stdafx.h"
#include "variant.h"
#include "code_loc.h"
#include "context.h"
#include "operator.h"
#include "identifier.h"
#include "type.h"
#include "import_cache.h"

struct Accu;

struct CodegenStage {
  static CodegenStage define();
  static CodegenStage declare();
  CodegenStage setImport();
  bool isDefine;
  bool isImport;

  private:
  CodegenStage() {}
};

struct Node {
  Node(CodeLoc);
  CodeLoc codeLoc;
  virtual void codegen(Accu&, CodegenStage) const {}
  virtual ~Node() {}
};

extern SType getType(Context&, unique_ptr<Expression>&, bool evaluateAtCompileTime = true);

struct Expression : Node {
  using Node::Node;
  virtual SType getTypeImpl(Context&) = 0;
  virtual nullable<SType> eval(const Context&) const;
  virtual nullable<SType> getDotOperatorType(Expression* left, Context& callContext);
  virtual void codegenDotOperator(Accu&, CodegenStage, Expression* leftSide) const;
};

struct Constant : Expression {
  Constant(CodeLoc, SCompileTimeValue);
  virtual SType getTypeImpl(Context&) override;
  virtual nullable<SType> eval(const Context&) const override;
  virtual void codegen(Accu&, CodegenStage) const override;
  SCompileTimeValue value;
};

struct EnumConstant : Expression {
  EnumConstant(CodeLoc, string enumName, string enumElement);
  virtual SType getTypeImpl(Context&) override;
  virtual void codegen(Accu&, CodegenStage) const override;
  virtual nullable<SType> eval(const Context&) const override;
  string enumName;
  string enumElement;
};

struct Variable : Expression {
  Variable(CodeLoc, IdentifierInfo);
  Variable(CodeLoc, string);
  virtual SType getTypeImpl(Context&) override;
  virtual nullable<SType> eval(const Context&) const override;
  virtual void codegen(Accu&, CodegenStage) const override;
  virtual nullable<SType> getDotOperatorType(Expression* left, Context& callContext) override;
  IdentifierInfo identifier;
};

struct BinaryExpression : Expression {
  static unique_ptr<Expression> get(CodeLoc, Operator, unique_ptr<Expression>, unique_ptr<Expression>);
  static unique_ptr<Expression> get(CodeLoc, Operator, vector<unique_ptr<Expression>>);
  virtual SType getTypeImpl(Context&) override;
  virtual nullable<SType> eval(const Context&) const override;
  virtual void codegen(Accu&, CodegenStage) const override;
  Operator op;
  vector<unique_ptr<Expression>> expr;
  bool subscriptOpWorkaround = true;
  struct Private {};
  BinaryExpression(Private, CodeLoc, Operator, vector<unique_ptr<Expression>>);
};

struct UnaryExpression : Expression {
  UnaryExpression(CodeLoc, Operator, unique_ptr<Expression>);
  virtual SType getTypeImpl(Context&) override;
  virtual nullable<SType> eval(const Context&) const override;
  virtual void codegen(Accu&, CodegenStage) const override;
  Operator op;
  unique_ptr<Expression> expr;
};

struct MoveExpression : Expression {
  MoveExpression(CodeLoc, string);
  virtual SType getTypeImpl(Context&) override;
  virtual void codegen(Accu&, CodegenStage) const override;
  string identifier;
  nullable<SType> type;
};

struct ArrayLiteral : Expression {
  ArrayLiteral(CodeLoc);
  virtual SType getTypeImpl(Context&) override;
  virtual void codegen(Accu&, CodegenStage) const override;
  vector<unique_ptr<Expression>> contents;
};

enum class MethodCallType { METHOD, FUNCTION_AS_METHOD, FUNCTION_AS_METHOD_WITH_POINTER };

struct FunctionCall : Expression {
  FunctionCall(CodeLoc, IdentifierInfo);
  FunctionCall(CodeLoc, IdentifierInfo, unique_ptr<Expression> arg);
  virtual SType getTypeImpl(Context&) override;
  virtual void codegen(Accu&, CodegenStage) const override;
  virtual nullable<SType> getDotOperatorType(Expression* left, Context& callContext) override;
  virtual void codegenDotOperator(Accu&, CodegenStage, Expression* leftSide) const override;
  virtual nullable<SType> eval(const Context&) const override;
  IdentifierInfo identifier;
  optional<FunctionInfo> functionInfo;
  vector<unique_ptr<Expression>> arguments;
  vector<optional<string>> argNames;
  optional<MethodCallType> callType;
};

struct Statement : Node {
  using Node::Node;
  virtual void addToContext(Context&);
  virtual void addToContext(Context&, ImportCache&);
  virtual void check(Context&) = 0;
  virtual bool hasReturnStatement(const Context&) const;
  virtual void codegen(Accu&, CodegenStage) const = 0;
  enum class TopLevelAllowance {
    CANT,
    CAN,
    MUST
  };
  virtual TopLevelAllowance allowTopLevel() const { return TopLevelAllowance::CANT; }
};

struct VariableDeclaration : Statement {
  VariableDeclaration(CodeLoc, optional<IdentifierInfo> type, string identifier, unique_ptr<Expression> initExpr);
  optional<IdentifierInfo> type;
  nullable<SType> realType;
  string identifier;
  unique_ptr<Expression> initExpr;
  bool isMutable = false;
  virtual void check(Context&) override;
  virtual void codegen(Accu&, CodegenStage) const override;
};

struct IfStatement : Statement {
  IfStatement(CodeLoc,
      unique_ptr<VariableDeclaration> decl /*can be null*/,
      unique_ptr<Expression> cond /*can be null if decl is non-null*/,
      unique_ptr<Statement> ifTrue,
      unique_ptr<Statement> ifFalse /* can be null*/);
  unique_ptr<VariableDeclaration> declaration;
  unique_ptr<Expression> condition;
  unique_ptr<Statement> ifTrue, ifFalse;
  virtual bool hasReturnStatement(const Context&) const override;
  virtual void check(Context&) override;
  virtual void codegen(Accu&, CodegenStage) const override;
};

struct StatementBlock : Statement {
  using Statement::Statement;
  vector<unique_ptr<Statement>> elems;
  virtual bool hasReturnStatement(const Context&) const override;
  virtual void check(Context&) override;
  virtual void codegen(Accu&, CodegenStage) const override;
};

struct ReturnStatement : Statement {
  using Statement::Statement;
  ReturnStatement(CodeLoc, unique_ptr<Expression>);
  unique_ptr<Expression> expr;
  virtual bool hasReturnStatement(const Context&) const override;
  virtual void check(Context&) override;
  virtual void codegen(Accu&, CodegenStage) const override;
};

struct BreakStatement : Statement {
  using Statement::Statement;
  virtual void check(Context&) override;
  virtual void codegen(Accu&, CodegenStage) const override;
};

struct ContinueStatement : Statement {
  using Statement::Statement;
  virtual void check(Context&) override;
  virtual void codegen(Accu&, CodegenStage) const override;
};

struct ExpressionStatement : Statement {
  ExpressionStatement(unique_ptr<Expression>);
  unique_ptr<Expression> expr;
  virtual void check(Context&) override;
  virtual void codegen(Accu&, CodegenStage) const override;
};

struct ForLoopStatement : Statement {
  ForLoopStatement(CodeLoc l, unique_ptr<Statement> init, unique_ptr<Expression> cond, unique_ptr<Expression> iter,
      unique_ptr<Statement> body);
  unique_ptr<Statement> init;
  unique_ptr<Expression> cond;
  unique_ptr<Expression> iter;
  unique_ptr<Statement> body;
  virtual void check(Context&) override;
  virtual void codegen(Accu&, CodegenStage) const override;
};

struct RangedLoopStatement : Statement {
  RangedLoopStatement(CodeLoc l, unique_ptr<VariableDeclaration> init, unique_ptr<Expression> container,
      unique_ptr<Statement> body);
  unique_ptr<VariableDeclaration> init;
  unique_ptr<Expression> container;
  unique_ptr<Expression> condition;
  unique_ptr<Expression> increment;
  unique_ptr<Statement> body;
  optional<string> containerName;
  unique_ptr<VariableDeclaration> containerEnd;
  virtual void check(Context&) override;
  virtual void codegen(Accu&, CodegenStage) const override;
};

struct WhileLoopStatement : Statement {
  WhileLoopStatement(CodeLoc l, unique_ptr<Expression> cond, unique_ptr<Statement> body);
  unique_ptr<Expression> cond;
  unique_ptr<Statement> body;
  virtual void check(Context&) override;
  virtual void codegen(Accu&, CodegenStage) const override;
};

struct FunctionDefinition;

struct TemplateParameter {
  string name;
  optional<string> type;
  CodeLoc codeLoc;
};

struct TemplateInfo {
  vector<TemplateParameter> params;
  vector<IdentifierInfo> requirements;
};

struct StructDefinition : Statement {
  StructDefinition(CodeLoc, string name);
  bool incomplete = false;
  string name;
  struct Member {
    IdentifierInfo type;
    string name;
    CodeLoc codeLoc;
  };
  vector<Member> members;
  TemplateInfo templateInfo;
  nullable<shared_ptr<StructType>> type;
  virtual void addToContext(Context&, ImportCache&) override;
  virtual void check(Context&) override;
  virtual void codegen(Accu&, CodegenStage) const override;
  virtual TopLevelAllowance allowTopLevel() const override { return TopLevelAllowance::MUST; }
  bool external = false;
};

struct VariantDefinition : Statement {
  VariantDefinition(CodeLoc, string name);
  string name;
  struct Element {
    IdentifierInfo type;
    string name;
    CodeLoc codeLoc;
  };
  vector<Element> elements;
  TemplateInfo templateInfo;
  nullable<shared_ptr<StructType>> type;
  virtual void addToContext(Context&) override;
  virtual void check(Context&) override;
  virtual void codegen(Accu&, CodegenStage) const override;
  virtual TopLevelAllowance allowTopLevel() const override { return TopLevelAllowance::MUST; }
};

struct ConceptDefinition : Statement {
  ConceptDefinition(CodeLoc, string name);
  string name;
  vector<unique_ptr<FunctionDefinition>> functions;
  TemplateInfo templateInfo;
  virtual void addToContext(Context&) override;
  virtual void check(Context&) override;
  virtual void codegen(Accu&, CodegenStage) const override;
  virtual TopLevelAllowance allowTopLevel() const override { return TopLevelAllowance::MUST; }
};

struct EnumDefinition : Statement {
  EnumDefinition(CodeLoc, string name);
  string name;
  vector<string> elements;
  vector<unique_ptr<FunctionDefinition>> methods;
  virtual void addToContext(Context&) override;
  virtual void check(Context&) override;
  virtual void codegen(Accu&, CodegenStage) const override;
  virtual TopLevelAllowance allowTopLevel() const override { return TopLevelAllowance::MUST; }
};

struct SwitchStatement : Statement {
  SwitchStatement(CodeLoc, unique_ptr<Expression>);
  struct CaseElem {
    CodeLoc codeloc;
    nullable<SType> getType(const Context&);
    variant<none_t, IdentifierInfo, SType> type = none;
    string id;
    unique_ptr<StatementBlock> block;
    enum VarType { VALUE, POINTER, NONE } varType = NONE;
  };
  string subtypesPrefix;
  vector<CaseElem> caseElems;
  unique_ptr<StatementBlock> defaultBlock;
  unique_ptr<Expression> expr;
  enum { ENUM, VARIANT} type;
  virtual void check(Context&) override;
  virtual void codegen(Accu&, CodegenStage) const override;
  virtual bool hasReturnStatement(const Context&) const override;

  private:
  void codegenEnum(Accu&) const;
  void codegenVariant(Accu&) const;
};

struct FunctionDefinition : Statement {
  FunctionDefinition(CodeLoc, IdentifierInfo returnType, FunctionName);
  IdentifierInfo returnType;
  FunctionName name;
  struct Parameter {
    CodeLoc codeLoc;
    IdentifierInfo type;
    optional<string> name;
    bool isMutable;
    bool isVirtual;
  };
  vector<Parameter> parameters;
  unique_ptr<StatementBlock> body;
  TemplateInfo templateInfo;
  optional<FunctionInfo> functionInfo;
  bool external = false;
  struct Initializer {
    CodeLoc codeLoc;
    string paramName;
    unique_ptr<Expression> expr;
  };
  bool isVirtual = false;
  bool isDefault = false;
  vector<Initializer> initializers;
  virtual void check(Context&) override;
  virtual void addToContext(Context&, ImportCache&) override;
  virtual void codegen(Accu&, CodegenStage) const override;
  virtual TopLevelAllowance allowTopLevel() const override { return TopLevelAllowance::MUST; }
  void setFunctionType(const Context&, bool concept = false, bool builtInImport = false);
  void addSignature(Accu&, string structName) const;
  void handlePointerParamsInOperator(Accu&) const;
  void handlePointerReturnInOperator(Accu&) const;
  void generateVirtualDispatchBody(Context& bodyContext);
  WithErrorLine<unique_ptr<Expression>> getVirtualFunctionCallExpr(const Context&, const string& funName,
      const string& alternativeName, const SType& alternativeType, int virtualIndex);
  WithErrorLine<unique_ptr<Expression>> getVirtualOperatorCallExpr(Context&, Operator,
      const string& alternativeName, const SType& alternativeType, int virtualIndex);
  void checkAndGenerateCopyFunction(const Context&);
};

struct EmbedStatement : Statement {
  EmbedStatement(CodeLoc, string value);
  string value;
  bool isPublic = false;
  bool isTopLevel = false;
  virtual void check(Context&) override;
  virtual void codegen(Accu&, CodegenStage) const override;
  virtual TopLevelAllowance allowTopLevel() const override;
  virtual bool hasReturnStatement(const Context&) const override;
};

struct AST;

struct ImportStatement : Statement {
  ImportStatement(CodeLoc, string path, bool isPublic, bool isBuiltIn);
  string path;
  vector<string> importDirs;
  unique_ptr<AST> ast;
  const bool isPublic;
  const bool isBuiltIn;
  void setImportDirs(const vector<string>& importDirs);
  virtual void addToContext(Context&, ImportCache& cache) override;
  virtual void check(Context&) override;
  virtual void codegen(Accu&, CodegenStage) const override;
  virtual TopLevelAllowance allowTopLevel() const override { return TopLevelAllowance::MUST; }

  private:
  void processImport(Context&, ImportCache&, const string& content, const string& path);
};

struct AST {
  vector<unique_ptr<Statement>> elems;
};

struct ModuleInfo {
  string path;
  bool builtIn;
};

extern vector<ModuleInfo> correctness(AST&, const vector<string>& importPaths, bool builtIn);
