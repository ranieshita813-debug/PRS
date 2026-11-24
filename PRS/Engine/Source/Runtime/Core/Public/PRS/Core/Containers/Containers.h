#pragma once
// Thin aliases over the standard containers so engine code has one vocabulary.
// They are aliases on purpose: if a container later needs a custom allocator or
// intrusive node, only this header and its users change.
#include "PRS/Core/Types.h"
#include <array>
#include <deque>
#include <map>
#include <optional>
#include <set>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace PRS
{
template <typename T> using Array = std::vector<T>;
template <typename T, size_t N> using FixedArray = std::array<T, N>;
template <typename T> using Span = std::span<T>;
template <typename T> using ConstSpan = std::span<const T>;
template <typename T> using Optional = std::optional<T>;
template <typename T> using Queue = std::deque<T>;
template <typename K, typename V> using HashMap = std::unordered_map<K, V>;
template <typename K> using HashSet = std::unordered_set<K>;
template <typename K, typename V> using OrderedMap = std::map<K, V>;
template <typename K> using OrderedSet = std::set<K>;
using String = std::string;
using StringView = std::string_view;
using Pair = std::pair<u32, u32>;
} // namespace PRS
