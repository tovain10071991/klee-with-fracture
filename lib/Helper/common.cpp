#include "Helper/common.h"

#include <string>

#include <unistd.h>

string get_absolute(string name)
{
  if(name[0] == '/')
    return name;
  assert(get_current_dir_name());
  return string(get_current_dir_name()) + "/" + name;
}