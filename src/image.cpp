#include "image.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace {

// Skip whitespace and comments (# ... end of line)
void skip_ws_and_comments(std::istream& in) {
  while (true) {
    int c = in.peek();
    if (c == '#') {
      std::string dummy;
      std::getline(in, dummy);
    } else if (std::isspace(c)) {
      in.get();
    } else {
      break;
    }
  }
}

// Read next integer token, skipping comments
int read_int(std::istream& in) {
  skip_ws_and_comments(in);
  int v;
  if (!(in >> v)) {
    throw std::runtime_error("Failed to read integer from image header");
  }
  return v;
}

} // namespace

Image load_pgm_or_ppm_grayscale(const std::string& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    throw std::runtime_error("Could not open image file: " + path);
  }

  std::string magic;
  in >> magic;
  if (magic != "P5" && magic != "P6") {
    throw std::runtime_error("Unsupported image format (expected P5 or P6): " + magic);
  }

  int width = read_int(in);
  int height = read_int(in);
  int maxval = read_int(in);

  if (width <= 0 || height <= 0) {
    throw std::runtime_error("Invalid image dimensions");
  }
  if (maxval <= 0 || maxval > 255) {
    throw std::runtime_error("Only 8-bit images supported (maxval <= 255)");
  }

  // Consume the single whitespace byte after maxval before binary data
  in.get();

  Image img;
  img.width = width;
  img.height = height;
  img.pixels.resize(static_cast<size_t>(width) * height);

  if (magic == "P5") {
    // Grayscale
    std::vector<uint8_t> buf(width * height);
    if (!in.read(reinterpret_cast<char*>(buf.data()), buf.size())) {
      throw std::runtime_error("Unexpected EOF while reading PGM data");
    }

    for (size_t i = 0; i < buf.size(); ++i) {
      img.pixels[i] = static_cast<double>(buf[i]);
    }
  } else {
    // P6: RGB → grayscale
    std::vector<uint8_t> buf(static_cast<size_t>(width) * height * 3);
    if (!in.read(reinterpret_cast<char*>(buf.data()), buf.size())) {
      throw std::runtime_error("Unexpected EOF while reading PPM data");
    }

    for (int i = 0; i < width * height; ++i) {
      uint8_t r = buf[3 * i + 0];
      uint8_t g = buf[3 * i + 1];
      uint8_t b = buf[3 * i + 2];

      // Standard luminance conversion
      img.pixels[i] = 0.299 * r + 0.587 * g + 0.114 * b;
    }
  }

  return img;
}

void save_pgm(const std::string& path, const Image& img) {
  if (img.width <= 0 || img.height <= 0 ||
      img.pixels.size() != static_cast<size_t>(img.width) * img.height) {
    throw std::runtime_error("Invalid image data for save");
  }

  std::ofstream out(path, std::ios::binary);
  if (!out) {
    throw std::runtime_error("Could not open output file: " + path);
  }

  // P5 header
  out << "P5\n" << img.width << " " << img.height << "\n255\n";

  std::vector<uint8_t> buf(static_cast<size_t>(img.width) * img.height);
  for (size_t i = 0; i < buf.size(); ++i) {
    double v = img.pixels[i];
    v = std::clamp(v, 0.0, 255.0);
    buf[i] = static_cast<uint8_t>(v + 0.5); // round to nearest
  }

  out.write(reinterpret_cast<const char*>(buf.data()), buf.size());
}
