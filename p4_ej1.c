
#include <stdio.h>
#include <math.h>
#include <mpi.h>

#define N 200000000L

/* Reparte los sumandos entre los procesos */
static void reparto(long n, int yo, int cuantos,
                    long *inicio, long *fin)
{
    long base = n / cuantos;
    long sobran = n % cuantos;

    *inicio = yo * base + (yo < sobran ? yo : sobran);
    *fin = *inicio + base + (yo < sobran ? 1 : 0);
}

/* Calcula la suma parcial */
static double trozo(long inicio, long fin)
{
    double s = 0.0;

    for (long i = inicio; i < fin; i++)
        s += sqrt((double)i + 0.5);

    return s;
}

int main(int argc, char **argv)
{
    MPI_Init(&argc, &argv);

    int yo, cuantos;

    MPI_Comm_rank(MPI_COMM_WORLD, &yo);
    MPI_Comm_size(MPI_COMM_WORLD, &cuantos);

    printf("hola, soy el rango %d de %d\n", yo, cuantos);
    fflush(stdout);

    long inicio, fin;

    reparto(N, yo, cuantos, &inicio, &fin);

    double t0 = MPI_Wtime();
    double mio = trozo(inicio, fin);
    double t = MPI_Wtime() - t0;

    /* Recogida de resultados */
    double total = mio;

    if (yo == 0) {

        /* El rango 0 recibe los resultados de los demas */
        for (int k = 1; k < cuantos; k++) {

            double ajeno;

            MPI_Recv(&ajeno, 1, MPI_DOUBLE, k, 0,
                     MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            total += ajeno;
        }

        printf("suma con %d procesos = %.6e\n", cuantos, total);
        printf("el rango 0 tardo %.4f s\n", t);

    } else {

        /* Los demas rangos envian su resultado al rango 0 */
        MPI_Send(&mio, 1, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD);
    }

    MPI_Finalize();

    return 0;
}
