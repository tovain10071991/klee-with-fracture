#include "Helper/common.h"

#include "llvm/Object/ObjectFile.h"
#include "llvm/Object/ELFObjectFile.h"

#include <map>
#include <string>

using namespace std;
using namespace llvm;

namespace saib {
namespace objcet {

static std::map<std::stirng, llvm::object::ObjectFile*> objects;

static object::ObjectFile* get_object(string obj_name)
{
  if(objects.find(get_absolute(obj_name))==objects.end())
  {
    object::ObjectFile* obj = object::ObjectFile::createObjectFile(obj_name);
    assert(obj->isObject() && obj->isELF() && "it is not elf object");
    objects[get_absolute(obj_name)] = obj;
  }
  return objects[get_absolute(obj_name)];
}

addr_t get_entry(string obj_name)
{
  auto elf_obj = dyn_cast<object::ELF64LEObjectFile>(get_object(obj_name))->getELFFile();
  assert(elf_obj);
  auto ehdr = elf_obj->getHeader();
  return ehdr->e_entry;
}

} // namespace objcet
} // namespace saib