#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "image.h"
#include "kernels.h"
#include "conv_naive.h"
#include "conv_fft.h"

using clock_t = std::chrono::high_resolution_clock;

struct Args {
  std::string in_path;
  std::string out_path;

  std::string filter = "blur";     // blur|sharpen|edge|emboss
  std::string method = "naive";    // naive|fft
  int kernel_size = 3;             // 3 default; allow 31/51 for blur demo
  int repeat = 1;                  // number of timed runs

  bool help = false;
};

static void print_usage(const char* prog) {
  std::cout
    << "Usage:\n"
    << "  " << prog << " --in INPUT.ppm --out OUTPUT.pgm [options]\n\n"
    << "Options:\n"
    << "  --filter NAME        blur|sharpen|edge|emboss (default: blur)\n"
    << "  --method NAME        naive|fft (default: naive)\n"
    << "  --kernel-size N      kernel size (odd). default: 3\n"
    << "                       for blur you may try 31 or 51 to show FFT advantage\n"
    << "  --repeat N           repeat timed runs (default: 1)\n"
    << "  --help               show this help\n\n"
    << "Example:\n"
    << "  " << prog << " --in data/oldkenyon.ppm --out out.pgm --filter edge --method fft --repeat 5\n";
}

static bool is_flag(const std::string& s) {
  return !s.empty() && s[0] == '-';
}

static int to_int(const std::string& s, const char* what) {
  try {
    size_t idx = 0;
    int v = std::stoi(s, &idx);
    if (idx != s.size()) throw std::invalid_argument("junk");
    return v;
  } catch (...) {
    throw std::runtime_error(std::string("Invalid integer for ") + what + ": " + s);
  }
}

static Args parse_args(int argc, char** argv) {
  Args a;

  std::vector<std::string> v(argv + 1, argv + argc);
  for (size_t i = 0; i < v.size(); ++i) {
    const std::string& tok = v[i];

    if (tok == "--help" || tok == "-h") {
      a.help = true;
      return a;
    }

    auto need_value = [&](const char* flag) -> const std::string& {
      if (i + 1 >= v.size() || is_flag(v[i + 1])) {
        throw std::runtime_error(std::string("Missing value after ") + flag);
      }
      return v[++i];
    };

    if (tok == "--in") {
      a.in_path = need_value("--in");
    } else if (tok == "--out") {
      a.out_path = need_value("--out");
    } else if (tok == "--filter") {
      a.filter = need_value("--filter");
    } else if (tok == "--method") {
      a.method = need_value("--method");
    } else if (tok == "--kernel-size") {
      a.kernel_size = to_int(need_value("--kernel-size"), "--kernel-size");
    } else if (tok == "--repeat") {
      a.repeat = to_int(need_value("--repeat"), "--repeat");
    } else {
      throw std::runtime_error("Unknown argument: " + tok);
    }
  }

  if (a.in_path.empty() || a.out_path.empty()) {
    throw std::runtime_error("You must specify --in and --out.");
  }
  if (a.kernel_size <= 0 || (a.kernel_size % 2) == 0) {
    throw std::runtime_error("--kernel-size must be a positive odd integer.");
  }
  if (a.repeat <= 0) {
    throw std::runtime_error("--repeat must be >= 1.");
  }

  // Minimal validation of enumerated options
  if (a.method != "naive" && a.method != "fft") {
    throw std::runtime_error("--method must be 'naive' or 'fft'.");
  }
  if (a.filter != "blur" && a.filter != "sharpen" && a.filter != "edge" && a.filter != "emboss") {
    throw std::runtime_error("--filter must be blur|sharpen|edge|emboss.");
  }

  // For demo clarity: large kernels only make sense for blur in this lab.
  if (a.kernel_size != 3 && a.filter != "blur") {
    throw std::runtime_error("For this demo, --kernel-size != 3 is only supported with --filter blur.");
  }

  return a;
}

static Image run_once(const Image& input, const Kernel& k, const std::string& method) {
  if (method == "naive") return convolve_naive(input, k);
  return convolve_fft(input, k);
}

int main(int argc, char** argv) {
  try {
    Args args = parse_args(argc, argv);
    if (args.help) {
      print_usage(argv[0]);
      return 0;
    }

    // Load image (we treat it as grayscale internally)
    Image input = load_pgm_or_ppm_grayscale(args.in_path);

    // Build kernel
    Kernel k = make_kernel(args.filter, args.kernel_size);

    // Warmup run (not timed): reduces one-time effects and ensures correctness before timing.
    // For very large kernels, this will also allocate FFT scratch buffers, etc.
    Image out = run_once(input, k, args.method);

    // Timed runs
    double total_ms = 0.0;
    for (int r = 0; r < args.repeat; ++r) {
      auto t0 = clock_t::now();
      out = run_once(input, k, args.method);
      auto t1 = clock_t::now();
      total_ms += std::chrono::duration<double, std::milli>(t1 - t0).count();
    }

    // Save result
    save_pgm(args.out_path, out);

    double avg_ms = total_ms / static_cast<double>(args.repeat);

    std::cout << "Method: " << args.method << "\n";
    std::cout << "Filter: " << args.filter << "\n";
    std::cout << "Kernel: " << args.kernel_size << "x" << args.kernel_size << "\n";
    std::cout << "Image:  " << input.width << " x " << input.height << "\n";
    std::cout << "Repeat: " << args.repeat << "\n";
    std::cout << "Average time: " << avg_ms << " ms\n";
    std::cout << "Wrote:  " << args.out_path << "\n";

    return 0;
  } catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << "\n\n";
    print_usage(argv[0]);
    return 1;
  }
}
