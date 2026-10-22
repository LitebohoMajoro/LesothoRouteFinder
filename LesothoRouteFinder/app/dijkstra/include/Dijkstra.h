
#pragma once

#include "Graph.h"
#include <climits>
#include <map>
#include <string>
#include <vector>

struct ShortestPaths {
    std::map<char, int> distances;
    std::map<char, char> predecessors;
    std::vector<std::string> steps;
    bool valid = true;
};

// Calculate shortest paths from the source.
ShortestPaths dijkstra(const Graph& graph, char source);

// Reconstruct a shortest route.
std::vector<char> reconstructPath(
    char source,
    char target,
    const ShortestPaths& result
);

// Find all equally short routes.
std::vector<std::vector<char>> allShortestPaths(
    const Graph& graph,
    char source,
    char target,
    const ShortestPaths& result
);
