#pragma once
#include <complex>
#include <vector>

int next_pow2(int n);

void fft1d_inplace(std::vector<std::complex<double>>& a, bool invert);

void fft2d_inplace(std::vector<std::complex<double>>& data,
                   int width, int height,
                   bool invert);
