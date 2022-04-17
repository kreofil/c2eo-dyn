#include <sstream>
#include "unit_transpiler.h"
#include "transpile_helper.h"
#include "aliases.h"
#include "tracer.h"

std::ostream &operator<<(std::ostream &os, UnitTranspiler unit) {
  ////EOObject test{"test", "start-of-output"};
  ////TraceOutEOObject(test);

  // Предварительный тест на наличие функций и прототипов в контейнерах
//   unit.func_manager.TestOut();

  if (unit.tmp.empty()) {
    unit.GenerateResult();
  }
  os << unit.tmp;

//   if (unit.tmp.empty()) {
//     unit.GetResult(os);
//   }

//   if (unit.tmp.empty()) {
//     unit.ResultToString();
//   }
//   os << unit.tmp;

  return os;
}

void UnitTranspiler::GenerateResult() {
  EOObject *body = new EOObject(EOObjectType::EO_ABSTRACT);
  body->arguments.push_back("args...");
  body->postfix = "global";
  body->nested.push_back(glob.GetEOObject());
  //body->nested.push_back(new EOObject{"memory", "empty-global-position"});
  body->nested.push_back(new EOObject{"memory", "empty-global-position"});
  body->nested.push_back(ret.GetEOObject());
  //body->nested.push_back(new EOObject{"memory", "return-mem_size"});
  body->nested.push_back(new EOObject{"memory", "return-mem_size"});
  EOObject *ret_addr = new EOObject("address", "return");
  ret_addr->nested.push_back(new EOObject{ret.name});
  ret_addr->nested.push_back(new EOObject{"0", EOObjectType::EO_LITERAL});
  body->nested.push_back(ret_addr);

  if (!glob.Empty()) {
    for (const auto &var: glob) {
      body->nested.push_back(var.GetAddress(glob.name));
    }
  }

//   func_manager.TestOut();

  // TODO write all declarations
  auto definitons = func_manager.GetAllDefinitions();
  for (const auto &func: definitons) {
    body->nested.push_back(func.GetEOObject());
  }

  EOObject* init_seq = new EOObject("seq", "@");
  for (const auto &var: glob) {
    init_seq->nested.push_back(var.GetInitializer());
  }
  if (std::find_if(body->nested.begin(), body->nested.end(),
                   [](EOObject* x) { return x->postfix == "main"; }) != body->nested.end()) {
    EOObject* main_call = new EOObject("main");
    extern UnitTranspiler transpiler;
    main_call->nested.push_back(new EOObject{std::to_string(transpiler.glob.RealMemorySize()), EOObjectType::EO_LITERAL});
    main_call->nested.push_back(new EOObject{"0", EOObjectType::EO_LITERAL});
    init_seq->nested.push_back(main_call);
  }

  init_seq->nested.push_back(new EOObject{"TRUE", EOObjectType::EO_LITERAL});

  body->nested.push_back(init_seq);

  std::stringstream result;
  result << "+package c2eo.src." << package_name << "\n";

  used_external_objects = FindAllExternalObjects(*body);
  for (auto &ext_obj: used_external_objects) {
    if (known_types.find(ext_obj) == known_types.end()) {
      std::string alias;
      try {
        alias = known_aliases.at(ext_obj);
        result << alias << "\n";
      }
      catch (std::out_of_range &) {
        llvm::errs() << "Not found alias for " << ext_obj << "\n";
      }
    }
  }
  result << "\n";
  result << *body;
  tmp = result.str();
}

void UnitTranspiler::SetPackageName(std::string packagename) {
  package_name = std::move(packagename);
}

void UnitTranspiler::GetResult(std::ostream &result) {
  EOObject *body = new EOObject(EOObjectType::EO_ABSTRACT);
  body->arguments.push_back("args...");
  body->postfix = "global";
  body->nested.push_back(glob.GetEOObject());
  //body->nested.push_back(new EOObject{"memory", "empty-global-position"});
  body->nested.push_back(new EOObject{"memory", "empty-global-position"});
  body->nested.push_back(ret.GetEOObject());
  //body->nested.push_back(new EOObject{"memory", "return-mem_size"});
  body->nested.push_back(new EOObject{"memory", "return-mem_size"});
  EOObject *ret_addr = new EOObject("address", "return");
  ret_addr->nested.push_back(new EOObject{ret.name});
  ret_addr->nested.push_back(new EOObject{"0", EOObjectType::EO_LITERAL});
  body->nested.push_back(ret_addr);

  if (!glob.Empty()) {
    for (const auto &var: glob) {
      //body->nested.push_back(var.GetAddress(glob.name));
      body->nested.push_back(var.GetAddress(glob.name));
    }
  }
  // TODO write all declarations
  for (const auto &func: func_manager.GetAllDefinitions()) {
    body->nested.push_back(func.GetEOObject());
  }

  EOObject* init_seq = new EOObject("seq", "@");
  for (const auto &var: glob) {
    init_seq->nested.push_back(var.GetInitializer());
  }
  if (std::find_if(body->nested.begin(), body->nested.end(),
                   [](EOObject* x) { return x->postfix == "main"; }) != body->nested.end()) {
    EOObject* main_call = new EOObject("main");
    extern UnitTranspiler transpiler;
    main_call->nested.push_back(new EOObject{std::to_string(transpiler.glob.RealMemorySize()), EOObjectType::EO_LITERAL});
    main_call->nested.push_back(new EOObject{"0", EOObjectType::EO_LITERAL});
    init_seq->nested.push_back(main_call);
  }

  init_seq->nested.push_back(new EOObject{"TRUE", EOObjectType::EO_LITERAL});

  body->nested.push_back(init_seq);

  ////std::stringstream result;
  result << "+package c2eo.src." << package_name << "\n";

  used_external_objects = FindAllExternalObjects(*body);
  for (auto &ext_obj: used_external_objects) {
    if (known_types.find(ext_obj) == known_types.end()) {
      std::string alias;
      try {
        alias = known_aliases.at(ext_obj);
        result << alias << "\n";
      }
      catch (std::out_of_range &) {
        llvm::errs() << "Not found alias for " << ext_obj << "\n";
      }
    }
  }
  result << "\n";
  result << *body;
  ////tmp = result.str();
}

void UnitTranspiler::ResultToString() {
  EOObject *body = new EOObject(EOObjectType::EO_ABSTRACT);
  body->arguments.push_back("args...");
  body->postfix = "global";
  body->nested.push_back(glob.GetEOObject());
  //body->nested.push_back(new EOObject{"memory", "empty-global-position"});
  body->nested.push_back(new EOObject{"memory", "empty-global-position"});
  body->nested.push_back(ret.GetEOObject());
  //body->nested.push_back(new EOObject{"memory", "return-mem_size"});
  body->nested.push_back(new EOObject{"memory", "return-mem_size"});
  EOObject *ret_addr = new EOObject("address", "return");
  ret_addr->nested.push_back(new EOObject{ret.name});
  ret_addr->nested.push_back(new EOObject{"0", EOObjectType::EO_LITERAL});
  body->nested.push_back(ret_addr);

  if (!glob.Empty()) {
    for (const auto &var: glob) {
      //body->nested.push_back(var.GetAddress(glob.name));
      body->nested.push_back(var.GetAddress(glob.name));
    }
  }
  // TODO write all declarations
  for (const auto &func: func_manager.GetAllDefinitions()) {
    body->nested.push_back(func.GetEOObject());
  }

  EOObject* init_seq = new EOObject("seq", "@");
  for (const auto &var: glob) {
    init_seq->nested.push_back(var.GetInitializer());
  }
  if (std::find_if(body->nested.begin(), body->nested.end(),
                   [](EOObject* x) { return x->postfix == "main"; }) != body->nested.end()) {
    EOObject* main_call = new EOObject("main");
    extern UnitTranspiler transpiler;
    main_call->nested.push_back(new EOObject{std::to_string(transpiler.glob.RealMemorySize()), EOObjectType::EO_LITERAL});
    main_call->nested.push_back(new EOObject{"0", EOObjectType::EO_LITERAL});
    init_seq->nested.push_back(main_call);
  }

  init_seq->nested.push_back(new EOObject{"TRUE", EOObjectType::EO_LITERAL});

  body->nested.push_back(init_seq);

  tmp += "+package c2eo.src." + package_name + "\n";

  used_external_objects = FindAllExternalObjects(*body);
  for (auto &ext_obj: used_external_objects) {
    if (known_types.find(ext_obj) == known_types.end()) {
      std::string alias;
      try {
        alias = known_aliases.at(ext_obj);
        tmp += alias + "\n";
      }
      catch (std::out_of_range &) {
        llvm::errs() << "Not found alias for " << ext_obj << "\n";
      }
    }
  }
  tmp += "\n";
  std::stringstream body_as_stream;
  body_as_stream << *body;
  std::string body_as_string{body_as_stream.str()};
  tmp += body_as_string;
}
