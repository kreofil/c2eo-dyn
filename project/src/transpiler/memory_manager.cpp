#include <algorithm>
#include <exception>
#include <stdexcept>
#include "memory_manager.h"

using namespace std;
using namespace clang;


Variable MemoryManager::Add(const VarDecl *id, size_t size, const std::string &type,
                            std::string alias, std::string value, std::string local_name,
                            size_t shift, bool is_initialized)
{
  Variable var{id, pointer, size, type, std::move(alias), std::move(value),
               std::move(local_name), shift, type.substr(2), is_initialized};
  // TODO fix this plug
  if (var.value.empty()) {
    var.value = "plug";
  }
  variables.push_back(var);
  pointer += size;
  return var;
}

bool MemoryManager::Empty() {
  return variables.empty();
}

size_t MemoryManager::RealMemorySize() {
  size_t result = 0;
  for (const auto &v: variables) {
    result += v.size;
  }
  return result;
}

std::vector<Variable>::const_iterator MemoryManager::begin() const {
  return variables.begin();
}

std::vector<Variable>::const_iterator MemoryManager::end() const {
  return variables.end();
}

const Variable &MemoryManager::GetVarByID(const VarDecl *id) const {
  auto res = find_if(variables.begin(), variables.end(),
                     [id](const Variable &x) { return x.id == id; });
  if (res == variables.end())
    throw invalid_argument("element with id " + to_string(reinterpret_cast<uint64_t>(id)) + " not found");
  return *res;
}

EOObject* MemoryManager::GetEOObject() const
{
  EOObject* res = new EOObject{"ram"};
  res->postfix = name;
  res->nested.push_back(new EOObject{to_string(mem_size), EOObjectType::EO_LITERAL});
  return res;
}


void MemoryManager::RemoveAllUsed (std::vector<Variable> &all_local) {
  for (const auto &var: all_local) {
    pointer -= var.size;
    variables.erase(find(variables.begin(), variables.end(), var));
  }
}

EOObject *Variable::GetInitializer() const {
  if (!is_initialized)
    return new EOObject(EOObjectType::EO_EMPTY);
  EOObject *res = new EOObject("write");
  res->nested.emplace_back(new EOObject{alias});
  // TODO if value will be EOObject then code below changed.
  if (value == "plug")
    res->nested.emplace_back(new EOObject{EOObjectType::EO_PLUG});
  else
    res->nested.emplace_back(new EOObject{value, EOObjectType::EO_LITERAL});
  return res;
}

EOObject* Variable::GetAddress(string mem_name) const {
  EOObject *addr = new EOObject("address", alias);
  if (!mem_name.empty())
    addr->nested.emplace_back(new EOObject{mem_name});
  if (!local_pointer.empty()) {
    EOObject *shift_obj = new EOObject("add");
    shift_obj->nested.emplace_back(new EOObject{local_pointer});
    // TODO may be, this don't work with dynamic memory allocation, but probably also should work
    shift_obj->nested.emplace_back(new EOObject{to_string(position - shift), EOObjectType::EO_LITERAL});
    addr->nested.push_back(shift_obj);
  } else {
    addr->nested.emplace_back(new EOObject{to_string(position), EOObjectType::EO_LITERAL});
  }
  return addr;
}

bool Variable::operator==(const Variable &var) const {
  return this->id == var.id;
}
