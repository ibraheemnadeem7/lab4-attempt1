#include "conv_naive.h"

#include <stdexcept>

Image convolve_naive(const Image& img, const Kernel& k) {
  if (img.width <= 0 || img.height <= 0) {
    throw std::runtime_error("convolve_naive: invalid image dimensions");
  }
  if (static_cast<int>(img.pixels.size()) != img.width * img.height) {
    throw std::runtime_error("convolve_naive: pixel buffer size mismatch");
  }
  if (k.size <= 0 || (k.size % 2) == 0) {
    throw std::runtime_error("convolve_naive: kernel size must be positive odd");
  }
  if (static_cast<int>(k.w.size()) != k.size * k.size) {
    throw std::runtime_error("convolve_naive: kernel buffer size mismatch");
  }

  Image out;
  out.width = img.width;
  out.height = img.height;
  out.pixels.assign(static_cast<size_t>(out.width) * out.height, 0.0);

  const int r = k.size / 2;

  // For each output pixel (x,y), compute sum_{i,j} img(x+i, y+j) * k(i,j)
  // using zero-padding when (x+i, y+j) is outside the image.
  for (int y = 0; y < img.height; ++y) {
    for (int x = 0; x < img.width; ++x) {
      double acc = 0.0;

      for (int ky = 0; ky < k.size; ++ky) {
        int iy = y + (ky - r);
        if (iy < 0 || iy >= img.height) continue; // zero padding

        for (int kx = 0; kx < k.size; ++kx) {
          int ix = x + (kx - r);
          if (ix < 0 || ix >= img.width) continue; // zero padding

          double pixel = img.pixels[static_cast<size_t>(iy) * img.width + ix];
          double w = k.w[static_cast<size_t>(ky) * k.size + kx];
          acc += pixel * w;
        }
      }

      out.pixels[static_cast<size_t>(y) * out.width + x] = acc;
    }
  }

  return out;
}
