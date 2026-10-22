#include "LinkedList.h"
#include <utility>
void LinkedList::clear() noexcept { while(head_){Node* n=head_;head_=head_->next;delete n;} size_=0; }
LinkedList::~LinkedList(){clear();}
LinkedList::LinkedList(const LinkedList& o){
    Node** tail=&head_;
    try {
        for(Node* n=o.head_;n;n=n->next){*tail=new Node(n->value);tail=&((*tail)->next);++size_;}
    } catch (...) {
        clear();
        throw;
    }
}
LinkedList& LinkedList::operator=(const LinkedList& o){if(this!=&o){LinkedList c(o);std::swap(head_,c.head_);std::swap(size_,c.size_);}return *this;}
LinkedList::LinkedList(LinkedList&& o) noexcept:head_(o.head_),size_(o.size_){o.head_=nullptr;o.size_=0;}
LinkedList& LinkedList::operator=(LinkedList&& o) noexcept {if(this!=&o){clear();head_=o.head_;size_=o.size_;o.head_=nullptr;o.size_=0;}return *this;}
bool LinkedList::add(int v){if(contains(v))return false;Node*n=new Node(v);n->next=head_;head_=n;++size_;return true;}
bool LinkedList::contains(int v)const{for(Node*n=head_;n;n=n->next)if(n->value==v)return true;return false;}
bool LinkedList::remove(int v){Node** p=&head_;while(*p&&(*p)->value!=v)p=&((*p)->next);if(!*p)return false;Node*n=*p;*p=n->next;delete n;--size_;return true;}
std::vector<int> LinkedList::values()const{std::vector<int> r;for(Node*n=head_;n;n=n->next)r.push_back(n->value);return r;}
