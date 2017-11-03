#include "state.h"
#include "ast.h"
#include "type.h"

optional<Type> State::getTypeOfVariable(const IdentifierInfo& id) const {
  id.codeLoc.check(id.parts.size() == 1 && id.parts.at(0).templateArguments.empty(),
      "Bad variable identifier: " + id.toString());
  auto name = id.parts.at(0).name;
  if (vars.count(name))
    return vars.at(name);
  else
    return none;
}

void State::addVariable(const string& id, Type t) {
  vars[id] = t;
}

const optional<Type>& State::getReturnType() const {
  return returnType;
}

void State::setReturnType(Type t) {
  CHECK(!returnType) << "Attempted to overwrite return type";
  returnType = t;
}

void State::addType(const string& name, Type t) {
  types[name] = t;
}

vector<Type> State::getTypeList(const vector<IdentifierInfo>& ids) const {
  vector<Type> params;
  for (auto& id : ids)
    if (auto type = getTypeFromString(id))
      params.push_back(*type);
    else
      id.codeLoc.error("Unrecognized type: " + quote(id.toString()));
  return params;
}

optional<Type> State::getTypeFromString(IdentifierInfo id) const {
  //INFO << "Get type " << id.toString();
  id.codeLoc.check(id.parts.size() == 1, "Bad type identifier: " + id.toString());
  auto name = id.parts.at(0).name;
  if (!types.count(name))
    return none;
  return instantiate(types.at(name), getTypeList(id.parts.at(0).templateArguments));
}

bool State::typeNameExists(const string& name) const {
  return types.count(name);
}

void State::addFunction(string id, FunctionType f) {
  INFO << "Inserting function " << id;
  CHECK(!functions.count(id));
  functions.insert(make_pair(id, f));
}

FunctionType State::getFunctionTemplate(CodeLoc codeLoc, IdentifierInfo id) const {
  string funName = id.parts.at(0).name;
  if (id.parts.size() == 2) {
    if (auto type = getTypeFromString(IdentifierInfo(id.parts.at(0)))) {
      INFO << "Looking for static method in type " << getName(*type);
      if (auto fun = getStaticMethod(*type, id.parts.at(1).name))
        return *fun;
      else
        id.codeLoc.error("Static method not found: " + id.toString());
    } else
      id.codeLoc.error("Type not found: " + id.toString());
  } else {
    id.codeLoc.check(id.parts.size() == 1, "Bad function identifier: " + id.toString());
    if (functions.count(funName))
      return functions.at(funName);
  }
  codeLoc.error("Function not found: " + quote(funName));
  return functions.at(funName);
}

vector<string> State::getFunctionParamNames(CodeLoc codeLoc, IdentifierInfo id) const {
  auto fun = getFunctionTemplate(codeLoc, id);
  return transform(fun.params, [](const FunctionType::Param& p) { return p.name; });
}

FunctionType State::getFunction(CodeLoc codeLoc, IdentifierInfo id, vector<Type> argTypes,
    vector<CodeLoc> argLoc) const {
  auto templateArgNames = id.parts.back().templateArguments;
  auto templateArgs = getTypeList(templateArgNames);
  auto ret = getFunctionTemplate(codeLoc, id);
  instantiate(ret, codeLoc, templateArgs, argTypes, argLoc);
  INFO << "Function " << id.toString() << " return type " << getName(*ret.retVal);
  return ret;
}