#include "Helper/common.h"

#include "llvm/Object/ObjectFile.h"
#include "llvm/Object/ELFObjectFile.h"

#include <string>

using namespace llvm;
using namespace std;

static object::ObjectFile* main_obj;

addr_t get_entry(string binary)
{
  main_obj = object::ObjectFile::createObjectFile(binary);
  assert(main_obj->isObject() && main_obj->isELF() && "it is not object");
  auto elf_file = dyn_cast<object::ELF64LEObjectFile>(main_obj)->getELFFile();
  auto ehdr = elf_file->getHeader();
  return ehdr->e_entry;
}

addr_t get_entry(object::ObjectFile* obj)
{
  auto elf_file = dyn_cast<object::ELF64LEObjectFile>(obj)->getELFFile();
  auto ehdr = elf_file->getHeader();
  return ehdr->e_entry;
}