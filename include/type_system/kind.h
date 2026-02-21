#pragma once
#include <cstddef>

class Kind {
public:
    bool constant = false;
    bool global   = false;
    bool literal  = false;
    size_t  pointer_count = 0;
    size_t  array_count   = 0;
    // TODO: add a vector of array data
};
