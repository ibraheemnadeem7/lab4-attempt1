# Lab 4 Write-up: Profiling Naive vs FFT Convolution

**Image:** `data/oldkenyon.ppm` — 1024 × 768 pixels  
**Platform:** Linux x86-64, compiled with `-O3 -g -march=native`  
**Profiler:** `perf record -g` / `perf report --stdio`

---

## Part 1 — Results Table

### Experiment 1: Small-kernel filter (emboss, 3×3) — FFT should lose

| Filter  | Kernel | Method | Run 1 (ms) | Run 2 (ms) | Average (ms) |
|---------|--------|--------|-----------|-----------|-------------|
| emboss  | 3×3    | naive  | 9.856     | 9.762     | **9.81**    |
| emboss  | 3×3    | fft    | 379.90    | 365.95    | **372.92**  |

FFT is **~38× slower** than naive for a 3×3 kernel.

### Experiment 2: Large blur (kernel-size 51) — FFT might win

| Filter | Kernel | Method | Run 1 (ms) | Run 2 (ms) | Average (ms) |
|--------|--------|--------|-----------|-----------|-------------|
| blur   | 51×51  | naive  | 2426.12   | 2420.78   | **2423.45** |
| blur   | 51×51  | fft    | 368.97    | 370.16    | **369.57**  |

FFT is **~6.6× faster** than naive for a 51×51 kernel.

### Experiment 3: Crossover sweep — blur, varying kernel size

| Filter | Kernel | Method | Average (ms) | Winner  |
|--------|--------|--------|-------------|---------|
| blur   | 3×3    | naive  | 12.03       | naive   |
| blur   | 3×3    | fft    | 391.44      |         |
| blur   | 7×7    | naive  | 40.07       | naive   |
| blur   | 7×7    | fft    | 385.00      |         |
| blur   | 15×15  | naive  | 216.35      | naive   |
| blur   | 15×15  | fft    | 386.54      |         |
| blur   | 31×31  | naive  | 910.12      | **fft** |
| blur   | 31×31  | fft    | 375.69      |         |
| blur   | 51×51  | naive  | 2425.00     | **fft** |
| blur   | 51×51  | fft    | 386.38      |         |
| blur   | 101×101| naive  | 9363.01     | **fft** |
| blur   | 101×101| fft    | 394.47      |         |

**Crossover is between k=15 and k=31.** At k=15, naive (216 ms) still beats FFT (387 ms). By k=31, FFT (376 ms) overtakes naive (910 ms), and the gap widens sharply from there.

A notable observation: FFT runtime stays nearly flat (~370–394 ms) across all kernel sizes, while naive scales quadratically.

---

## Part 2 — Profiler Evidence

Profiled with `perf record -g` on Linux. Workloads:
- FFT: `--filter blur --kernel-size 51 --repeat 5` (~2.2 s total)
- Naive: `--filter blur --kernel-size 51 --repeat 1` (~2.7 s total)

### FFT blur — top hotspots (`perf report --stdio --no-children`)

| % CPU | Function |
|-------|----------|
| 61.04% | `fft1d_inplace` — butterfly computation (radix-2 Cooley-Tukey) |
| 22.90% | `fft2d_inplace` — row/column loop, dispatches to `fft1d_inplace` |
|  5.44% | `do_user_addr_fault` (kernel) — page faults on initial array allocation |
|  3.13% | `convolve_fft` — setup: padding, pointwise multiply, crop |
|  2.92% | `clear_page_erms` (kernel) — zeroing freshly allocated pages |

**~84% of FFT time is inside the FFT butterflies themselves.** Another ~8% is pure memory
allocation overhead (page faults + zero-fill), confirming the fixed startup cost.

### Naive blur — top hotspots

| % CPU | Function |
|-------|----------|
| 99.58% | `convolve_naive` — the triple-nested pixel × kernel inner loop |
|  <0.5% | everything else (I/O, memory, kernel) |

Naive has zero overhead: 100% of time is the arithmetic inner loop.

---

## Part 3 — Explanation

### Why does FFT lose badly for emboss/edge/sharpen (3×3 kernels)?

FFT-based convolution has a large, fixed overhead that does not depend on kernel size:

1. **Memory allocation:** two complex arrays of size `Wp × Hp` must be allocated and
   zero-filled each call. For a 1024×768 image, the next power-of-2 padding is 2048×1024,
   meaning each array holds ~2M `complex<double>` values (~16 MB). The perf report
   confirmed 5.4% of FFT time is spent in `do_user_addr_fault` (page faults) and 2.9%
   in `clear_page_erms` (zeroing) — that is ~8% spent just on memory setup before any
   math begins.

2. **Three FFT passes:** a forward FFT of the image, a forward FFT of the kernel, and an
   inverse FFT of the product — each O(Wp · Hp · log(Wp · Hp)). The perf data shows 84%
   of total time inside `fft1d_inplace` / `fft2d_inplace`.

3. **The naive 3×3 cost is trivially small:** 1024 × 768 × 9 ≈ 7 million multiply-adds,
   completing in ~10 ms. FFT's fixed overhead of ~370 ms dwarfs this entirely, making FFT
   ~38× slower.

The key insight: FFT overhead is **O(N log N)** where N is the padded image area, independent
of kernel size. Naive cost is **O(N · k²)** where k is the kernel side length. For k=3,
O(N · 9) is far cheaper than the FFT baseline.

### For blur, does FFT become competitive? At what kernel sizes?

Yes. The crossover occurs between **k=15 and k=31** on this image:

- At k=15: naive ≈ 216 ms, FFT ≈ 387 ms → naive still faster
- At k=31: naive ≈ 910 ms, FFT ≈ 376 ms → FFT wins (~2.4×)
- At k=51: FFT is ~6.6× faster
- At k=101: FFT is **~23.7× faster** (naive: 9363 ms vs FFT: 394 ms)

**Why does FFT runtime stay flat?** All kernel sizes from k=3 to k=101 require padding to
the same grid: 2048×1024 (since 1024 + 101 − 1 = 1124, whose next power of 2 is still 2048;
768 + 101 − 1 = 868, whose next power of 2 is still 1024). The FFT is always operating on
the same-size arrays, so its runtime is constant regardless of k.

**Why does naive scale quadratically?** The inner loop runs k² times per output pixel and
there are W × H output pixels. Doubling k multiplies runtime by 4. From k=15 (216 ms) to
k=31 (910 ms): ratio = (31/15)² ≈ 4.3×, matching the observed ~4.2× slowdown. From k=31 to
k=101: ratio = (101/31)² ≈ 10.6×, observed ≈ 10.3× — almost perfect quadratic scaling.

### Where is the CPU time going for each method?

**Naive:** 99.6% of time is in `convolve_naive` — specifically the innermost loops:
```
for ky ... for kx ...
    acc += img[iy * W + ix] * k[ky * ks + kx];
```
Every cycle is a multiply-add. There is no overhead from memory allocation, FFT passes,
or anything else.

**FFT:** Time is split across three phases:
- ~84% in FFT butterfly math (`fft1d_inplace` + `fft2d_inplace`)
- ~8% in memory allocation/zero-fill (page faults + `clear_page_erms`)
- ~3% in `convolve_fft` for the pointwise frequency-domain multiply and image padding/crop

### Is performance limited by compute (math) or memory movement?

**Naive is compute-bound at large kernel sizes.** For k=51, each output pixel
requires 2601 multiply-add ops. The image (1024×768 doubles ≈ 6 MB) fits in L3 cache
after the first pass, so subsequent kernel-size iterations stay warm. The bottleneck is
raw floating-point throughput: ~2 billion multiply-adds per second matching what ~2.4 s
at 2601 ops/pixel × 786K pixels implies. The 99.6% perf concentration in one function
with zero kernel overhead confirms this.

**FFT is primarily compute-bound but with a notable memory component.** The 84% in FFT
butterfly functions indicates math dominates. However, the ~8% cost from page faults and
zero-fill reveals that memory allocation for the large complex arrays is a meaningful
secondary cost. The FFT butterfly itself has poor cache behavior (stride-2 access patterns
in the Cooley-Tukey algorithm hit many cache lines), which contributes to it being slower
in practice than its theoretical operation count would suggest.

**Summary table:**

| Method | Kernel | Bottleneck | Notes |
|--------|--------|------------|-------|
| naive  | small (≤15) | fixed overhead negligible, compute fast | wins easily |
| naive  | large (≥31) | compute-bound O(k²) inner loop | loses badly |
| fft    | all sizes   | FFT butterfly math + memory alloc overhead | ~370 ms floor |
| fft    | large (≥31) | compute-bound FFT on fixed-size padded grid | wins decisively |
