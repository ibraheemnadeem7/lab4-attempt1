#pragma once

#include <string>
#include <vector>

struct Kernel {
  int size = 0;                 // odd, e.g. 3, 31, 51
  std::vector<double> w;        // row-major, size*size

  double at(int r, int c) const {
    return w[static_cast<size_t>(r) * size + c];
  }
};

// Create a kernel by name.
// name: blur|sharpen|edge|emboss
// size: must be odd. For blur, size can be > 3 to create a box blur kernel.
// For other filters, only size==3 is supported.
Kernel make_kernel(const std::string& name, int size);
