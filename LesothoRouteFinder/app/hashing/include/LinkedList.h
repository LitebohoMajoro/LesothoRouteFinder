#pragma once
#include <vector>
class LinkedList {
    struct Node { int value; Node* next; explicit Node(int v):value(v),next(nullptr){} };
    Node* head_ = nullptr; int size_ = 0;
    void clear() noexcept;
public:
    LinkedList() = default; ~LinkedList();
    LinkedList(const LinkedList&); LinkedList& operator=(const LinkedList&);
    LinkedList(LinkedList&&) noexcept; LinkedList& operator=(LinkedList&&) noexcept;
    bool add(int); bool contains(int) const; bool remove(int);
    int size() const noexcept { return size_; }
    std::vector<int> values() const;
};
