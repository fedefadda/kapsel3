#include "wall.h"

void Init_Wall(double* phi_wall) {
    if (SW_WALL == FLAT_WALL) {  // print wall params
        char              dmy[128];
        static const char axis[] = {'x', 'y', 'z'};
        sprintf(dmy,
                "phi_flatwall_%dx%dx%d_h%c%d.dat",
                Ns[0],
                Ns[1],
                Ns[2],
                axis[wall.axis],
                static_cast<int>((wall.hi - wall.lo) / DX));

        FILE*  fwall = filecheckopen(dmy, "w");
        double dr    = DX;
        double l     = L[wall.axis];
        double hl    = l / 2.0;
        double r     = 0.0;
        double phiw  = 0.0;
        while (r <= l) {
            phiw = (r < hl ? Phi(r, wall.lo) : 1.0 - Phi(r, wall.hi));
            fprintf(fwall, "%.5f %.5f\n", r, phiw);
            r += dr;
        }
        fclose(fwall);
    }

    {  // compute wall phi field
        double  rijk[DIM] = {0.0, 0.0, 0.0};
        double& r         = rijk[wall.axis];
        double  hl        = HL[wall.axis];
        for (int i = 0; i < NX; i++) {
            rijk[0] = static_cast<double>(i) * DX;

            for (int j = 0; j < NY; j++) {
                rijk[1] = static_cast<double>(j) * DX;

                for (int k = 0; k < NZ; k++) {
                    rijk[2]      = static_cast<double>(k) * DX;
                    int im       = (i * NY * NZ_) + (j * NZ_) + k;
                    phi_wall[im] = (r < hl ? Phi(r, wall.lo) : 1.0 - Phi(r, wall.hi));
                }
            }
        }
    }
}
void Add_f_wall(Particle* p) {
    double cutoff = wall.A_R_cutoff * LJ_dia;
    double offset = 0.5 * LJ_dia;
    if (SW_WALL == FLAT_WALL) {
#pragma omp parallel for
        for (int n = 0; n < Particle_Number; n++) {
            double x   = p[n].x[wall.axis];
            double f_h = 0.0;
            double h   = x - wall.lo + offset;  // distance to lower mirror particle
            if (h <= cutoff) f_h += MIN(DBL_MAX / h, Lennard_Jones_f(h, LJ_dia)) * h;

            h = wall.hi - x + offset;
            if (h <= cutoff) f_h -= MIN(DBL_MAX / h, Lennard_Jones_f(h, LJ_dia)) * h;
            p[n].fr[wall.axis] += f_h;
        }
    }
}