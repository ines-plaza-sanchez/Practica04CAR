/* calor_sec.c - difusion de calor en una barra: version secuencial.
   Referencia de correccion y tiempo para el ejercicio 5.
   gcc -O2 -Wall -Wextra -o calor_sec calor_sec.c
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#define N     4000000
#define PASOS 200
#define ALFA  0.2

/* Sustituye a crono.h: devuelve el tiempo en segundos. */
double reloj(void)
{
    struct timeval t;
    gettimeofday(&t, NULL);
    return t.tv_sec + t.tv_usec * 1e-6;
}

/* Un paso: cada punto interior se acerca a la media de sus vecinos. */
void un_paso(const double *u, double *v, long desde, long hasta)
{
    for (long i = desde; i < hasta; i++)
        v[i] = u[i] + ALFA * (u[i - 1] - 2.0 * u[i] + u[i + 1]);
}

/* La barra empieza fria, con el extremo izquierdo caliente. */
void inicial(double *u, long n)
{
    for (long i = 0; i < n; i++)
        u[i] = 0.0;

    u[0] = 100.0;
}

int main(void)
{
    double *u = malloc(N * sizeof *u);
    double *v = malloc(N * sizeof *v);

    if (!u || !v) {
        fprintf(stderr, "sin memoria\n");
        free(u);
        free(v);
        return 1;
    }

    inicial(u, N);
    inicial(v, N);

    double t0 = reloj();

    for (int p = 0; p < PASOS; p++) {
        un_paso(u, v, 1, N - 1);

        double *aux = u;
        u = v;
        v = aux;
    }

    double t = reloj() - t0;

    double s = 0.0;
    for (long i = 0; i < N; i++)
        s += u[i];

    printf("%d puntos, %d pasos: %.4f s suma = %.10e u[1] = %.10e\n",
           N, PASOS, t, s, u[1]);

    free(u);
    free(v);

    return 0;
}
