#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <mpi.h>

#define N 4000000L
#define PASOS 200
#define ALFA 0.2

double reloj(void)
{
    struct timeval t;
    gettimeofday(&t, NULL);
    return t.tv_sec + t.tv_usec * 1e-6;
}

void reparto(long n, int rango, int procesos, long *inicio, long *fin)
{
    long base = n / procesos;
    long resto = n % procesos;

    *inicio = rango * base + (rango < resto ? rango : resto);
    *fin = *inicio + base + (rango < resto ? 1 : 0);
}

int main(int argc, char **argv)
{
    MPI_Init(&argc, &argv);

    int rango, procesos;
    MPI_Comm_rank(MPI_COMM_WORLD, &rango);
    MPI_Comm_size(MPI_COMM_WORLD, &procesos);

    long inicio, fin;
    reparto(N, rango, procesos, &inicio, &fin);

    long mios = fin - inicio;

    double *u = calloc((size_t)mios + 2, sizeof(double));
    double *v = calloc((size_t)mios + 2, sizeof(double));

    int error_memoria = (!u || !v);
    int algun_error;
    MPI_Allreduce(&error_memoria, &algun_error, 1, MPI_INT,
                  MPI_MAX, MPI_COMM_WORLD);

    if (algun_error) {
        if (rango == 0)
            fprintf(stderr, "Error reservando memoria\n");
        free(u);
        free(v);
        MPI_Finalize();
        return 1;
    }

    /* La barra comienza fría; el primer punto está a 100. */
    if (inicio == 0) {
        u[1] = 100.0;
        v[1] = 100.0;
    }

    int izq = (rango == 0) ? MPI_PROC_NULL : rango - 1;
    int der = (rango == procesos - 1) ? MPI_PROC_NULL : rango + 1;

    double t0 = MPI_Wtime();

    for (int paso = 0; paso < PASOS; paso++) {

        /* Intercambio del halo derecho. */
        MPI_Sendrecv(&u[1], 1, MPI_DOUBLE, izq, 0,
                     &u[mios + 1], 1, MPI_DOUBLE, der, 0,
                     MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        /* Intercambio del halo izquierdo. */
        MPI_Sendrecv(&u[mios], 1, MPI_DOUBLE, der, 1,
                     &u[0], 1, MPI_DOUBLE, izq, 1,
                     MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        /* No se actualizan los extremos físicos de la barra. */
        long primero = (inicio == 0) ? 2 : 1;
        long ultimo = (fin == N) ? mios - 1 : mios;

        for (long i = primero; i <= ultimo; i++) {
            v[i] = u[i] + ALFA * (u[i - 1] - 2.0 * u[i] + u[i + 1]);
        }

        double *aux = u;
        u = v;
        v = aux;
    }

    double tiempo_local = MPI_Wtime() - t0;
    double tiempo_paralelo;

    MPI_Reduce(&tiempo_local, &tiempo_paralelo, 1, MPI_DOUBLE,
               MPI_MAX, 0, MPI_COMM_WORLD);

    int *cuentas = NULL;
    int *desplazamientos = NULL;
    double *resultado = NULL;

    if (rango == 0) {
        cuentas = malloc((size_t)procesos * sizeof(int));
        desplazamientos = malloc((size_t)procesos * sizeof(int));
        resultado = malloc((size_t)N * sizeof(double));
    }

    int error_raiz = (rango == 0 &&
        (!cuentas || !desplazamientos || !resultado));
    MPI_Bcast(&error_raiz, 1, MPI_INT, 0, MPI_COMM_WORLD);

    if (error_raiz) {
        if (rango == 0)
            fprintf(stderr, "Error reservando memoria en rango 0\n");
        free(u);
        free(v);
        free(cuentas);
        free(desplazamientos);
        free(resultado);
        MPI_Finalize();
        return 1;
    }

    if (rango == 0) {
        for (int p = 0; p < procesos; p++) {
            long a, b;
            reparto(N, p, procesos, &a, &b);
            cuentas[p] = (int)(b - a);
            desplazamientos[p] = (int)a;
        }
    }

    MPI_Gatherv(&u[1], (int)mios, MPI_DOUBLE,
                resultado, cuentas, desplazamientos, MPI_DOUBLE,
                0, MPI_COMM_WORLD);

    if (rango == 0) {
        double *ref_u = malloc((size_t)N * sizeof(double));
        double *ref_v = malloc((size_t)N * sizeof(double));

        if (!ref_u || !ref_v) {
            fprintf(stderr, "Error reservando memoria para referencia\n");
            free(ref_u);
            free(ref_v);
            free(resultado);
            free(cuentas);
            free(desplazamientos);
            free(u);
            free(v);
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        for (long i = 0; i < N; i++) {
            ref_u[i] = 0.0;
            ref_v[i] = 0.0;
        }
        ref_u[0] = 100.0;
        ref_v[0] = 100.0;

        for (int paso = 0; paso < PASOS; paso++) {
            for (long i = 1; i < N - 1; i++) {
                ref_v[i] = ref_u[i] + ALFA *
                    (ref_u[i - 1] - 2.0 * ref_u[i] + ref_u[i + 1]);
            }

            double *aux = ref_u;
            ref_u = ref_v;
            ref_v = aux;
        }

        long distintas = 0;
        double suma = 0.0;

        for (long i = 0; i < N; i++) {
            if (resultado[i] != ref_u[i])
                distintas++;
            suma += resultado[i];
        }

        printf("%d procesos | paralelo %.6f s | posiciones distintas %ld"
               " | suma %.10e | u[1] = %.10e\n",
               procesos, tiempo_paralelo, distintas, suma, resultado[1]);

        free(ref_u);
        free(ref_v);
        free(resultado);
        free(cuentas);
        free(desplazamientos);
    }

    free(u);
    free(v);

    MPI_Finalize();
    return 0;
}
