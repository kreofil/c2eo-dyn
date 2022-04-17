#ifndef __FUNCTION_MANAGER__
#define __FUNCTION_MANAGER__

#include <cstdint>
#include <string>
#include <utility>
#include "clang/AST/Decl.h"
#include "eo_object.h"
#include "memory_manager.h"

// Representation of a simple function
struct FunctionDefinition {
  const clang::FunctionDecl *FD;
  std::string name;

  EOObject* GetEOObject() const;

  // Тестовая функция, осуществляющая вывод содержимого функции
  // и ее тела (добавление будет по ходу тестирования)
  void TestOut();

private:
  EOObject* GetBody() const;
};

struct FunctionDeclaration {
  const clang::FunctionDecl *FD;
  std::string name;

  // Тестовая функция, осуществляющая вывод объявления функции
  void TestOut();
};


struct FunctionManager {
  FunctionManager() {
//     definitions.reserve(1000);
  }
  void Add(const clang::FunctionDecl *FD);
  const std::vector<FunctionDefinition> &GetAllDefinitions();
  const std::vector<FunctionDefinition> *GetPtrAllDefinitions();
//   const std::list<FunctionDefinition> &GetAllDefinitions();
//   const std::list<FunctionDefinition> *GetPtrAllDefinitions();
  EOObject* GetFunctionCall(const clang::FunctionDecl* FD, size_t param_size);

  // Тестовый вывод всех определений и прототипов функций
  void TestOut();

private:
  std::vector<FunctionDefinition> definitions;
  std::vector<FunctionDeclaration> declarations;
//   std::list<FunctionDefinition> definitions;
//   std::list<FunctionDeclaration> declarations;
};

#endif // __FUNCTION_MANAGER__
