
#include "Tests.h"
#include "Dijkstra.h"

#include <climits>
#include <iostream>
#include <map>
#include <vector>

void runTests() {
    std::cout << "\nALGORITHM TESTS\n";

    int passed = 0;
    int failed = 0;

    auto check = [&](bool condition, const char* description) {
        std::cout << (condition ? "[PASS] " : "[FAIL] ")
                  << description << '\n';

        if (condition) {
            ++passed;
        } else {
            ++failed;
        }
    };

    Graph graph = createAssignmentGraph();
    auto result = dijkstra(graph, 'A');
    check(result.valid && graph.towns().size() == 10, "10 districts loaded");
    const std::map<char, int> expected = {
        {'A',0},{'B',35},{'C',100},{'D',155},{'E',210},
        {'F',95},{'G',225},{'H',235},{'I',145},{'J',70}
    };
    for (const auto& [id, distance] : expected) {
        check(result.distances.at(id) == distance,
              ("Shortest distance to " + graph.nameOf(id)).c_str());
    }
    check(reconstructPath('A', 'H', result) ==
          std::vector<char>({'A','J','I','H'}),
          "Route reconstruction to Quthing");
    Graph invalid = createAssignmentGraph();
    invalid.addRoad('B','C',-2);
    auto bad = dijkstra(invalid,'A');
    check(!bad.valid && bad.distances.empty() && bad.predecessors.empty(),
          "Reject negative edges");
    Graph isolated = createAssignmentGraph();
    isolated.addTown('K', "Isolated");
    auto disconnected = dijkstra(isolated, 'A');
    check(disconnected.distances.at('K') == INT_MAX &&
          reconstructPath('A','K',disconnected).empty(),
          "Disconnected district is unreachable");
    Graph single;
    single.addTown('A', "Maseru");
    check(dijkstra(single,'A').distances.at('A') == 0,
          "Single-town graph");

    std::cout << "\nTests Passed: " << passed;
    std::cout << "\nTests Failed: " << failed;
}
