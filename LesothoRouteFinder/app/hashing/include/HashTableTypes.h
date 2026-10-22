#pragma once
#include <string>
#include <vector>
enum class Mode { Open, Closed };
enum class SlotState { Empty, Occupied, Deleted };
struct Slot { int value = 0; SlotState state = SlotState::Empty; };
struct Event { std::string text; int index = -1; bool warning = false; };
