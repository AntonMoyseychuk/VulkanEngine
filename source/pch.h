#pragma once

#include <vector>
#include <array>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <list>
#include <forward_list>
#include <bitset>
#include <string>

#include <algorithm>
#include <utility>
#include <memory>
#include <functional>
#include <optional>

#include <filesystem>
#include <fstream>
#include <sstream>

#include <type_traits>

#include <random>

#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cstring>

#include <nlohmann/json.hpp>

#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include "core.h"

#if defined(AM_OS_WINDOWS)
    #define WIN32_LEAN_AND_MEAN
    #define NOMINMAX
    #include <Windows.h>
#endif