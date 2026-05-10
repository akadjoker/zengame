#pragma once

// ============================================================
// pch.hpp - Precompiled Header
//
// Include all stable, rarely-changing headers here.
// This file is compiled once and reused across all translation units.
//
// Rules:
//   - Only include headers that change infrequently
//   - Never include project-specific headers here
//   - Keep grouped by category
// ============================================================

// ----------------------------------------------------------
// C Standard Library
// ----------------------------------------------------------
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>

// ----------------------------------------------------------
// C++ Standard Library - Containers
// ----------------------------------------------------------
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <array>

// ----------------------------------------------------------
// C++ Standard Library - Utilities
// ----------------------------------------------------------
#include <algorithm>
#include <functional>
#include <memory>
#include <utility>
#include <limits>

// ----------------------------------------------------------
// C++ Standard Library - Streams (debug / logging only)
// ----------------------------------------------------------
#include <iostream>
#include <sstream>

#include <raylib.h>