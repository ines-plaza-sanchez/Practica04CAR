#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

#define MAX_BYTES (8L * 1024L * 1024L) /* 8 MB = 8388608 bytes */

static int repeticiones_para(long bytes)
{
    if (bytes <= 1024L)       return 10000; /* miles para mensajes pequeños */
    if (bytes <= 65536L)      return 2000;
    if (bytes <= 524288L)     return 500;
    return 100;                             /* decenas/centenas para grandes */
}

int main(int argc, char **argv)
{
    MPI_Init(&argc, &argv);

    int yo, cuantos;
    MPI_Comm_rank(MPI_COMM_WORLD, &yo);
    MPI_Comm_size(MPI_COMM_WORLD, &cuantos);

    if (cuantos != 2) {
        if (yo == 0) {
            fprintf(stderr, "Este programa debe ejecutarse con 2 procesos (-n 2 / -np 2).\n");
        }
        MPI_Finalize();
        return 1;
    }

    int otro = 1 - yo;
    unsigned char *buf = (unsigned char *)malloc((size_t)MAX_BYTES);
    if (!buf) {
        fprintf(stderr, "Error al reservar memoria en rango %d.\n", yo);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    for (long i = 0; i < MAX_BYTES; i++) {
        buf[i] = (unsigned char)((i + yo + 1) & 0xFF);
    }

    volatile unsigned long sumidero = 0;

    if (yo == 0) {
        printf("%10s %14s %15s %12s %6s\n",
               "bytes", "ida_vuelta(us)", "una_dir(us)", "MB/s", "reps");
    }

    for (long n = 8L; n <= MAX_BYTES; n *= 4L) {
        int rep = repeticiones_para(n);

        MPI_Barrier(MPI_COMM_WORLD);
        double t0 = MPI_Wtime();

        for (int r = 0; r < rep; r++) {
            if (yo == 0) {
                MPI_Send(buf, (int)n, MPI_BYTE, otro, 0, MPI_COMM_WORLD);
                MPI_Recv(buf, (int)n, MPI_BYTE, otro, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            } else {
                MPI_Recv(buf, (int)n, MPI_BYTE, otro, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Send(buf, (int)n, MPI_BYTE, otro, 0, MPI_COMM_WORLD);
            }
        }

        double dt = MPI_Wtime() - t0;
        sumidero += (unsigned long)buf[0] + (unsigned long)buf[n - 1];

        if (yo == 0) {
            double rtt_s = dt / (double)rep;       /* tiempo ida y vuelta (s) */
            double dir_s = rtt_s / 2.0;            /* tiempo una direccion (s) */
            double rtt_us = rtt_s * 1e6;
            double dir_us = dir_s * 1e6;
            double mb_s = ((double)n / 1e6) / dir_s; /* MB/s usando una direccion */

            printf("%10ld %14.2f %15.2f %12.1f %6d\n",
                   n, rtt_us, dir_us, mb_s, rep);
        }
    }

    (void)sumidero;
    free(buf);
    MPI_Finalize();
    return 0;
}