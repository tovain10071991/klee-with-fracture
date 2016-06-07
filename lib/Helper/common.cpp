#include <string>
#include <algorithm>

#include <stdlib.h>
#include <unistd.h>

using namespace std;

string get_absolute(string name)
{
  return canonicalize_file_name(name.c_str());
}

string omit_case(string name)
{
  transform(name.begin(), name.end(), name.begin(), ::tolower);
  return name;
}