
#include "Dijkstra.h"

#include <algorithm>
#include <functional>
#include <queue>
#include <set>
#include <utility>

ShortestPaths dijkstra(const Graph& graph, char source) {
    ShortestPaths result;

    // Reject invalid graphs and negative distances.
    if (!graph.contains(source) || graph.hasNegativeRoad()) {
        result.valid = false;
        return result;
    }

    // Initialize all distances to infinity.
    for (const auto& [id, name] : graph.towns()) {
        result.distances[id] = INT_MAX;
    }

    result.distances[source] = 0;

    result.steps.push_back(
        "Initialize: source " + graph.nameOf(source) +
        " = 0 km; other towns = INF."
    );

    // Min-heap: {distance, town}
    using Entry = std::pair<int, char>;

    std::priority_queue<
        Entry,
        std::vector<Entry>,
        std::greater<Entry>
    > minHeap;

    minHeap.push({0, source});

    std::set<char> visited;

    while (!minHeap.empty()) {
        const auto [distance, town] = minHeap.top();
        minHeap.pop();

        // Avoid reprocessing visited towns.
        if (visited.count(town)) {
            continue;
        }

        visited.insert(town);

        result.steps.push_back(
            "Visit " + graph.nameOf(town) +
            " (" + std::to_string(distance) + " km)."
        );

        // Check neighboring towns.
        for (const Edge& edge : graph.roadsFrom(town)) {
            if (visited.count(edge.to)) {
                continue;
            }

            // Prevent integer overflow.
            if (distance > INT_MAX - edge.weight) {
                continue;
            }

            int candidate = distance + edge.weight;

            // Relax the edge if we found a shorter path.
            if (candidate < result.distances[edge.to]) {
                result.distances[edge.to] = candidate;
                result.predecessors[edge.to] = town;

                minHeap.push({candidate, edge.to});

                result.steps.push_back(
                    "  Update " + graph.nameOf(edge.to) +
                    " via " + graph.nameOf(town) +
                    " -> " + std::to_string(candidate) +
                    " km."
                );
            }
            else if (candidate == result.distances[edge.to]) {
                result.steps.push_back(
                    "  Equal-cost route to " +
                    graph.nameOf(edge.to) +
                    " via " + graph.nameOf(town) + "."
                );
            }
        }
    }

    // Report unreachable towns.
    for (const auto& [id, distance] : result.distances) {
        if (distance == INT_MAX) {
            result.steps.push_back(
                graph.nameOf(id) + " is unreachable."
            );
        }
    }

    return result;
}

// Reconstruct a path from the predecessor map.
std::vector<char> reconstructPath(
    char source,
    char target,
    const ShortestPaths& result
) {
    auto it = result.distances.find(target);

    if (!result.valid ||
        it == result.distances.end() ||
        it->second == INT_MAX) {
        return {};
    }

    std::vector<char> path;
    std::set<char> seen;

    char current = target;

    while (true) {
        if (!seen.insert(current).second) {
            return {};
        }

        path.push_back(current);

        if (current == source) {
            break;
        }

        auto previous = result.predecessors.find(current);

        if (previous == result.predecessors.end()) {
            return {};
        }

        current = previous->second;
    }

    std::reverse(path.begin(), path.end());
    return path;
}

// Find all routes that preserve shortest distances.
static void enumeratePaths(
    const Graph& graph,
    char current,
    char target,
    const ShortestPaths& result,
    std::vector<char>& path,
    std::set<char>& seen,
    std::vector<std::vector<char>>& answer
) {
    if (current == target) {
        answer.push_back(path);
        return;
    }

    for (const Edge& road : graph.roadsFrom(current)) {
        if (seen.count(road.to)) {
            continue;
        }

        auto from = result.distances.find(current);
        auto to = result.distances.find(road.to);

        if (from == result.distances.end() ||
            to == result.distances.end() ||
            from->second == INT_MAX ||
            to->second == INT_MAX) {
            continue;
        }

        if (from->second > INT_MAX - road.weight ||
            from->second + road.weight != to->second) {
            continue;
        }

        seen.insert(road.to);
        path.push_back(road.to);

        enumeratePaths(
            graph, road.to, target, result,
            path, seen, answer
        );

        path.pop_back();
        seen.erase(road.to);
    }
}

std::vector<std::vector<char>> allShortestPaths(
    const Graph& graph,
    char source,
    char target,
    const ShortestPaths& result
) {
    if (!result.valid ||
        !graph.contains(source) ||
        !graph.contains(target) ||
        result.distances.count(target) == 0 ||
        result.distances.at(target) == INT_MAX) {
        return {};
    }

    std::vector<std::vector<char>> paths;

    std::vector<char> working{source};
    std::set<char> seen{source};

    enumeratePaths(
        graph, source, target, result,
        working, seen, paths
    );

    return paths;
}
