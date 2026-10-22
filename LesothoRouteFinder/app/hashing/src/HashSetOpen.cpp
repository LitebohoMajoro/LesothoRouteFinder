#include "HashSetOpen.h"
HashSetOpen::HashSetOpen(int c):buckets_(c>0?c:1){}
int HashSetOpen::hash(int v)const{int i=v%capacity();return i<0?i+capacity():i;}
bool HashSetOpen::add(int v){if(!buckets_[hash(v)].add(v))return false;++size_;return true;}
bool HashSetOpen::contains(int v)const{return buckets_[hash(v)].contains(v);}
bool HashSetOpen::remove(int v){if(!buckets_[hash(v)].remove(v))return false;--size_;return true;}
double HashSetOpen::loadFactor()const{return (double)size_/capacity();}
