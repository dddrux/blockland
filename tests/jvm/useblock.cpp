#include "jvmrt.h"
int main(){ std::vector<std::string> lg; rt::loadMods(lg); for(auto&s:lg) printf("%s\n",s.c_str());
  for(auto o:rt::host().registered) printf("reg %s %s\n", o->ci->name.c_str(), rt::regNameOf(o).c_str());
  static std::map<int,rt::Obj*> tl; rt::host().getTile=[](int x,int,int)->rt::Obj*{return tl.count(x)?tl[x]:nullptr;}; rt::host().setTile=[](int x,int,int,rt::Obj*t){tl[x]=t;};
  int inv=0; rt::host().openInventory=[&inv](rt::Obj*,rt::Obj*){inv++;};
  for(const char*k:{"ironfurnaces_diamond_furnace_idle","ironfurnaces_iron_furnace_active","x_y"}){ std::string e; int r=rt::useBlock(k,7,64,7,&e); printf("use %s -> %d inv=%d %s\n",k,r,inv,e.c_str()); }
  for(auto&m:rt::missing()) printf("sem shim: %s\n",m.c_str()); return 0; }
