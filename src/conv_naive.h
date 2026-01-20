#pragma once

#include "image.h"
#include "kernels.h"

// Direct spatial convolution (O(W * H * k^2)) using zero-padding at borders.
Image convolve_naive(const Image& img, const Kernel& k);
