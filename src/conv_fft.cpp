#include "conv_fft.h"

#include "fft.h"

#include <complex>
#include <stdexcept>
#include <vector>

static inline size_t idx2d(int x, int y, int w) {
  return static_cast<size_t>(y) * static_cast<size_t>(w) + static_cast<size_t>(x);
}

// Place a kernel into a padded array with its CENTER shifted to (0,0).
// This is the standard trick to make FFT-based multiplication correspond
// to a centered spatial convolution (matching the naive implementation).
static void place_centered_kernel(std::vector<std::complex<double>>& K,
                                  int Wp, int Hp,
                                  const Kernel& k) {
  const int ks = k.size;
  const int r = ks / 2;

  // K is assumed initialized to 0s, size Wp*Hp
  for (int ky = 0; ky < ks; ++ky) {
    for (int kx = 0; kx < ks; ++kx) {
      // Shift so that (r,r) maps to (0,0)
      int tx = (kx - r) % Wp;
      int ty = (ky - r) % Hp;
      if (tx < 0) tx += Wp;
      if (ty < 0) ty += Hp;

      K[idx2d(tx, ty, Wp)] += std::complex<double>(k.w[static_cast<size_t>(ky) * ks + kx], 0.0);
    }
  }
}

Image convolve_fft(const Image& img, const Kernel& k) {
  if (img.width <= 0 || img.height <= 0) {
    throw std::runtime_error("convolve_fft: invalid image dimensions");
  }
  if (static_cast<int>(img.pixels.size()) != img.width * img.height) {
    throw std::runtime_error("convolve_fft: pixel buffer size mismatch");
  }
  if (k.size <= 0 || (k.size % 2) == 0) {
    throw std::runtime_error("convolve_fft: kernel size must be positive odd");
  }
  if (static_cast<int>(k.w.size()) != k.size * k.size) {
    throw std::runtime_error("convolve_fft: kernel buffer size mismatch");
  }

  const int W = img.width;
  const int H = img.height;
  const int ks = k.size;

  // For linear convolution (no wrap-around), pad to at least (W+ks-1, H+ks-1)
  const int needW = W + ks - 1;
  const int needH = H + ks - 1;

  // FFT requires powers of two in our implementation
  const int Wp = next_pow2(needW);
  const int Hp = next_pow2(needH);

  // Padded complex arrays
  std::vector<std::complex<double>> I(static_cast<size_t>(Wp) * Hp, {0.0, 0.0});
  std::vector<std::complex<double>> Kpad(static_cast<size_t>(Wp) * Hp, {0.0, 0.0});

  // Copy image into top-left corner (zero-padding elsewhere)
  for (int y = 0; y < H; ++y) {
    for (int x = 0; x < W; ++x) {
      I[idx2d(x, y, Wp)] = std::complex<double>(img.pixels[static_cast<size_t>(y) * W + x], 0.0);
    }
  }

  // Place kernel with center shifted to (0,0)
  place_centered_kernel(Kpad, Wp, Hp, k);

  // Forward FFTs
  fft2d_inplace(I, Wp, Hp, /*invert=*/false);
  fft2d_inplace(Kpad, Wp, Hp, /*invert=*/false);

  // Pointwise multiply in frequency domain
  for (size_t i = 0; i < I.size(); ++i) {
    I[i] *= Kpad[i];
  }

  // Inverse FFT to get spatial result
  fft2d_inplace(I, Wp, Hp, /*invert=*/true);

  // Crop back to original image size.
  // Taking the top-left W x H region matches our "same-size" naive output.
  Image out;
  out.width = W;
  out.height = H;
  out.pixels.resize(static_cast<size_t>(W) * H);

  for (int y = 0; y < H; ++y) {
    for (int x = 0; x < W; ++x) {
      out.pixels[static_cast<size_t>(y) * W + x] = I[idx2d(x, y, Wp)].real();
    }
  }

  return out;
}
