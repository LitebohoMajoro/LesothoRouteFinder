#pragma once
#include "HashSetOpen.h"
#include "HashSetClosed.h"
#include <string>

namespace HashTableFileExporter {
bool writeOpen(const HashSetOpen& table, const std::string& path, std::string* error=nullptr);
bool writeClosed(const HashSetClosed& table, const std::string& path, std::string* error=nullptr);
}
