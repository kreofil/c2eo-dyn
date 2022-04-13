#include "transpile_helper.h"
#include "memory_manager.h"
#include "unit_transpiler.h"
#include "vardecl.h"
#include <queue>
#include <sstream>
#include "tracer.h"  // Подключение функций трассировки программы

using namespace clang;
using namespace llvm;
using namespace std;

vector<Variable> ProcessFunctionLocalVariables(const clang::CompoundStmt *CS, size_t shift);
vector<Variable> ProcessFunctionParams(ArrayRef<ParmVarDecl *> params, size_t shift);

EOObject* GetBinaryStmtEOObject(const BinaryOperator *p_operator);
EOObject* GetAssignmentOperationOperatorEOObject(const CompoundAssignOperator *p_operator);
EOObject* GetUnaryStmtEOObject(const UnaryOperator *p_operator);
EOObject* GetIfStmtEOObject(const IfStmt *p_stmt);
EOObject* GetWhileStmtEOObject(const WhileStmt *p_stmt);
EOObject* GetDoWhileStmtEOObject(const DoStmt *p_stmt);
EOObject* GetIntegerLiteralEOObject(const IntegerLiteral *p_literal);
EOObject* GetReturnStmpEOObject(const ReturnStmt *p_stmt);
EOObject* GetAssignmentOperatorEOObject(const BinaryOperator *p_operator);
EOObject* GetCompoundAssignEOObject(const CompoundAssignOperator *p_operator);
EOObject* GetFloatingLiteralEOObject(const FloatingLiteral *p_literal);
EOObject* GetFunctionCallEOObject(const CallExpr *op);


size_t GetParamMemorySize(ArrayRef<ParmVarDecl *> params);

extern UnitTranspiler transpiler;

EOObject* GetFunctionBody(const clang::FunctionDecl *FD) {
  if (!FD->hasBody())
    // TODO if not body may be need to create simple complete or abstract object with correct name
    return new EOObject(EOObjectType::EO_EMPTY);
  const auto funcBody = dyn_cast<CompoundStmt>(FD->getBody());
  size_t shift = transpiler.glob.RealMemorySize();
  size_t param_memory_size = GetParamMemorySize(FD->parameters());
  vector<Variable> all_param = ProcessFunctionParams(FD->parameters(), shift);
  vector<Variable> all_local = ProcessFunctionLocalVariables(funcBody, shift + param_memory_size);
  EOObject *func_body_eo = new EOObject(EOObjectType::EO_EMPTY);
  EOObject *local_start = new EOObject("add", "local-start");
  local_start->nested.emplace_back(new EOObject{"param-start"});
  local_start->nested.emplace_back(new EOObject{"param-size"});
  func_body_eo->nested.push_back(local_start);
  size_t free_pointer = transpiler.glob.RealMemorySize();
  EOObject *local_empty_position = new EOObject("add", "empty-local-position");
  local_empty_position->nested.emplace_back(new EOObject{"local-start"});
  local_empty_position->nested.emplace_back(new EOObject{to_string(free_pointer - shift - param_memory_size),
                                           EOObjectType::EO_LITERAL});
  func_body_eo->nested.push_back(local_empty_position);
  for (const auto &param: all_param) {
    func_body_eo->nested.push_back(param.GetAddress(transpiler.glob.name));
  }
  for (auto &var: all_local) {
    func_body_eo->nested.push_back(var.GetAddress(transpiler.glob.name));
  }
  EOObject* body_seq = GetCompoundStmt(funcBody, true);
  std::reverse(all_local.begin(), all_local.end());
  for (const auto &var: all_local) {
    if (var.is_initialized)
      body_seq->nested.insert(body_seq->nested.begin(), var.GetInitializer());
  }
  func_body_eo->nested.push_back(body_seq);
  transpiler.glob.RemoveAllUsed(all_param);
  transpiler.glob.RemoveAllUsed(all_local);

  return func_body_eo;
}

size_t GetParamMemorySize(ArrayRef<ParmVarDecl *> params) {
  size_t res = 0;
  for (auto VD: params) {
    TypeInfo type_info = VD->getASTContext().getTypeInfo(VD->getType());
    size_t type_size = type_info.Width / 8;
    res += type_size;
  }
  return res;
}

vector<Variable> ProcessFunctionParams(ArrayRef<ParmVarDecl *> params, size_t shift) {
  vector<Variable> all_params;
  for (auto param: params) {
    all_params.push_back(ProcessVariable(param, "param-start", shift));
  }
  return all_params;
}

vector<Variable> ProcessFunctionLocalVariables(const clang::CompoundStmt *CS, size_t shift) {
  vector<Variable> all_local;
  for (auto stmt: CS->body()) {
    Stmt::StmtClass stmtClass = stmt->getStmtClass();
    if (stmtClass == Stmt::DeclStmtClass) {
      auto decl_stmt = dyn_cast<DeclStmt>(stmt);
      for (auto decl: decl_stmt->decls()) {
        // TODO if var created in nested statement we don't found it. Fix
        // TODO duplicate names problem should be resoved
        Decl::Kind decl_kind = decl->getKind();
        if (decl_kind == Decl::Kind::Var) {
          auto var_decl = dyn_cast<VarDecl>(decl);
          all_local.push_back(ProcessVariable(var_decl, "local-start", shift));
        }
      }
    }
  }
  return all_local;
}

// Function to get eo representation of CompoundStmt
EOObject* GetCompoundStmt(const clang::CompoundStmt *CS, bool is_decorator) {
  EOObject* res = new EOObject{"seq"};
  if (is_decorator)
    res->postfix = "@";
  for (auto stmt: CS->body()) {
    Stmt::StmtClass stmtClass = stmt->getStmtClass();
    // Костыльное решение для тестового вывода
    if (stmtClass == Stmt::ImplicitCastExprClass) // Нужно разобраться с именами перечислимых типов
    {
      auto ref = dyn_cast<DeclRefExpr>(*stmt->child_begin());
      if (!ref)
        continue;
      try {
        const Variable &var = transpiler.glob.GetVarByID(dyn_cast<VarDecl>(ref->getFoundDecl()));
        string formatter = "d";
        if (var.type_postfix == "float32" || var.type_postfix == "float64")
          formatter = "f";
        EOObject* printer = new EOObject{"printf"};
        printer->nested.emplace_back(new EOObject{"\"%" + formatter + "\\n\"", EOObjectType::EO_LITERAL});
        EOObject* read_val = new EOObject{"read-as-" + var.type_postfix};
        read_val->nested.emplace_back(new EOObject{var.alias});
        printer->nested.push_back(read_val);
        res->nested.push_back(printer);
      }
      catch (invalid_argument &) {
        res->nested.emplace_back(new EOObject{EOObjectType::EO_PLUG});
      }
      continue;
    }
    EOObject* stmt_obj = GetStmtEOObject(stmt);
    res->nested.push_back(stmt_obj);
    #ifdef TRACEOUT_EO
      TraceOutEOObject(*stmt_obj);
    #endif
  }
  res->nested.emplace_back(new EOObject{"TRUE", EOObjectType::EO_LITERAL});
  //!
  #ifdef TRACEOUT_EO
    TraceOutEOObject(*res);
  #endif
  return res;
}

EOObject* GetStmtEOObject(const clang::Stmt* p_stmt)
{
  Stmt::StmtClass stmtClass = p_stmt->getStmtClass();
  //! Trace out of Statement Class
  //p_stmt->dump();
  #ifdef TRACEOUT
    p_stmt->dump();
    TraceOutASTnode(stmtClass);
  #endif
  if (stmtClass == Stmt::BinaryOperatorClass) {
    const auto *op = dyn_cast<BinaryOperator>(p_stmt);
    return GetBinaryStmtEOObject(op);
  } else if (stmtClass == Stmt::UnaryOperatorClass) {
    const auto *op = dyn_cast<UnaryOperator>(p_stmt);
    return GetUnaryStmtEOObject(op);
  } else if (stmtClass == Stmt::CompoundAssignOperatorClass) {
    const auto *op = dyn_cast<CompoundAssignOperator>(p_stmt);
    return GetAssignmentOperationOperatorEOObject(op);
  } else if (stmtClass == Stmt::ParenExprClass) {
    const auto *op = dyn_cast<ParenExpr>(p_stmt);
    return GetStmtEOObject(*op->child_begin());
  } else if (stmtClass == Stmt::ImplicitCastExprClass) {
    const auto *op = dyn_cast<ImplicitCastExpr>(p_stmt);
    if (op->getCastKind() == clang::CK_LValueToRValue) {
      string type = op->getType()->isPointerType() ? "ptr" : GetTypeName(op->getType());
      EOObject* read = new EOObject{"read-as-" + type};
      read->nested.push_back(GetStmtEOObject(*op->child_begin()));
      return read;
    }
    // TODO if cast kinds and also split it to another func
    return GetStmtEOObject(*op->child_begin());
  } else if (stmtClass == Stmt::DeclRefExprClass) {
    auto ref = dyn_cast<DeclRefExpr>(p_stmt);
    if (!ref)
      return new EOObject{EOObjectType::EO_PLUG};
    try {
      const Variable &var = transpiler.glob.GetVarByID(dyn_cast<VarDecl>(ref->getFoundDecl()));
      return new EOObject{var.alias};
    }
    catch (invalid_argument &er) {
      cerr << er.what() << "\n";
      return new EOObject(EOObjectType::EO_PLUG);
    }
  } else if (stmtClass == Stmt::IfStmtClass) {
    const auto *op = dyn_cast<IfStmt>(p_stmt);
    return GetIfStmtEOObject(op);
  } else if (stmtClass == Stmt::WhileStmtClass) {
    const auto *op = dyn_cast<WhileStmt>(p_stmt);
    return GetWhileStmtEOObject(op);
  } else if (stmtClass == Stmt::DoStmtClass) {
    const auto *op = dyn_cast<DoStmt>(p_stmt);
    return GetDoWhileStmtEOObject(op);
  } else if (stmtClass == Stmt::CompoundStmtClass) {
    const auto *op = dyn_cast<CompoundStmt>(p_stmt);
    return GetCompoundStmt(op);
  } else if (stmtClass == Stmt::IntegerLiteralClass) {
    const auto *op = dyn_cast<IntegerLiteral>(p_stmt);
    return GetIntegerLiteralEOObject(op);
  } else if (stmtClass == Stmt::FloatingLiteralClass) {
    const auto *op = dyn_cast<FloatingLiteral>(p_stmt);
    return GetFloatingLiteralEOObject(op);
  } else if (stmtClass == Stmt::DeclStmtClass) {
    return new EOObject(EOObjectType::EO_EMPTY);
  } else if (stmtClass == Stmt::CallExprClass) {
    const auto *op = dyn_cast<CallExpr>(p_stmt);
    return GetFunctionCallEOObject(op);
  } else if (stmtClass == Stmt::ReturnStmtClass) {
    const auto *op = dyn_cast<ReturnStmt>(p_stmt);
    return GetReturnStmpEOObject(op);
  }
  return new EOObject(EOObjectType::EO_PLUG);
}

EOObject* GetFunctionCallEOObject(const CallExpr *op) {
  // EOObject call(EOObjectType::EO_EMPTY);
  EOObject* call = new EOObject("seq");
  vector<std::size_t> var_sizes;
  for (auto VD: op->getDirectCallee()->parameters()) {
    TypeInfo type_info = VD->getASTContext().getTypeInfo(VD->getType());
    size_t type_size = type_info.Width / 8;
    var_sizes.push_back(type_size);
  }
  // Формирование оператора, задающего последовательность действий
  size_t shift = 0;
  int i = 0;
  for (auto arg: op->arguments()) {
    EOObject* param = new EOObject{"write"};
    EOObject* address = new EOObject{"address"};
    address->nested.emplace_back(new EOObject{"global-ram"});
    EOObject* add = new EOObject{"add"};
    add->nested.emplace_back(new EOObject{"empty-local-position"});
    add->nested.emplace_back(new EOObject{to_string(shift), EOObjectType::EO_LITERAL});
    address->nested.push_back(add);
    param->nested.push_back(address);
    param->nested.push_back(GetStmtEOObject(arg));
    shift += var_sizes[i];
    // may be it will works with param.
    i = i == var_sizes.size() - 1 ? i : i + 1;
    call->nested.push_back(param);
  }
  call->nested.push_back(transpiler.func_manager.GetFunctionCall(op->getDirectCallee(), shift));

  std::string postfix = GetTypeName(op->getType());
  if (postfix != "undefinedtype") { // считается, что если тип не void,то генерация чтения данных нужна
    EOObject* read_ret = new EOObject{"read-as-" + postfix};
    EOObject* ret_val = new EOObject{"return"};
    read_ret->nested.push_back(ret_val);
    call->nested.push_back(read_ret);
  } else { // если тип void,то возвращается TRUE
    call->nested.emplace_back(new EOObject{"TRUE",EOObjectType::EO_LITERAL});
  }

  //!
//   #ifdef TRACEOUT_EO
//      TraceOutEOObject(*call);
//   #endif
  return call;
}

EOObject* GetFloatingLiteralEOObject(const FloatingLiteral *p_literal) {
  APFloat an_float = p_literal->getValue();
  ostringstream ss{};
  ss << fixed << an_float.convertToDouble();
  return new EOObject{ss.str(), EOObjectType::EO_LITERAL};
}

EOObject* GetIntegerLiteralEOObject(const IntegerLiteral *p_literal) {
  bool is_signed = p_literal->getType()->isSignedIntegerType();
  APInt an_int = p_literal->getValue();
  //! Тестовый вывод узла дерева
  #ifdef TRACEOUT
    TraceOutIntegerLiteral(an_int, is_signed);
  #endif
  // Формирование строкового значения для целого числа
  int64_t val = 0;
  if(is_signed) {
    val = an_int.getSExtValue();
  }
  else {
    val = an_int.getZExtValue();
  }
  std::string strVal{std::to_string(val)};
  return new EOObject{strVal, EOObjectType::EO_LITERAL};
//   return new EOObject{an_int.toString(10, is_signed), EOObjectType::EO_LITERAL};
}

EOObject* GetCompoundAssignEOObject(const CompoundAssignOperator *p_operator) {
  auto op_code = p_operator->getOpcode();
  std::string operation;

  if (op_code == BinaryOperatorKind::BO_AddAssign) {
    operation = "add";
  } else if (op_code == BinaryOperatorKind::BO_SubAssign) {
    operation = "sub";
  } else if (op_code == BinaryOperatorKind::BO_MulAssign) {
    operation = "mul";
  } else if (op_code == BinaryOperatorKind::BO_DivAssign) {
    operation = "div";
  } else if (op_code == BinaryOperatorKind::BO_RemAssign) {
    operation = "mod";
  } else if (op_code == BinaryOperatorKind::BO_AndAssign) {
    operation = "bit-and";
  } else if (op_code == BinaryOperatorKind::BO_XorAssign) {
    operation = "bit-xor";
  } else if (op_code == BinaryOperatorKind::BO_OrAssign) {
    operation = "bit-or";
  } else if (op_code == BinaryOperatorKind::BO_ShlAssign) {
    operation = "shift-left";
  } else if (op_code == BinaryOperatorKind::BO_ShrAssign) {
    operation = "shift-right";
  }

  EOObject* binop = new EOObject{operation};
  if (p_operator->getLHS()->getStmtClass() == Stmt::DeclRefExprClass) {
    auto ref = dyn_cast<DeclRefExpr>(p_operator->getLHS());
    if (!ref)
      binop->nested.emplace_back(new EOObject{EOObjectType::EO_PLUG});
    try {
      const Variable &var = transpiler.glob.GetVarByID(dyn_cast<VarDecl>(ref->getFoundDecl()));
      EOObject* eoObject = new EOObject{"read-as-" + var.type_postfix};
      EOObject* nestObj = new EOObject{var.alias};
      eoObject->nested.push_back(nestObj);
      binop->nested.emplace_back(eoObject);
    }
    catch (invalid_argument &er) {
      cerr << er.what() << "\n";
      binop->nested.emplace_back(new EOObject(EOObjectType::EO_PLUG));
    }
  } else {
    binop->nested.push_back(GetStmtEOObject(p_operator->getLHS()));
  }
  binop->nested.push_back(GetStmtEOObject(p_operator->getRHS()));
  return binop;
}

EOObject* GetBinaryStmtEOObject(const BinaryOperator *p_operator) {
  auto opCode = p_operator->getOpcode();
  //! Тестовый вывод бинарного оператора, совмещенного с присваиванием
  #ifdef TRACEOUT
    TraceOutBinaryOperator(opCode);
  #endif
  std::string operation;
  if (opCode == BinaryOperatorKind::BO_Assign) {
    return GetAssignmentOperatorEOObject(p_operator);
  } else if (opCode == BinaryOperatorKind::BO_Add) {
    // Усложняется, так как нужно еще учесть указатель
    operation = "add";
  } else if (opCode == BinaryOperatorKind::BO_Sub) {
    operation = "sub";
  } else if (opCode == BinaryOperatorKind::BO_Mul) {
    operation = "mul";
  } else if (opCode == BinaryOperatorKind::BO_Div) {
    operation = "div";
  } else if (opCode == BinaryOperatorKind::BO_Rem) {
    operation = "mod";
  } else if (opCode == BinaryOperatorKind::BO_And) {
    operation = "bit-and";
  } else if (opCode == BinaryOperatorKind::BO_Or) {
    operation = "bit-or";
  } else if (opCode == BinaryOperatorKind::BO_Xor) {
    operation = "bit-xor";
  } else if (opCode == BinaryOperatorKind::BO_Shl) {
    operation = "shift-left";
  } else if (opCode == BinaryOperatorKind::BO_Shr) {
    operation = "shift-right";
  } else if (opCode == BinaryOperatorKind::BO_EQ) {
    operation = "eq";
  } else if (opCode == BinaryOperatorKind::BO_NE) {
    operation = "neq";
  } else if (opCode == BinaryOperatorKind::BO_LT) {
    operation = "less";
  } else if (opCode == BinaryOperatorKind::BO_LE) {
    operation = "leq";
  } else if (opCode == BinaryOperatorKind::BO_GT) {
    operation = "greater";
  } else if (opCode == BinaryOperatorKind::BO_GE) {
    operation = "geq";
  } else {
    operation = "undefined";
  }

  EOObject* binop = new EOObject{operation};
  binop->nested.push_back(GetStmtEOObject(p_operator->getLHS()));
  binop->nested.push_back(GetStmtEOObject(p_operator->getRHS()));
  return binop;
}

EOObject* GetUnaryStmtEOObject(const UnaryOperator *p_operator) {
  auto opCode = p_operator->getOpcode();
  std::string operation;
  Stmt *stmt = nullptr;

  // [C99 6.5.2.4] Postfix increment and decrement
  if (opCode == UnaryOperatorKind::UO_PostInc) { // UNARY_OPERATION(PostInc, "++")
    std::string postfix = GetTypeName(p_operator->getType());
    EOObject* variable = new EOObject{"post-inc-" + postfix};
    variable->nested.push_back(GetStmtEOObject(p_operator->getSubExpr()));
    return variable;
  } else if (opCode == UnaryOperatorKind::UO_PostDec) { // UNARY_OPERATION(PostDec, "--")
    std::string postfix = GetTypeName(p_operator->getType());
    EOObject* variable = new EOObject{"post-dec-" + postfix};
    variable->nested.push_back(GetStmtEOObject(p_operator->getSubExpr()));
    return variable;
    // [C99 6.5.3.1] Prefix increment and decrement
  } else if (opCode == UnaryOperatorKind::UO_PreInc) { // UNARY_OPERATION(PreInc, "++")
    std::string postfix = GetTypeName(p_operator->getType());
    EOObject* variable = new EOObject{"pre-inc-" + postfix};
    variable->nested.push_back(GetStmtEOObject(p_operator->getSubExpr()));
    return variable;
  } else if (opCode == UnaryOperatorKind::UO_PreDec) { // UNARY_OPERATION(PreDec, "--")
    std::string postfix = GetTypeName(p_operator->getType());
    EOObject* variable = new EOObject{"pre-dec-" + postfix};
    variable->nested.push_back(GetStmtEOObject(p_operator->getSubExpr()));
    return variable;
    // [C99 6.5.3.2] Address and indirection
  } else if (opCode == UnaryOperatorKind::UO_AddrOf) { // UNARY_OPERATION(AddrOf, "&")
    // stmt = p_operator->getSubExpr();
    EOObject* variable = new EOObject{"addr-of"};
    variable->nested.push_back(GetStmtEOObject(p_operator->getSubExpr()));
    return variable;
  } else if (opCode == UnaryOperatorKind::UO_Deref) { // UNARY_OPERATION(Deref, "*")
    EOObject* variable = new EOObject{"address"};
    EOObject* ram = new EOObject{"global-ram"};
    variable->nested.push_back(ram);
    variable->nested.push_back(GetStmtEOObject(p_operator->getSubExpr()));
    return variable;
    // operation = "read-as-address";
    // EOObject* unoop {operation};
    return GetStmtEOObject(p_operator->getSubExpr());
    // [C99 6.5.3.3] Unary arithmetic
  } else if (opCode == UnaryOperatorKind::UO_Plus) { // UNARY_OPERATION(Plus, "+")
    operation = "plus";
  } else if (opCode == UnaryOperatorKind::UO_Minus) { // UNARY_OPERATION(Minus, "-")
    operation = "neg";
  } else if (opCode == UnaryOperatorKind::UO_Not) { // UNARY_OPERATION(Not, "~")
    operation = "bit-not";
  } else if (opCode == UnaryOperatorKind::UO_LNot) { // UNARY_OPERATION(LNot, "!")
    operation = "not";
    // "__real expr"/"__imag expr" Extension.
  } else if (opCode == UnaryOperatorKind::UO_Real) { // UNARY_OPERATION(Real, "__real")
    operation = "real";
  } else if (opCode == UnaryOperatorKind::UO_Imag) { // UNARY_OPERATION(Imag, "__imag")
    operation = "imag";
    // __extension__ marker.
  } else if (opCode == UnaryOperatorKind::UO_Extension) { // UNARY_OPERATION(Extension, "__extension__")
    operation = "extension";
    // [C++ Coroutines] co_await operator
  } else if (opCode == UnaryOperatorKind::UO_Coawait) { // UNARY_OPERATION(Coawait, "co_await")
    operation = "coawait";
    // Incorrect unary operator
  } else {
    operation = "undefined";
  }

  //! Тестовый вывод бинарного оператора, совмещенного с присваиванием
  std::cout << "  Unary operator = " << operation << "\n";

  EOObject* unoop = new EOObject{operation};
  unoop->nested.push_back(GetStmtEOObject(p_operator->getSubExpr()));
  return unoop;
}

EOObject* GetAssignmentOperatorEOObject(const BinaryOperator *p_operator) {
  std::string postfix = GetTypeName(p_operator->getType());
  EOObject* binop = new EOObject{"write-as-" + postfix};
/*
  const auto *op = dyn_cast<DeclRefExpr>(p_operator->getLHS());
  // Здесь нужны проверки на разные типы левого выражения. Не только DeclRefExpr
  if (op) {
    try {
      string type = op->getType()->isPointerType() ? "ptr" : GetTypeName(op->getType());
      binop->name += type;
      const Variable &var = transpiler.glob.GetVarByID(dyn_cast<VarDecl>(op->getFoundDecl()));
      binop->nested.emplace_back(new EOObject{var.alias});
    }
    catch (invalid_argument &) {
      binop->nested.emplace_back(new EOObject{EOObjectType::EO_LITERAL});
    }
  } else {
    binop->nested.emplace_back(new EOObject{EOObjectType::EO_EMPTY});
  }
*/
  binop->nested.push_back(GetStmtEOObject(p_operator->getLHS()));
  binop->nested.push_back(GetStmtEOObject(p_operator->getRHS()));
  return binop;
}

EOObject* GetAssignmentOperationOperatorEOObject(const CompoundAssignOperator *p_operator) {
  EOObject* binop = new EOObject{"write-as-"};
  const auto *op = dyn_cast<DeclRefExpr>(p_operator->getLHS());
  if (op) {
    try {
      string type = op->getType()->isPointerType() ? "ptr" : GetTypeName(op->getType());
      binop->name += type;
      const Variable &var = transpiler.glob.GetVarByID(dyn_cast<VarDecl>(op->getFoundDecl()));
      binop->nested.emplace_back(new EOObject{var.alias});
    }
    catch (invalid_argument &) {
      binop->nested.emplace_back(new EOObject{EOObjectType::EO_LITERAL});
    }
  } else {
    binop->nested.emplace_back(new EOObject{EOObjectType::EO_EMPTY});
  }

  binop->nested.push_back(GetCompoundAssignEOObject(p_operator));
  return binop;
}

EOObject* GetReturnStmpEOObject(const ReturnStmt *p_stmt) {
  // TODO: Нужно сделать write-as-...
  EOObject* ret = new EOObject{"write"};
  EOObject* addr = new EOObject{"return"};
  ret->nested.push_back(addr);
  ret->nested.push_back(GetStmtEOObject(p_stmt->getRetValue()));
  return ret;
}

EOObject* GetIfStmtEOObject(const IfStmt *p_stmt) {
  EOObject* if_stmt = new EOObject{"if"};
  if_stmt->nested.push_back(GetStmtEOObject(p_stmt->getCond()));
  // TODO then and else is seq everytime!
  if_stmt->nested.push_back(GetStmtEOObject(p_stmt->getThen()));
  if (p_stmt->hasElseStorage()) {
    if_stmt->nested.push_back(GetStmtEOObject(p_stmt->getElse()));
  } else {
    EOObject* empty_seq = new EOObject{"seq"};
    empty_seq->nested.emplace_back(new EOObject{"TRUE", EOObjectType::EO_LITERAL});
    if_stmt->nested.push_back(empty_seq);
  }
  return if_stmt;
}

EOObject* GetWhileStmtEOObject(const WhileStmt *p_stmt) {
  EOObject* while_stmt = new EOObject{"while"};
  while_stmt->nested.push_back(GetStmtEOObject(p_stmt->getCond()));
  // TODO body is seq everytime!
  while_stmt->nested.push_back(GetStmtEOObject(p_stmt->getBody()));
  return while_stmt;
}

EOObject* GetDoWhileStmtEOObject(const DoStmt *p_stmt) {
  EOObject* do_stmt = new EOObject{"seq"};
  do_stmt->nested.push_back(GetStmtEOObject(p_stmt->getBody()));
  EOObject* while_stmt = new EOObject{"while"};
  while_stmt->nested.push_back(GetStmtEOObject(p_stmt->getCond()));
  // TODO body is seq everytime!
  while_stmt->nested.push_back(GetStmtEOObject(p_stmt->getBody()));
  do_stmt->nested.push_back(while_stmt);
  do_stmt->nested.emplace_back(new EOObject{"TRUE", EOObjectType::EO_LITERAL});
  return do_stmt;
}

std::string GetTypeName(QualType qualType) {
  extern ASTContext *context;
  const clang::Type *typePtr = qualType.getTypePtr();
  TypeInfo typeInfo = context->getTypeInfo(typePtr);
  uint64_t typeSize = typeInfo.Width;
  std::string str{""};

  if (typePtr->isBooleanType()) {
    str += "bool";
    return str;
  }

  if (typePtr->isPointerType()) {
    // str += "int64";
    str += "ptr";
    return str;
  }

  if (typePtr->isFloatingType()) {
    str += "float" + std::to_string(typeSize);
    return str;
  }

  if (!typePtr->isSignedIntegerType())
    str += "u";
  if (typePtr->isCharType()) {
    str += "char";
    return str;
  }
  if (typePtr->isIntegerType()) {
    str += "int" + std::to_string(typeSize);
    return str;
  }


  if (typePtr->isUnionType())
    str = "un_";
  if (typePtr->isStructureType())
    str = "st_";
  if (typePtr->isUnionType() || typePtr->isStructureType()) {
    RecordDecl *RD = typePtr->getAsRecordDecl();
    if (RD->hasNameForLinkage())
      str += RD->getNameAsString();
    else
      str += std::to_string(reinterpret_cast<uint64_t>(RD));
    return str;
  }

  return "undefinedtype";
}

std::set<std::string> FindAllExternalObjects(const EOObject &obj) {
  std::set<std::string> all_known = {obj.postfix};
  std::set<std::string> unknown = {};
  // TODO maybe should use pointers or copy constructor to avoid unnecessary copying of objects
  std::queue<EOObject*> not_visited;
  for (auto child: obj.nested) {
    not_visited.push(std::move(child));
  }
  while (!not_visited.empty()) {
    EOObject* cur = not_visited.front();
    not_visited.pop();
    switch (cur->type) {
      case EOObjectType::EO_ABSTRACT:

        all_known.insert(cur->postfix);
        for (const auto &arg: cur->arguments)
          all_known.insert(arg);
        break;
      case EOObjectType::EO_COMPLETE:
        all_known.insert(cur->postfix);
        if (all_known.find(cur->name) == all_known.end())
          unknown.insert(cur->name);
        break;
      case EOObjectType::EO_EMPTY:
      case EOObjectType::EO_LITERAL:
        break;
      case EOObjectType::EO_PLUG:
        if (cur->nested.empty())
          unknown.insert("plug");
        break;
    }
    for (auto child: cur->nested) {
      not_visited.push(std::move(child));
    }
  }
  for (const auto &known_obj: all_known) {
    unknown.erase(known_obj);
  }

  return unknown;
}

