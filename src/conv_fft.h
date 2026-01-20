#pragma once

#include "image.h"
#include "kernels.h"

// FFT-based convolution (O(N log N)) producing same-size output as input.
// Uses zero-padding outside the image, consistent with convolve_naive().
Image convolve_fft(const Image& img, const Kernel& k);
