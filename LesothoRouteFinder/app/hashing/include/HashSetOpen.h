#pragma once
#include "LinkedList.h"
#include <vector>
class HashSetOpen { std::vector<LinkedList> buckets_; int size_=0; int hash(int)const;
public: explicit HashSetOpen(int capacity=11); bool add(int);bool contains(int)const;bool remove(int);int size()const{return size_;}int capacity()const{return (int)buckets_.size();}double loadFactor()const;const std::vector<LinkedList>& buckets()const{return buckets_;} };
