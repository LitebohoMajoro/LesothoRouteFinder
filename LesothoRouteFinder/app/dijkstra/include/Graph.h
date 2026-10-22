
#pragma once

#include <map>
#include <string>
#include <vector>

struct Edge {
    char to;
    int weight;
};

class Graph {
public:
    void addTown(char id, const std::string& name);
    void addRoad(char from, char to, int distance);

    const std::map<char, std::string>& towns() const;
    const std::vector<Edge>& roadsFrom(char town) const;

    bool contains(char town) const;
    std::string nameOf(char town) const;
    bool hasNegativeRoad() const;

private:
    std::map<char, std::string> names_;
    std::map<char, std::vector<Edge>> adjacency_;
};

Graph createAssignmentGraph();
