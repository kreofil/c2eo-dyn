#ifndef __TRACER__
#define __TRACER__

// Вспомогательные функции, осуществляющие трассировку программы
#include <iostream>
#include "clang/AST/Stmt.h"
#include "clang/AST/Expr.h"
#include "llvm/ADT/APInt.h"

#include "eo_object.h"

using namespace clang;
using namespace llvm;
using namespace std;

//#define TRACEOUT
#define TRACEOUT_EO

// Вывод операторного узла синтаксического дерева
void TraceOutASTnode(clang::Stmt::StmtClass stmtClass);

// Тестовый вывод информации о бинарном операторе
void TraceOutBinaryOperator(BinaryOperatorKind kind);

// Тестовый вывод информации о целочисленных литералах
void TraceOutIntegerLiteral(APInt &v, bool is_signed);

// Тестовый вывод объекта EO
void TraceOutEOObject(EOObject &eoObject);

#endif // __TRACER__

