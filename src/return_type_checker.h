#pragma once

#include "operator.h"

class Context;

class ReturnTypeChecker {
  public:
  ReturnTypeChecker(nullable<SType> explicitReturn);
  JustError<string> addReturnStatement(const Context&, SType);
  SType getReturnType() const;

  private:
  nullable<SType> explicitReturn;
  nullable<SType> returnStatement;
};
