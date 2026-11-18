/* p4_ej3.c - juntar un resultado de todos: a mano o con una colectiva.
   mpicc -O2 -Wall -Wextra -o p4_ej3 p4_ej3.c ; mpirun -np 8 ./p4_ej3 */
#include <stdio.h>
#include <mpi.h>

#define REP 2000

/* A mano: cada rango manda su numero al 0, que los va sumando en un bucle. */
static double a_mano(double mio, int yo, int cuantos, double *total)
{
    double t0 = MPI_Wtime(), s = 0.0;
    for (int r = 0; r < REP; r++) {
        if (yo != 0) {
            MPI_Send(&mio, 1, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD);
        } else {
            s = mio;
            double ajeno;
            for (int k = 1; k < cuantos; k++) {
                MPI_Recv(&ajeno, 1, MPI_DOUBLE, k, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                s += ajeno;
            }
        }
    }
    *total = s;
    return (MPI_Wtime() - t0) / REP;
}

/* Con la colectiva: una sola llamada, que ademas la hacen todos. */
static double con_reduce(double mio, int yo, int cuantos, double *total)
{
    (void)cuantos;
    double suma = 0.0;
    double t0 = MPI_Wtime();
    for (int r = 0; r < REP; r++) {
        MPI_Reduce(&mio, &suma, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
    }
    double t = (MPI_Wtime() - t0) / REP;
    if (yo == 0) *total = suma;   /* solo el rango 0 tiene el resultado */
    return t;
}

int main(int argc, char **argv)
{
    MPI_Init(&argc, &argv);
    int yo, cuantos;
    MPI_Comm_rank(MPI_COMM_WORLD, &yo);
    MPI_Comm_size(MPI_COMM_WORLD, &cuantos);
    double mio = (double)(yo + 1), t1, t2, s1 = 0.0, s2 = 0.0;

    MPI_Barrier(MPI_COMM_WORLD);
    t1 = a_mano(mio, yo, cuantos, &s1);
    MPI_Barrier(MPI_COMM_WORLD);
    t2 = con_reduce(mio, yo, cuantos, &s2);

    double esperado = (double)cuantos * (cuantos + 1) / 2.0;
    if (yo == 0)
        printf("%3d procesos a mano %8.1f us MPI_Reduce %8.1f us %5.2fx %s\n",
               cuantos, t1 * 1e6, t2 * 1e6, t1 / t2,
               (s1 == esperado && s2 == esperado) ? "suma correcta" : "SUMA MAL");
    MPI_Finalize();
    return 0;
}
