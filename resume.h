/*!
  \file resume.h
  \brief Routines to read/write restart file (header file)
  \author Y. Nakayama
  \date 2006/06/27
  \version 1.1
 */

#ifndef RESUME_H
#define RESUME_H

#include "input.h"
#include "variable.h"
#include "rigid.h"
#include "fft_wrapper.h"


//UDF RESTART WRITING
//

/*!
  \brief Save system parameters needed to restart a simulation
 */
void Save_Restart_udf(
		      double **zeta
		      ,double *uk_dc
		      ,const Particle *p
		      ,const CTime &time
		      ,double **conc_k
		      );

/*!
  \brief Write particle data to udf restart file
 */
void Save_Particle_udf(const Particle *p, const int &n_out_particles);

/*!
  \brief Write rigid body connectivity data to restart file
 */
void Save_Rigid_Particle_udf();

/*!
  \brief Creates temporary rigid particle structure
 */
void Get_Rigid_Particle_Data(Particle *rigid_p, const Particle *p);



//UDF RESTART READING
/*!
  \brief Set system parameters from restart file
 */
void Force_restore_parameters(
			      double **zeta
			      ,double *uk_dc
			      ,Particle *p
			      ,CTime &time
			      ,double **conc_k
			      );

/*!
  \brief Read particle data from udf restart file
 */
void Read_Particle_udf(Particle *p, const int &n_in_particles);

/*!
  \brief Read rigid body connectivity data from restart file
 */
void Read_Rigid_Particle_udf(Particle *rigid_p);

/*!
  \brief Recover rigid body configurations
 */
void Set_Rigid_Particle_Data(Particle *rigid_p, Particle *p);

void Save_Restart_udf_fdm(double **u, double **u_o, const Particle *p, const CTime &time);
void Save_Restart_udf_fdm_phase_separation(double **u, double **u_o, double * psi, double * psi_o, double **stress_o, const Particle *p, const CTime &time);
void Force_restore_parameters_fdm(double **u, double **u_o, Particle *p, CTime &time);
void Force_restore_parameters_fdm_phase_separation(double **u, double **u_o, double * psi, double * psi_o, double **stress_o, Particle *p, CTime &time);
#endif
