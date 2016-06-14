#include <string>
#include <algorithm>

#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <err.h>

using namespace std;

string get_absolute(string name)
{
  char* absolute_path = canonicalize_file_name(name.c_str());
  if(!absolute_path)
    err(errno, "get_absolute failed");
  return absolute_path;
}

string omit_case(string name)
{
  transform(name.begin(), name.end(), name.begin(), ::tolower);
  return name;
}