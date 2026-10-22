
#include "Graph.h"
#include <stdexcept>

void Graph::addTown(char id, const std::string& name) {
    names_[id] = name;
    adjacency_[id];
}

void Graph::addRoad(char from, char to, int distance) {
    if (!contains(from) || !contains(to)) {
        throw std::invalid_argument(
            "Both towns must exist before adding a road."
        );
    }

    adjacency_[from].push_back({to, distance});
}

const std::map<char, std::string>& Graph::towns() const {
    return names_;
}

const std::vector<Edge>& Graph::roadsFrom(char town) const {
    return adjacency_.at(town);
}

bool Graph::contains(char town) const {
    return names_.count(town) != 0;
}

std::string Graph::nameOf(char town) const {
    return names_.at(town);
}

bool Graph::hasNegativeRoad() const {
    for (const auto& [town, roads] : adjacency_) {
        for (const auto& road : roads) {
            if (road.weight < 0) {
                return true;
            }
        }
    }
    return false;
}

Graph createAssignmentGraph() {
    Graph graph;
    // Ten administrative districts. Weights are invented examples,
    // NOT real driving distances between district headquarters.
    graph.addTown('A', "Maseru");
    graph.addTown('B', "Berea");
    graph.addTown('C', "Leribe");
    graph.addTown('D', "Butha-Buthe");
    graph.addTown('E', "Mokhotlong");
    graph.addTown('F', "Thaba-Tseka");
    graph.addTown('G', "Qacha's Nek");
    graph.addTown('H', "Quthing");
    graph.addTown('I', "Mohale's Hoek");
    graph.addTown('J', "Mafeteng");

    graph.addRoad('A', 'B', 35);
    graph.addRoad('A', 'J', 70);
    graph.addRoad('A', 'F', 95);
    graph.addRoad('B', 'C', 65);
    graph.addRoad('B', 'F', 110);
    graph.addRoad('C', 'D', 55);
    graph.addRoad('C', 'E', 125);
    graph.addRoad('D', 'E', 105);
    graph.addRoad('F', 'E', 115);
    graph.addRoad('F', 'G', 130);
    graph.addRoad('F', 'I', 100);
    graph.addRoad('J', 'I', 75);
    graph.addRoad('I', 'H', 90);
    graph.addRoad('G', 'H', 110);
    graph.addRoad('I', 'G', 190);
    graph.addRoad('J', 'H', 210);
    return graph;
}
