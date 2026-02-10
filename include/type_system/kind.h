#pragma once
#include <cstddef>
#include <cstdint>

struct Kind {
    bool constant = false;
    bool global   = false;
    bool literal  = false;
    size_t  pointer_count = 0;
    int64_t deref_offset  = -1;
    size_t  array_count   = 0;
    // TODO: add a vector of array data
};
