#ifndef __EO_OBJECT__
#define __EO_OBJECT__

#include <iostream>
#include <ostream>
#include <vector>
#include <list>
#include <string>

enum class EOObjectType {
  EO_EMPTY,
  EO_COMPLETE,
  EO_LITERAL,
  EO_ABSTRACT,
  EO_PLUG,
};


struct EOObject {
public:
  explicit EOObject(EOObjectType type);

  // Create simple complete Object
  explicit EOObject(std::string name);

  // Create simple object, may be used for literal
  EOObject(std::string name, EOObjectType type);

  // create complete name with body
  EOObject(std::string name, std::string postfix);

  // Add nested object to vector of nested
  void AddNested(EOObject *obj);

  // Добавление вложенного объекта в голову вектора
  void AddToHeadInNested(EOObject *obj);

  std::vector<std::string> arguments;
//   std::list<std::string> arguments;
  std::string name;
  std::string prefix;
  std::string postfix;
  EOObjectType type;
//   std::vector<EOObject*> nested;
  std::list<EOObject*> nested;

  friend std::ostream &operator<<(std::ostream &os, const EOObject &obj);

private:
  static std::string GetSpaceIndent();

  static int indent;
};


EOObject* createSeq();


#endif // __EO_OBJECT__
