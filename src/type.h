#pragma once

#include "stdafx.h"
#include "util.h"
#include "operator.h"
#include "code_loc.h"
#include "context.h"
#include "function_name.h"

class Context;
struct TypeMapping;
struct FunctionType;
struct SwitchStatement;
struct FunctionDefinition;
struct Accu;

struct Type : public owned_object<Type> {
  Type();
  virtual string getName(bool withTemplateArguments = true) const = 0;
  virtual string getCodegenName() const;
  virtual optional<string> getMangledName() const;
  virtual SType getUnderlying() const;
  virtual bool canAssign(SType from) const;
  virtual optional<string> getMappingError(const Context&, TypeMapping&, SType argType) const;
  SType replace(SType from, SType to, ErrorBuffer&) const;
  virtual bool canReplaceBy(SType) const;
  virtual SType replaceImpl(SType from, SType to, ErrorBuffer&) const;
  virtual SType getType() const;
  struct MemberInfo {
    SType type;
    string name;
  };
  virtual WithError<MemberInfo> getTypeOfMember(const SCompileTimeValue&) const;
  virtual WithError<SType> getTypeOfMember(const string& name) const;
  void codegenDefinition(set<const Type*>& visited, Accu&) const;
  virtual ~Type() {}
  virtual WithErrorLine<SType> instantiate(const Context&, vector<SType> templateArgs, CodeLoc) const;
  Context& getStaticContext();
  enum class SwitchArgument {
    VALUE,
    REFERENCE,
    MUTABLE_REFERENCE
  };
  virtual optional<ErrorLoc> handleSwitchStatement(SwitchStatement&, Context&, SwitchArgument) const;
  virtual bool isBuiltinCopyable(const Context&) const;
  virtual SType removePointer() const;
  virtual optional<string> getSizeError(const Context&) const;
  Context staticContext;

  protected:
  virtual void codegenDefinitionImpl(set<const Type*>& visited, Accu&) const;
};

struct ArithmeticType : public Type {
  virtual string getName(bool withTemplateArguments = true) const override;
  virtual string getCodegenName() const override;
  virtual bool isBuiltinCopyable(const Context&) const override;
  using DefType = shared_ptr<ArithmeticType>;
  static DefType INT;
  static DefType DOUBLE;
  static DefType BOOL;
  static DefType VOID;
  static DefType CHAR;
  static DefType STRING;
  static DefType NORETURN;
  static DefType ANY_TYPE;
  static DefType ENUM_TYPE;
  static DefType STRUCT_TYPE;
  ArithmeticType(const string& name, optional<string> codegenName = none);

  private:
  string name;
  string codegenName;
};

struct ReferenceType : public Type {
  virtual string getName(bool withTemplateArguments = true) const override;
  virtual optional<string> getMangledName() const override;
  virtual string getCodegenName() const override;
  virtual SType getUnderlying() const override;
  virtual bool canAssign(SType from) const override;
  virtual optional<string> getMappingError(const Context&, TypeMapping& mapping, SType from) const override;
  virtual SType replaceImpl(SType from, SType to, ErrorBuffer&) const override;
  virtual optional<ErrorLoc> handleSwitchStatement(SwitchStatement&, Context&, SwitchArgument) const override;
  virtual WithError<MemberInfo> getTypeOfMember(const SCompileTimeValue&) const override;
  virtual WithError<SType> getTypeOfMember(const string& name) const override;
  virtual SType removePointer() const override;

  static shared_ptr<ReferenceType> get(SType);
  SType underlying;
  ReferenceType(SType);
};

struct MutableReferenceType : public Type {
  virtual string getName(bool withTemplateArguments = true) const override;
  virtual optional<string> getMangledName() const override;
  virtual string getCodegenName() const override;
  virtual SType getUnderlying() const override;
  virtual bool canAssign(SType from) const override;
  virtual optional<string> getMappingError(const Context&, TypeMapping& mapping, SType from) const override;
  virtual SType replaceImpl(SType from, SType to, ErrorBuffer&) const override;
  virtual optional<ErrorLoc> handleSwitchStatement(SwitchStatement&, Context&, SwitchArgument) const override;
  virtual WithError<MemberInfo> getTypeOfMember(const SCompileTimeValue&) const override;
  virtual WithError<SType> getTypeOfMember(const string& name) const override;
  virtual SType removePointer() const override;

  static shared_ptr<MutableReferenceType> get(SType);
  MutableReferenceType(SType);
  SType underlying;
};

struct PointerType : public Type {
  virtual string getName(bool withTemplateArguments = true) const override;
  virtual string getCodegenName() const override;
  virtual optional<string> getMangledName() const override;
  virtual SType replaceImpl(SType from, SType to, ErrorBuffer&) const override;
  virtual optional<string> getMappingError(const Context&, TypeMapping&, SType argType) const override;
  virtual bool isBuiltinCopyable(const Context&) const override;
  virtual SType removePointer() const override;

  static shared_ptr<PointerType> get(SType);
  SType underlying;
  PointerType(SType);
};

struct MutablePointerType : public Type {
  virtual string getName(bool withTemplateArguments = true) const override;
  virtual string getCodegenName() const override;
  virtual optional<string> getMangledName() const override;
  virtual SType replaceImpl(SType from, SType to, ErrorBuffer&) const override;
  virtual optional<string> getMappingError(const Context&, TypeMapping&, SType argType) const override;
  virtual bool isBuiltinCopyable(const Context&) const override;
  virtual SType removePointer() const override;

  static shared_ptr<MutablePointerType> get(SType);
  MutablePointerType(SType);
  SType underlying;
};

struct EnumType;

#define COMPARABLE(Type, ...)\
auto asTuple() const {\
  return std::forward_as_tuple(__VA_ARGS__);\
}\
bool operator == (const Type& o) const {\
  return asTuple() == o.asTuple();\
}\
bool operator < (const Type& o) const {\
  return asTuple() < o.asTuple();\
}

struct CompileTimeValue : public Type {
  virtual string getName(bool withTemplateArguments = true) const override;
  virtual string getCodegenName() const override;
  virtual optional<string> getMappingError(const Context&, TypeMapping&, SType argType) const override;
  virtual bool canReplaceBy(SType) const override;
  virtual SType replaceImpl(SType from, SType to, ErrorBuffer&) const override;
  virtual SType getType() const override;
  virtual optional<string> getMangledName() const override;
  struct TemplateValue {
    SType type;
    string name;
    COMPARABLE(TemplateValue, type, name)
  };
  struct TemplateExpression {
    Operator op;
    vector<SCompileTimeValue> args;
    SType type;
    COMPARABLE(TemplateExpression, op, args, type)
  };
  struct TemplateFunctionCall {
    string name;
    vector<SType> args;
    SType retVal;
    CodeLoc loc;
    vector<CodeLoc> argLoc;
    Context::BuiltInFunctionInfo functionInfo;
    COMPARABLE(TemplateFunctionCall, name, args)
  };
  struct EnumValue {
    shared_ptr<EnumType> type;
    int index;
    COMPARABLE(EnumValue, type, index)
  };
  struct ArrayValue {
    vector<SCompileTimeValue> values;
    SType type;
    COMPARABLE(ArrayValue, values, type)
  };

  using Value = variant<int, bool, double, char, string, EnumValue, ArrayValue, TemplateValue, TemplateExpression, TemplateFunctionCall>;
  static shared_ptr<CompileTimeValue> get(Value);
  static shared_ptr<CompileTimeValue> getTemplateValue(SType type, string name);
  Value value;

  CompileTimeValue(Value);
};

struct TemplateParameterType : public Type {
  virtual string getName(bool withTemplateArguments = true) const override;
  virtual bool canReplaceBy(SType) const override;
  virtual optional<string> getMangledName() const override;
  virtual SType getType() const override;
  virtual bool isBuiltinCopyable(const Context&) const override;
  virtual WithError<MemberInfo> getTypeOfMember(const SCompileTimeValue&) const override;
  TemplateParameterType(string name, CodeLoc);
  TemplateParameterType(SType type, string name, CodeLoc);
  string name;
  CodeLoc declarationLoc;
  SType type;
};

struct TemplateStructMemberType : public Type {
  virtual string getName(bool withTemplateArguments = true) const override;
  virtual optional<string> getMangledName() const override;
  static shared_ptr<TemplateStructMemberType> get(SType structType, SCompileTimeValue);
  SType structType;
  SCompileTimeValue memberIndex;
  struct Private {};
  TemplateStructMemberType(Private, SType structType, SCompileTimeValue);
};

struct StructType : public Type {
  virtual string getName(bool withTemplateArguments = true) const override;
  virtual string getCodegenName() const override;
  virtual optional<string> getMangledName() const override;
  virtual SType replaceImpl(SType from, SType to, ErrorBuffer&) const override;
  virtual WithErrorLine<SType> instantiate(const Context&, vector<SType> templateArgs, CodeLoc) const override;
  virtual optional<string> getMappingError(const Context&, TypeMapping&, SType argType) const override;
  virtual optional<ErrorLoc> handleSwitchStatement(SwitchStatement&, Context&, SwitchArgument) const override;
  virtual optional<string> getSizeError(const Context&) const override;
  virtual void codegenDefinitionImpl(set<const Type*>& visited, Accu&) const override;
  virtual WithError<MemberInfo> getTypeOfMember(const SCompileTimeValue&) const override;
  virtual SType getType() const override;
  virtual WithError<SType> getTypeOfMember(const string&) const override;
  string name;
  shared_ptr<StructType> getInstance(vector<SType> templateParams);
  vector<SType> templateParams;
  vector<shared_ptr<StructType>> instances;
  nullable<shared_ptr<StructType>> parent;
  vector<SConcept> requirements;
  struct Variable {
    string name;
    SType type;
  };
  vector<Variable> members;
  vector<Variable> alternatives;
  bool external = false;
  optional<CodeLoc> definition;
  void updateInstantations();
  SType getInstantiated(vector<SType> templateParams);
  struct Private {};
  StructType(string name, Private);
};

struct EnumType : public Type {
  virtual string getName(bool withTemplateArguments = true) const override;
  virtual optional<ErrorLoc> handleSwitchStatement(SwitchStatement&, Context&, SwitchArgument) const override;
  virtual bool isBuiltinCopyable(const Context&) const override;
  virtual SType getType() const override;
  virtual void codegenDefinitionImpl(set<const Type*>& visited, Accu&) const override;

  string name;
  vector<string> elements;
  bool external = false;
  optional<CodeLoc> definition;
  struct Private {};
  EnumType(string name, Private);
};

struct ArrayType : public Type {
  virtual string getName(bool withTemplateArguments = true) const override;
  virtual string getCodegenName() const override;
  virtual optional<string> getMangledName() const override;
  virtual SType replaceImpl(SType from, SType to, ErrorBuffer&) const override;
  virtual optional<string> getMappingError(const Context&, TypeMapping&, SType argType) const override;
  static shared_ptr<ArrayType> get(SType, SCompileTimeValue size);
  virtual bool isBuiltinCopyable(const Context&) const override;
  virtual optional<string> getSizeError(const Context&) const override;
  virtual void codegenDefinitionImpl(set<const Type*>& visited, Accu&) const override;
  ArrayType(SType, SCompileTimeValue size);
  SCompileTimeValue size;
  SType underlying;
};

struct SliceType : public Type {
  virtual string getName(bool withTemplateArguments = true) const override;
  virtual string getCodegenName() const override;
  virtual optional<string> getMangledName() const override;
  virtual SType replaceImpl(SType from, SType to, ErrorBuffer&) const override;
  virtual optional<string> getMappingError(const Context&, TypeMapping&, SType argType) const override;
  static shared_ptr<SliceType> get(SType);
  virtual bool isBuiltinCopyable(const Context&) const override;
  struct Private {};
  SliceType(Private, SType);
  SType underlying;
};

struct FunctionType {
  struct Param {
    Param(optional<string> name, SType type);
    Param(string name, SType type);
    Param(SType type);
    optional<string> name;
    SType type;
    COMPARABLE(FunctionType::Param, name, type)
  };
  FunctionType(SType returnType, vector<Param> params, vector<SType> templateParams);
  FunctionType setBuiltin();
  SType retVal;
  vector<Param> params;
  vector<SType> templateParams;
  vector<SConcept> requirements;
  nullable<SType> parentType;
  bool fromConcept = false;
  bool builtinOperator = false;
  bool generatedConstructor = false;
  COMPARABLE(FunctionType, retVal, params, templateParams, parentType)
};

struct FunctionInfo : public owned_object<FunctionInfo> {
  struct Private {};
  static SFunctionInfo getImplicit(FunctionId, FunctionType);
  static SFunctionInfo getInstance(FunctionId, FunctionType, SFunctionInfo parent);
  static SFunctionInfo getDefined(FunctionId, FunctionType, FunctionDefinition*);
  const FunctionId id;
  const FunctionType type;
  vector<SFunctionInfo> instantiations;
  const nullable<SFunctionInfo> parent;
  FunctionDefinition* const definition = nullptr;
  string prettyString() const;
  optional<string> getMangledName() const;
  FunctionInfo(Private, FunctionId, FunctionType, nullable<SFunctionInfo> parent);
  FunctionInfo(Private, FunctionId, FunctionType, FunctionDefinition*);
  SFunctionInfo getParent() const;
};

struct Concept : public owned_object<Concept> {
  Concept(const string& name, Context emptyContext);
  string getName() const;
  SConcept translate(vector<SType> params, ErrorBuffer&) const;
  SConcept replace(SType from, SType to, ErrorBuffer&) const;
  const vector<SType>& getParams() const;
  const Context& getContext() const;
  vector<SType>& modParams();
  Context& modContext();

  private:
  vector<SType> params;
  string name;
  Context context;
};

struct Expression;
class Context;

struct IdentifierInfo;
extern WithErrorLine<SFunctionInfo> instantiateFunction(const Context& context, const SFunctionInfo&, CodeLoc,
    vector<SType> templateArgs, vector<SType> argTypes, vector<CodeLoc> argLoc, vector<FunctionType> existing = {});
extern FunctionType replaceInFunction(FunctionType, SType from, SType to, ErrorBuffer&);
extern SFunctionInfo replaceInFunction(const SFunctionInfo&, SType from, SType to, ErrorBuffer&);
extern string joinTemplateParams(const vector<SType>&);
extern string joinTypeList(const vector<SType>&);
extern string joinTemplateParamsCodegen(const vector<SType>&);
extern string joinTypeListCodegen(const vector<SType>&);
