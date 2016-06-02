#include "Helper/MapsHelper.h"
#include "Helper/LLDBHelper.h"

#include <vector>

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

using namespace std;

#define HIGH_BYTE_POSN ((sizeof (uint64_t) - 1) * 8)

static int
is_digit_in_base (unsigned char digit, int base)
{
  if (!isalnum (digit))
    return 0;
  if (base <= 10)
    return (isdigit (digit) && digit < base + '0');
  else
    return (isdigit (digit) || tolower (digit) < base - 10 + 'a');
}

static int
digit_to_int (unsigned char c)
{
  if (isdigit (c))
    return c - '0';
  else
    return tolower (c) - 'a' + 10;
}

uint64_t
strtoulst (const char *num, const char **trailer, int base)
{
  unsigned int high_part;
  uint64_t result;
  int minus = 0;
  int i = 0;

  /* Skip leading whitespace.  */
  while (isspace (num[i]))
    i++;

  /* Handle prefixes.  */
  if (num[i] == '+')
    i++;
  else if (num[i] == '-')
    {
      minus = 1;
      i++;
    }

  if (base == 0 || base == 16)
    {
      if (num[i] == '0' && (num[i + 1] == 'x' || num[i + 1] == 'X'))
	{
	  i += 2;
	  if (base == 0)
	    base = 16;
	}
    }

  if (base == 0 && num[i] == '0')
    base = 8;

  if (base == 0)
    base = 10;

  if (base < 2 || base > 36)
    {
      errno = EINVAL;
      return 0;
    }

  result = high_part = 0;
  for (; is_digit_in_base (num[i], base); i += 1)
    {
      result = result * base + digit_to_int (num[i]);
      high_part = high_part * base + (unsigned int) (result >> HIGH_BYTE_POSN);
      result &= ((uint64_t) 1 << HIGH_BYTE_POSN) - 1;
      if (high_part > 0xff)
	{
	  errno = ERANGE;
	  result = ~ (uint64_t) 0;
	  high_part = 0;
	  minus = 0;
	  break;
	}
    }

  if (trailer != NULL)
    *trailer = &num[i];

  result = result + ((uint64_t) high_part << HIGH_BYTE_POSN);
  if (minus)
    return -result;
  else
    return result;
}

const char *
skip_spaces_const (const char *chp)
{
  if (chp == NULL)
    return NULL;
  while (*chp && isspace (*chp))
    chp++;
  return chp;
}

static void
read_mapping (const char *line,
	      uint64_t *addr, uint64_t *endaddr,
	      const char **permissions, size_t *permissions_len,
	      uint64_t *offset,
              const char **device, size_t *device_len,
	      uint64_t *inode,
	      const char **filename)
{
  const char *p = line;

  *addr = strtoulst (p, &p, 16);
  if (*p == '-')
    p++;
  *endaddr = strtoulst (p, &p, 16);

  p = skip_spaces_const (p);
  *permissions = p;
  while (*p && !isspace (*p))
    p++;
  *permissions_len = p - *permissions;

  *offset = strtoulst (p, &p, 16);

  p = skip_spaces_const (p);
  *device = p;
  while (*p && !isspace (*p))
    p++;
  *device_len = p - *device;

  *inode = strtoulst (p, &p, 10);

  p = skip_spaces_const (p);
  *filename = p;
}

void get_stack_range(uint64_t* stack_addr, uint64_t* stack_endaddr)
{
  char filename[100];
  char mappings[12000];
  char* line;
  sprintf(filename, "/proc/%d/maps", get_pid());
  
  FILE* fd = fopen(filename, "r");
  assert(fd);
  size_t nitems = fread(mappings, 150, 80, fd);
  assert(nitems<80);
  for(line = strtok(mappings, "\n"); line; line = strtok(NULL, "\n"))
  {
    uint64_t addr, endaddr, offset, inode;
    const char *permissions, *device, *filename;
    size_t permissions_len, device_len;
    
    read_mapping(line, &addr, &endaddr, &permissions, &permissions_len, &offset, &device, &device_len, &inode, &filename);
    if(!strcmp(filename, "[stack]"))
    {
      *stack_addr = addr;
      *stack_endaddr = endaddr;
      return;
    }
  }
  assert(0 && "unreachable");
}

vector<map_t> get_data_segments()
{
  vector<map_t> data_segments;
  char filename[100];
  char mappings[12000];
  char* line;
  sprintf(filename, "/proc/%d/maps", get_pid());
  
  FILE* fd = fopen(filename, "r");
  assert(fd);
  size_t nitems = fread(mappings, 150, 80, fd);
  assert(nitems<80);
  for(line = strtok(mappings, "\n"); line; line = strtok(NULL, "\n"))
  {
    uint64_t addr, endaddr, offset, inode;
    const char *permissions, *device, *filename;
    size_t permissions_len, device_len;
    
    read_mapping(line, &addr, &endaddr, &permissions, &permissions_len, &offset, &device, &device_len, &inode, &filename);
    data_segments.push_back({addr, endaddr, permissions, permissions_len, offset, device, device_len, inode, filename});
  }
  return data_segments;
}