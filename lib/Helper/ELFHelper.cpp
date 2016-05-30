#include "Helper/ELFHelper.h"
#include "Helper/LLDBHelper.h"

#include <gelf.h>

#include <map>
#include <string>

#include <err.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

using namespace std;

static map<string, Elf*> elfs;

class ELFInited {
public:
  ELFInited()
  {
    if(elf_version(EV_CURRENT)==EV_NONE)
      errx(elf_errno(), "elf_version: %s\n", elf_errmsg(elf_errno()));
  }
  ~ELFInited()
  {
    for(auto elf_iter = elfs.begin(); elf_iter != elfs.end(); ++elf_iter)
      elf_end(elf_iter->second);
  }
};
static ELFInited inited;

unsigned long get_got_plt_addr(string obj_name)
{
  if(elfs.find(obj_name)==elfs.end())
  {
    int fd = open(obj_name.c_str(), O_RDONLY);
    if(fd == -1)
      err(errno, "open in init_module");
    if((elfs[obj_name]=elf_begin(fd, ELF_C_READ, NULL))==NULL)
      errx(elf_errno(), "%s elf_begin: %s\n", obj_name.c_str(), elf_errmsg(elf_errno()));
  }
  Elf* elf = elfs[obj_name];

  //从section查找.plt基址和大小和.got.plt表基址
  Elf_Scn* scn = NULL;
  GElf_Shdr shdr;
  while((scn=elf_nextscn(elf, scn))!=NULL)
  {
  	if(gelf_getshdr(scn, &shdr)==NULL)
  		errx(elf_errno(), "%s gelf_getshdr: %s\n", obj_name.c_str(), elf_errmsg(elf_errno()));
  	if(shdr.sh_type==SHT_DYNAMIC)
  	{
  		Elf_Data* data = elf_getdata(scn, NULL);
  		if(data==NULL)
  			errx(elf_errno(), "%s elf_getdata: %s\n", obj_name.c_str(), elf_errmsg(elf_errno()));
  		Elf64_Dyn dyn;
  		for(unsigned long i=0;i<data->d_size;i+=sizeof(Elf64_Dyn))
  		{
  			memcpy(&dyn, (void*)((unsigned long)data->d_buf+i), sizeof(Elf64_Dyn));
  			if(dyn.d_tag==DT_PLTGOT)
  			{
  				return dyn.d_un.d_ptr;
  			}
  		}
  	}
  }
  errx(-1, "can't find .got.plt");
}
