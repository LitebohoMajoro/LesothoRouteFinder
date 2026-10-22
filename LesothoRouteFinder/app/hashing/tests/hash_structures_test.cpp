#include "LinkedList.h"
#include "HashSetOpen.h"
#include "HashSetClosed.h"
#include "HashTableFileExporter.h"
#include <cassert>
#include <vector>
#include <fstream>
#include <string>

int main() {
    LinkedList list;
    assert(list.size()==0 && !list.contains(4) && !list.remove(4));
    assert(list.add(4) && list.add(5) && list.add(6));
    assert(!list.add(5) && list.size()==3);
    assert(list.remove(6) && list.remove(5) && list.remove(4));
    assert(list.size()==0);
    list.add(1); list.add(2); list.add(3);
    LinkedList copy=list;
    copy.remove(2);
    assert(list.contains(2) && !copy.contains(2));
    copy=copy;
    assert(copy.size()==2);
    LinkedList assigned; assigned=list; assigned.remove(1);
    assert(list.contains(1) && !assigned.contains(1));

    HashSetOpen open(7);
    assert(open.capacity()==7);
    assert(open.add(10) && open.add(21) && open.add(-1));
    assert(open.loadFactor()>0 && open.contains(10) && open.contains(21));
    assert(!open.add(21) && open.remove(10) && !open.contains(10));
    assert(open.add(28)); // shares bucket 0 with 21 at capacity 7
    assert(open.size()==3);

    HashSetClosed closed(11);
    std::vector<Event> events;
    assert(closed.add(10,&events) && closed.add(21,&events));
    assert(closed.slots()[10].value==10 && closed.slots()[0].value==21);
    assert(!closed.add(21) && closed.contains(21));
    assert(closed.remove(10) && closed.contains(21)); // search must pass a tombstone
    assert(closed.add(32)); // reuses a deleted slot in its probe sequence
    for(int value: {1,2,3,4,5,6}) assert(closed.add(value)); // forces growth
    assert(closed.capacity()>11 && closed.size()==8);
    for(int value: {21,32,1,2,3,4,5,6}) assert(closed.contains(value));
    assert(closed.loadFactor()<=0.5);

    std::string error;
    assert(HashTableFileExporter::writeOpen(open,"tests/open_export.txt",&error));
    assert(HashTableFileExporter::writeClosed(closed,"tests/closed_export.txt",&error));
    std::ifstream of("tests/open_export.txt");std::string ot((std::istreambuf_iterator<char>(of)),{});
    std::ifstream cf("tests/closed_export.txt");std::string ct((std::istreambuf_iterator<char>(cf)),{});
    assert(ot.find("Capacity: 7")!=std::string::npos && ot.find("Total Values: 3")!=std::string::npos);
    assert(ot.find("EMPTY")!=std::string::npos && ot.find(" -> ")!=std::string::npos);
    assert(ct.find("Capacity: "+std::to_string(closed.capacity()))!=std::string::npos);
    assert(ct.find("Load Factor: ")!=std::string::npos && ct.find("Total Values: 8")!=std::string::npos);
}
