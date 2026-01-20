#include "kernels.h"

#include <stdexcept>

static Kernel make3(const std::vector<double>& vals) {
  if (vals.size() != 9) {
    throw std::runtime_error("Internal error: 3x3 kernel must have 9 values");
  }
  Kernel k;
  k.size = 3;
  k.w = vals;
  return k;
}

static Kernel box_blur(int size) {
  if (size <= 0 || (size % 2) == 0) {
    throw std::runtime_error("box_blur: size must be positive odd");
  }
  Kernel k;
  k.size = size;
  k.w.assign(static_cast<size_t>(size) * size, 0.0);

  const double val = 1.0 / static_cast<double>(size * size);
  for (double& x : k.w) x = val;

  return k;
}

Kernel make_kernel(const std::string& name, int size) {
  if (size <= 0 || (size % 2) == 0) {
    throw std::runtime_error("Kernel size must be a positive odd integer.");
  }

  if (name == "blur") {
    // 3x3 blur is the classic "box blur".
    // For larger sizes, we generate a size x size box blur to make FFT meaningful.
    if (size == 3) {
      return make3({
        1.0/9, 1.0/9, 1.0/9,
        1.0/9, 1.0/9, 1.0/9,
        1.0/9, 1.0/9, 1.0/9
      });
    }
    return box_blur(size);
  }

  // For the remaining filters, we keep it simple: fixed 3x3 kernels only.
  if (size != 3) {
    throw std::runtime_error("Only blur supports --kernel-size != 3 in this demo.");
  }

  if (name == "sharpen") {
    // Common sharpen kernel
    return make3({
       0, -1,  0,
      -1,  5, -1,
       0, -1,  0
    });
  }

  if (name == "edge") {
    // Laplacian edge detector (8-neighbor)
    return make3({
      -1, -1, -1,
      -1,  8, -1,
      -1, -1, -1
    });
  }

  if (name == "emboss") {
    // Emboss kernel (directional)
    return make3({
      -2, -1,  0,
      -1,  1,  1,
       0,  1,  2
    });
  }

  throw std::runtime_error("Unknown filter: " + name + " (expected blur|sharpen|edge|emboss)");
}
