// ============================================================
// EJERCICIO 3 — COMPLETAR
// Objetivo: SAXPY -> y = a*x + y   (Single-precision A*X Plus Y)
// Es el "hola mundo" del algebra lineal en GPU. Mismo patron
// que el ej. 2, pero ahora hay un escalar 'a' y modificas 'y'
// en el lugar (in-place).
// ============================================================
#include <cstdio>
#include <cmath>
#include <cuda_runtime.h>

__global__ void saxpy(int n, float a, const float *x, float *y) {
    // TODO 1: indice global
    int i = /* ??? */ 0;

    // TODO 2: guarda contra desborde y hace y[i] = a*x[i] + y[i]
    // if (???) y[i] = ???;
}

int main() {
    const int N = 1 << 20;
    const size_t bytes = N * sizeof(float);
    const float A = 2.0f;

    float *h_x = (float*)malloc(bytes);
    float *h_y = (float*)malloc(bytes);
    for (int i = 0; i < N; ++i) { h_x[i] = 1.0f; h_y[i] = 2.0f; }
    // Esperado: y = 2*1 + 2 = 4.0

    float *d_x, *d_y;
    cudaMalloc(&d_x, bytes);
    cudaMalloc(&d_y, bytes);
    cudaMemcpy(d_x, h_x, bytes, cudaMemcpyHostToDevice);
    cudaMemcpy(d_y, h_y, bytes, cudaMemcpyHostToDevice);

    int threads = 256;
    int blocks = (N + threads - 1) / threads;   // <- fijate este patron de redondeo

    // TODO 3: lanza el kernel saxpy con la config <<<blocks, threads>>>
    //         y los argumentos (N, A, d_x, d_y)
    // saxpy<<< ??? >>>( ??? );

    cudaDeviceSynchronize();
    cudaMemcpy(h_y, d_y, bytes, cudaMemcpyDeviceToHost);

    float maxErr = 0.0f;
    for (int i = 0; i < N; ++i) maxErr = fmaxf(maxErr, fabsf(h_y[i] - 4.0f));
    printf(maxErr == 0.0f ? "OK: SAXPY correcto.\n" : "FALLO: error max = %f\n", maxErr);

    cudaFree(d_x); cudaFree(d_y);
    free(h_x); free(h_y);
    return maxErr == 0.0f ? 0 : 1;
}
