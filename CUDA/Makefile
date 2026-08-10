# Makefile para los ejercicios CUDA.
# Ya trae -ccbin g++-15 porque el sistema tiene GCC 16 (no soportado por nvcc 13.3).

NVCC    := /opt/cuda/bin/nvcc
CCBIN   := g++-15
NVFLAGS := -ccbin $(CCBIN) -O2

# Detecta todos los .cu y genera un binario por cada uno (sin extension).
SRCS := $(wildcard *.cu)
BINS := $(SRCS:.cu=)

all: $(BINS)

# Regla generica: cada %.cu -> binario %
%: %.cu
	$(NVCC) $(NVFLAGS) $< -o $@

# 'make run' compila y ejecuta todos en orden.
run: all
	@for b in $(BINS); do \
		echo "===== ./$$b ====="; \
		./$$b; \
		echo ""; \
	done

clean:
	rm -f $(BINS)

.PHONY: all run clean
