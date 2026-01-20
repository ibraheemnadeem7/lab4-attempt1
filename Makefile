# ------------------------------------------------------------
# imgconv_fft Makefile
#
# Builds a single binary: imgconv
# Tuned for performance profiling (chrono, perf, Instruments)
# ------------------------------------------------------------

CXX       := g++
CXXFLAGS  := -std=c++17 -O3 -g -march=native -Wall -Wextra -pedantic
LDFLAGS   :=

TARGET    := imgconv
SRC_DIR   := src
OBJ_DIR   := build

SRCS := \
	$(SRC_DIR)/main.cpp \
	$(SRC_DIR)/image.cpp \
	$(SRC_DIR)/kernels.cpp \
	$(SRC_DIR)/conv_naive.cpp \
	$(SRC_DIR)/fft.cpp \
	$(SRC_DIR)/conv_fft.cpp

OBJS := $(SRCS:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)

# ------------------------------------------------------------
# Default target
# ------------------------------------------------------------

all: $(TARGET)

# ------------------------------------------------------------
# Link
# ------------------------------------------------------------

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

# ------------------------------------------------------------
# Compile
# ------------------------------------------------------------

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# ------------------------------------------------------------
# Build directory
# ------------------------------------------------------------

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

# ------------------------------------------------------------
# Utility targets
# ------------------------------------------------------------

clean:
	rm -rf $(OBJ_DIR) $(TARGET)

rebuild: clean all

.PHONY: all clean rebuild
