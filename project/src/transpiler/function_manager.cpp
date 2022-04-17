#include <iostream>
#include "function_manager.h"
#include "transpile_helper.h"

using namespace clang;
using namespace llvm;

//------------------------------------------------------------------------------
EOObject* FunctionDefinition::GetEOObject() const {
  EOObject *func_object = new EOObject(EOObjectType::EO_ABSTRACT);
  func_object->postfix = name;
  func_object->arguments = {"param-start", "param-size"};
  func_object->nested.push_back(GetBody());
  return func_object;
}

//------------------------------------------------------------------------------
EOObject* FunctionDefinition::GetBody() const
{
  return GetFunctionBody(FD);
}

//------------------------------------------------------------------------------
// Тестовая функция, осуществляющая вывод содержимого функции
// и ее тела (добавление будет по ходу тестирования)
void FunctionDefinition::TestOut() {
  std::cout << name << "\n";
  if (FD == nullptr) {
    std::cout << "  Incorrect pointer to definition\n";
  }
  else {
    // Вывод содержимого функции
    DeclarationNameInfo declNameInfo{FD->getNameInfo()};
    std::string func_name{declNameInfo.getAsString()};
//     std::string func_name{FD->getNameAsString()};
    std::cout << func_name << ": ";
    std::cout.flush();

    if(FD->isDefined()) {
      std::cout << "  Defined!\n";
    }
    else {
      std::cout << "  No defined!\n";
    }

    if(FD->hasBody()) {
      std::cout << "  Has body!\n";
      Stmt* body = FD->getBody();
      CompoundStmt* funcBody = dyn_cast<CompoundStmt>(body);
      if(funcBody != nullptr) {
        std::cout << "  Has body! Body pointer =  " << body << "\n";
        if(funcBody->size() > 0) {
          int i = 0;
          for (auto stmt: funcBody->body()) {
            Stmt::StmtClass stmtClass = stmt->getStmtClass();
            std::cout << "    Statement # " << i++ << "\n";
          }
        }
        else {
          std::cout << "    The body is empty\n";
        }
      }
    }
    else {
      std::cout << "  Body is absent!\n";
    }

  }
}

//------------------------------------------------------------------------------
// Тестовая функция, осуществляющая вывод прототипа функции
// и ее тела (добавление будет по ходу тестирования)
void FunctionDeclaration::TestOut() {
  std::cout << name << "\n";
  if (FD == nullptr) {
    std::cout << "  Incorrect pointer to definition\n";
  }
  else {
    // Вывод содержимого функции
    DeclarationNameInfo declNameInfo{FD->getNameInfo()};
    std::string func_name{declNameInfo.getAsString()};
    std::cout << func_name << "\n";
//     std::cout << FD->getNameAsString() << "\n";
  }
}

//------------------------------------------------------------------------------
void FunctionManager::Add(const clang::FunctionDecl *FD) {
  DeclarationNameInfo declNameInfo{FD->getNameInfo()};
  std::string funcName{declNameInfo.getAsString()};
//   auto funcName = FD->getNameAsString();
  if (funcName != "main")
    funcName = "f-" + funcName;
  bool isDefinition = FD->isThisDeclarationADefinition();
//   if (std::find_if(definitions.begin(), definitions.end(),
//                    [&funcName](FunctionDefinition f) { return f.name == funcName; }) != definitions.end()) {
//     return;
//   }
  if (isDefinition) {
////     auto declaration = std::remove_if(declarations.begin(), declarations.end(),
////                                       [&funcName](FunctionDeclaration f) { return f.name == funcName; });
////     declarations.erase(declaration, declarations.end());
//   if (FD->isDefined()) {
    definitions.push_back({FD, funcName});
  } else {
    declarations.push_back({FD, funcName});
  }

}

//------------------------------------------------------------------------------
const std::vector<FunctionDefinition> &FunctionManager::GetAllDefinitions() {
// const std::list<FunctionDefinition> &FunctionManager::GetAllDefinitions() {
  return definitions;
}

//------------------------------------------------------------------------------
const std::vector<FunctionDefinition> *FunctionManager::GetPtrAllDefinitions() {
// const std::list<FunctionDefinition> *FunctionManager::GetPtrAllDefinitions() {
  return &definitions;
}

//------------------------------------------------------------------------------
EOObject* FunctionManager::GetFunctionCall(const clang::FunctionDecl* FD, size_t param_size)
{
  auto res_def = std::find_if(definitions.begin(), definitions.end(), [&FD](const FunctionDefinition &decl) {
    return decl.FD == FD;
  });
  if (res_def != definitions.end()) {
    EOObject* call = new EOObject(res_def->name);
    call->prefix = "^";
    call->nested.emplace_back(new EOObject{"empty-local-position"});
    call->nested.emplace_back(new EOObject{std::to_string(param_size), EOObjectType::EO_LITERAL});
    return call;
  }
  auto res_decl = std::find_if(declarations.begin(), declarations.end(), [&FD](const FunctionDeclaration &decl) {
    return decl.FD == FD;
  });
  if (res_decl != declarations.end()) {
    EOObject* call = new EOObject(res_decl->name);
    call->prefix = "^";
    call->nested.emplace_back(new EOObject{"empty-local-position"});
    call->nested.emplace_back(new EOObject{std::to_string(param_size), EOObjectType::EO_LITERAL});
    return call;
  }
  return new EOObject(EOObjectType::EO_PLUG);
}


// Тестовый вывод содержимого контейнеров объявления и определения функций
void FunctionManager::TestOut() {
  if (declarations.size() > 0) {
    for(auto declaration: declarations) {
      declaration.TestOut();
    }
  }
  else {
    std::cout << "No function declarations\n";
  }
  if (definitions.size() > 0) {
    for(auto definition: definitions) {
      definition.TestOut();
    }
  }
  else {
    std::cout << "No function definitions\n";
  }
}

