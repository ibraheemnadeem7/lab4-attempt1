#include "fft.h"

#include <cmath>
#include <stdexcept>
#include <utility>

static bool is_power_of_two(int n) {
  return n > 0 && (n & (n - 1)) == 0;
}

int next_pow2(int n) {
  if (n <= 0) throw std::runtime_error("next_pow2: n must be >= 1");
  int p = 1;
  while (p < n) p <<= 1;
  return p;
}

void fft1d_inplace(std::vector<std::complex<double>>& a, bool invert) {
  const int n = static_cast<int>(a.size());
  if (n == 0) return;
  if (!is_power_of_two(n)) {
    throw std::runtime_error("fft1d_inplace: size must be a power of two");
  }

  // Bit-reversal permutation
  for (int i = 1, j = 0; i < n; ++i) {
    int bit = n >> 1;
    for (; j & bit; bit >>= 1) j ^= bit;
    j ^= bit;
    if (i < j) std::swap(a[i], a[j]);
  }

  // Iterative Danielson–Lanczos portion
  for (int len = 2; len <= n; len <<= 1) {
    const double ang = 2.0 * M_PI / static_cast<double>(len) * (invert ? -1.0 : 1.0);
    std::complex<double> wlen(std::cos(ang), std::sin(ang));

    for (int i = 0; i < n; i += len) {
      std::complex<double> w(1.0, 0.0);
      const int half = len >> 1;

      for (int j = 0; j < half; ++j) {
        std::complex<double> u = a[i + j];
        std::complex<double> v = a[i + j + half] * w;
        a[i + j] = u + v;
        a[i + j + half] = u - v;
        w *= wlen;
      }
    }
  }

  // Normalize inverse transform
  if (invert) {
    for (auto& x : a) x /= static_cast<double>(n);
  }
}

void fft2d_inplace(std::vector<std::complex<double>>& data, int width, int height, bool invert) {
  if (width <= 0 || height <= 0) {
    throw std::runtime_error("fft2d_inplace: invalid dimensions");
  }
  if (!is_power_of_two(width) || !is_power_of_two(height)) {
    throw std::runtime_error("fft2d_inplace: width and height must be powers of two");
  }
  if (static_cast<int>(data.size()) != width * height) {
    throw std::runtime_error("fft2d_inplace: data size mismatch");
  }

  // FFT rows
  std::vector<std::complex<double>> row(width);
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      row[x] = data[static_cast<size_t>(y) * width + x];
    }
    fft1d_inplace(row, invert);
    for (int x = 0; x < width; ++x) {
      data[static_cast<size_t>(y) * width + x] = row[x];
    }
  }

  // FFT columns
  std::vector<std::complex<double>> col(height);
  for (int x = 0; x < width; ++x) {
    for (int y = 0; y < height; ++y) {
      col[y] = data[static_cast<size_t>(y) * width + x];
    }
    fft1d_inplace(col, invert);
    for (int y = 0; y < height; ++y) {
      data[static_cast<size_t>(y) * width + x] = col[y];
    }
  }

  // Note: Because fft1d_inplace() normalizes by 1/N on inverse,
  // doing inverse on rows and then columns results in overall scaling of
  // 1/(width*height) automatically. No extra scaling needed here.
}
