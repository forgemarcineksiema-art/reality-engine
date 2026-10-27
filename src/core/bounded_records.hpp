#pragma once
#include "core/entity_registry.hpp"
#include <algorithm>
#include <stdexcept>
namespace reality {
template <typename T> void prepare_records(std::vector<T>& records, std::size_t extra) {
    if (records.size() > record_limit || extra > record_limit - records.size())
        throw std::length_error("prototype record limit reached");
    if (records.size() + extra > records.capacity())
        records.reserve(
            std::max(records.size() + extra,
                     std::min(record_limit, std::max(std::size_t{8}, records.capacity() * 2))));
}
} // namespace reality
