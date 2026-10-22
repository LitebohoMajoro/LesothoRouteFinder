#pragma once
#include "HashTableTypes.h"
#include <vector>
class HashSetClosed {std::vector<Slot> slots_;int size_=0;int hash(int)const;void rehash(int);bool insertRaw(int);
public:explicit HashSetClosed(int capacity=11);bool add(int,std::vector<Event>* events=nullptr);bool contains(int,std::vector<Event>* events=nullptr)const;bool remove(int,std::vector<Event>* events=nullptr);int size()const{return size_;}int capacity()const{return (int)slots_.size();}double loadFactor()const{return (double)size_/capacity();}const std::vector<Slot>& slots()const{return slots_;}};
