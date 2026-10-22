#include "HashTableFileExporter.h"
#include <filesystem>
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace {
std::string border(std::size_t width) { return "+-------+-" + std::string(width, '-') + "+\n"; }
void fail(std::string* error, const std::string& path) { if(error) *error="Could not update "+path+"."; }
template<class Row> bool write(const std::string& path, const std::string& title, const std::string& method, int capacity, int size, double load, Row row, bool closed, std::string* error) {
 try {
  auto parent=std::filesystem::path(path).parent_path(); if(!parent.empty()) std::filesystem::create_directories(parent);
  std::size_t width=24; for(int i=0;i<capacity;++i) width=std::max(width,row(i).size());
  std::ofstream f(path,std::ios::binary|std::ios::trunc); if(!f){fail(error,path);return false;}
  f<<title<<"\r\n"<<std::string(title.size(),'=')<<"\r\nCapacity: "<<capacity<<"\r\nCollision Method: "<<method<<"\r\n\r\n";
  f<<border(width)<<"| Index | "<<std::left<<std::setw((int)width)<<"Data"<<" |\r\n"<<border(width);
  for(int i=0;i<capacity;++i) f<<"| "<<std::left<<std::setw(5)<<i<<" | "<<std::left<<std::setw((int)width)<<row(i)<<" |\r\n";
  f<<border(width)<<"\r\nTotal Values: "<<size<<"\r\n"; if(closed) f<<"Load Factor: "<<std::fixed<<std::setprecision(2)<<load<<"\r\n";
  if(!f){fail(error,path);return false;} return true;
 } catch(...) { fail(error,path); return false; }
}
}
namespace HashTableFileExporter {
bool writeOpen(const HashSetOpen& t,const std::string& p,std::string* e){return write(p,"OPEN HASHING TABLE","Separate Chaining",t.capacity(),t.size(),t.loadFactor(),[&](int i){std::string s;for(int v:t.buckets()[i].values()){if(!s.empty())s+=" -> ";s+=std::to_string(v);}return s.empty()?std::string("EMPTY"):s;},false,e);}
bool writeClosed(const HashSetClosed& t,const std::string& p,std::string* e){return write(p,"CLOSED HASHING TABLE","Linear Probing",t.capacity(),t.size(),t.loadFactor(),[&](int i){const auto&s=t.slots()[i];return s.state==SlotState::Occupied?std::to_string(s.value):std::string("EMPTY");},true,e);}
}
