# imgconv_fft — FFT-Based Image Convolution & Profiling Demo

This project demonstrates **naive vs FFT-based convolution** on a real image
(Kenyon College), and is designed specifically for **profiling and algorithm
analysis** in class.

It supports four filters:

* blur
* sharpen
* edge detect
* emboss

and two convolution methods:

* `naive` (direct spatial convolution)
* `fft` (FFT-based convolution using the convolution theorem)

The code is intentionally written to be:

* readable
* profiler-friendly
* free of external dependencies

This is **not** a DSP or graphics project.
It is an **algorithms + performance** demo.

---

## Quick Build

Requirements:

* C++17 compiler (g++ or clang++)
* Linux (for `perf`) or macOS (for Instruments)

Build everything with:

```bash
make
```

This produces:

```text
imgconv
```

The Makefile uses:

```text
-O3 -g -march=native
```

which is ideal for profiling.

---

## Quick Test (Sanity Check)

Run a simple blur using the naive method:

```bash
./imgconv \
  --in data/oldkenyon.ppm \
  --out blur_naive.pgm \
  --filter blur \
  --method naive
```

Now run the same blur using FFT:

```bash
./imgconv \
  --in data/oldkenyon.ppm \
  --out blur_fft.pgm \
  --filter blur \
  --method fft
```

The two output images should look **visually identical**.
This confirms correctness before profiling.

---

## Expected Demo Commands (In-Class)

### 1. Chrono timing: naive vs FFT (small kernel)

```bash
./imgconv --filter blur --method naive --repeat 5 --out /tmp/a.pgm
./imgconv --filter blur --method fft   --repeat 5 --out /tmp/b.pgm
```

Expected observation:

* Naive is often faster for 3×3 blur
* FFT has noticeable overhead

This is a **feature**, not a bug.

---

### 2. Chrono timing: large blur (FFT wins)

```bash
./imgconv --filter blur --kernel-size 51 --method naive --repeat 3 --out /tmp/a.pgm
./imgconv --filter blur --kernel-size 51 --method fft   --repeat 3 --out /tmp/b.pgm
```

Expected observation:

* Naive becomes extremely slow
* FFT runtime grows much more slowly
* Algorithmic complexity becomes obvious

---

### 3. Visual filters (FFT method)

```bash
./imgconv --filter sharpen --method fft --out sharpen.pgm
./imgconv --filter edge    --method fft --out edge.pgm
./imgconv --filter emboss  --method fft --out emboss.pgm
```

What students should notice:

* Sharpen increases edge contrast
* Edge detect removes most low-frequency regions
* Emboss shows strong directional structure
* All are the same algorithm with different kernels

---

## Profiling Demos

### Linux: perf

Build (already done by default):

```bash
make
```

Profile FFT edge detection:

```bash
perf record -g ./imgconv --filter edge --method fft --repeat 10 --out /tmp/o.pgm
perf report
```

What to show in class:

* `fft1d_inplace` dominates runtime
* Clear call tree
* Image I/O is negligible
* Optimizations move time, they don’t eliminate it

---

### macOS: Instruments Time Profiler

1. Open **Instruments**
2. Choose **Time Profiler**
3. Select the `imgconv` executable
4. Set arguments:

   ```text
   --filter edge --method fft --repeat 10 --out /tmp/o.pgm
   ```
5. Record and stop

What to show:

* Call tree view
* FFT stages (butterflies, bit reversal)
* Phase-based execution (FFT → multiply → inverse FFT)

---

## Command-Line Summary

```text
--in INPUT.ppm          input image (PGM or PPM)
--out OUTPUT.pgm        output image (PGM)
--filter NAME           blur | sharpen | edge | emboss
--method NAME           naive | fft
--kernel-size N         odd size (only >3 supported for blur)
--repeat N              repeat timed runs
```

---

## What This Demo Is For

* Comparing algorithmic complexity in practice
* Showing why asymptotics matter
* Teaching profiling as a **measurement tool**, not a guess
* Demonstrating that “faster” depends on input size

## What This Demo Is Not

* Not RF modeling
* Not image processing theory
* Not a graphics pipeline
* Not optimized production FFT code

---

## Teaching Notes

Some discussion questions:

* Why does naive win for small kernels?
* Where does FFT spend its time?
* What changes when image size doubles?
* Why doesn’t optimization eliminate runtime?

This project exists to make those questions concrete.
