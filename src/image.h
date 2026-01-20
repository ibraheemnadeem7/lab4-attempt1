#pragma once

#include <string>
#include <vector>

struct Image {
  int width = 0;
  int height = 0;
  std::vector<double> pixels;  // grayscale, row-major, size = width * height
};

// Load a PGM (P5) or PPM (P6) file and convert to grayscale Image.
// Throws std::runtime_error on error.
Image load_pgm_or_ppm_grayscale(const std::string& path);

// Save a grayscale image as PGM (P5).
void save_pgm(const std::string& path, const Image& img);
