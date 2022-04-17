#include "eo_object.h"
#include "util.h"
#include "tracer.h"

using namespace std;

int EOObject::indent = 0;

EOObject::EOObject(EOObjectType type) : type(type) {
//   nested.reserve(10000);
  #ifdef TRACEOUT_NEW_EO
    std::cout << *this; // << "\n";
  #endif
}

// Create simple complete Object
EOObject::EOObject(std::string name) :
    name(std::move(name)),
    type(EOObjectType::EO_COMPLETE) {
//   nested.reserve(10000);
  #ifdef TRACEOUT_NEW_EO
    std::cout << *this; // << "\n";
  #endif
}

// Create simple object, may be used for literal
EOObject::EOObject(std::string name, EOObjectType type) : name(std::move(name)), type(type) {
//   nested.reserve(10000);
  #ifdef TRACEOUT_NEW_EO
    std::cout << *this; // << "\n";
  #endif
}

// create complete name with body
EOObject::EOObject(std::string name, std::string postfix) :
    name(std::move(name)),
    postfix(std::move(postfix)),
    type(EOObjectType::EO_COMPLETE) {
//   nested.reserve(10000);
  #ifdef TRACEOUT_NEW_EO
    std::cout << *this; // << "\n";
  #endif
}

void EOObject::AddNested(EOObject* obj)
{
  nested.push_back(obj);
}

// Добавление вложенного объекта в голову вектора
void EOObject::AddToHeadInNested(EOObject* obj)
{
  nested.insert(nested.begin(), obj);
}

std::string EOObject::GetSpaceIndent() {
  return std::string(2 * EOObject::indent, ' ');
}

std::ostream &operator<<(ostream &os, const EOObject &obj) {
  if (obj.type == EOObjectType::EO_EMPTY) {
    for (const auto child: obj.nested) {
      os << *child;
    }
    os.flush();
    return os;
  }

  os << EOObject::GetSpaceIndent();

  if (obj.type == EOObjectType::EO_PLUG) {
    os << "plug" << "\n";
    os.flush();
    return os;
  }

  if (obj.type == EOObjectType::EO_ABSTRACT) {
    os << "[" << str_join(obj.arguments) << "]";
  } else {
    if (!obj.prefix.empty()) {
      os << obj.prefix << ".";
    }
    os << obj.name;
  }
  if (!obj.postfix.empty()) {
    os << " > " << obj.postfix;
  }
  os << "\n";
  if (!obj.nested.empty()) {
    EOObject::indent++;
    for (const auto &child: obj.nested) {
      os << *child;
    }
    EOObject::indent--;
  }
  os.flush();
  return os;
}


// Temporary test function
EOObject* createSeq() {
  EOObject* eo = new EOObject(EOObjectType::EO_ABSTRACT);
  eo->postfix = "global";
  eo->arguments = {"args..."};

  EOObject* glob_ram = new EOObject("ram", "global_ram");
  glob_ram->AddNested(new EOObject{"2048", EOObjectType::EO_LITERAL});
  eo->AddNested(glob_ram);

  eo->AddNested(new EOObject{"memory", "empty-global-position"});

  EOObject *ret_ram = new EOObject("ram", "return-ram");
  ret_ram->AddNested(new EOObject{"1024", EOObjectType::EO_LITERAL});
  eo->AddNested(ret_ram);

  eo->AddNested(new EOObject{"memory", "return-mem_size"});

  EOObject* ret = new EOObject("address", "return");
  ret->AddNested(new EOObject{"return-ram"});
  ret->AddNested(new EOObject{"0", EOObjectType::EO_LITERAL});
  eo->AddNested(ret);

  EOObject* x = new EOObject("address", "x");
  x->AddNested(new EOObject{"global-ram"});
  x->AddNested(new EOObject{"0", EOObjectType::EO_LITERAL});
  eo->AddNested(x);

  EOObject* main = new EOObject{EOObjectType::EO_ABSTRACT};
  main->postfix = "main";
  EOObject* seq1 = new EOObject{"seq", "@"};
  EOObject* write1 = new EOObject{"write", EOObjectType::EO_COMPLETE};
  write1->AddNested(new EOObject{"x"});
  EOObject* add1 = new EOObject{"add"};
  EOObject* rai64 = new EOObject{"read-as-int64"};
  rai64->AddNested(new EOObject{"x"});
  add1->AddNested(rai64);
  add1->AddNested(new EOObject{"1", EOObjectType::EO_LITERAL});
  write1->AddNested(add1);
  seq1->AddNested(write1);

  EOObject* printf1 = new EOObject{"printf"};
  printf1->AddNested(new EOObject{"\"%d\\n\"", EOObjectType::EO_LITERAL});
  printf1->AddNested(rai64);
  seq1->AddNested(printf1);


  EOObject* write2 = new EOObject{"write", EOObjectType::EO_COMPLETE};
  write2->AddNested(new EOObject{"x"});
  EOObject* add2 = new EOObject{"add"};
  add2->AddNested(new EOObject{"1", EOObjectType::EO_LITERAL});
  add2->AddNested(rai64);
  write2->AddNested(add1);
  seq1->AddNested(write2);

  seq1->AddNested(printf1);

  seq1->AddNested(new EOObject{"TRUE", EOObjectType::EO_LITERAL});
  main->AddNested(seq1);
  eo->AddNested(main);

  EOObject* eo_app = new EOObject{EOObjectType::EO_ABSTRACT};
  eo_app->postfix = "eo-application";
  eo_app->arguments = {"arg"};

  EOObject* seq2 = new EOObject{"seq", "@"};
  seq2->AddNested(new EOObject{"main"});
  seq2->AddNested(new EOObject{"TRUE", EOObjectType::EO_LITERAL});
  eo_app->AddNested(seq2);
  eo->AddNested(eo_app);

  EOObject* seq3 = new EOObject{"seq", "@"};

  EOObject* write3 = new EOObject{"write"};
  write3->AddNested(new EOObject{"x"});
  write3->AddNested(new EOObject{"1", EOObjectType::EO_LITERAL});
  seq3->AddNested(write3);

  EOObject* write4 = new EOObject{"write"};
  write4->AddNested(new EOObject{"empty-global-position"});
  write4->AddNested(new EOObject{"8", EOObjectType::EO_LITERAL});
  seq3->AddNested(write4);

  EOObject* eo_app_st = new EOObject{"eo-application"};
  eo_app_st->AddNested(new EOObject{"args"});
  seq3->AddNested(eo_app_st);
  seq3->AddNested(new EOObject{"TRUE", EOObjectType::EO_LITERAL});
  eo->AddNested(seq3);

  return eo;
}
