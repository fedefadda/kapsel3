#ifndef AVS_PROCESS_H
#define AVS_PROCESS_H
#include <string.h>
#include <iostream>
#include <fstream>
#include <dirent.h>
#include <assert.h>
#include "alloc.h"
#include "udfmanager.h"
#include "macro.h"
#include "parameter_define.h"
#include "tricubic.h"
#include "rigid_body.h"
#include "quaternion.h"
enum V_OPTION {relative_velocity, total_velocity};
enum JAX {x_axis, y_axis, z_axis};
enum INTERPOLATION {cubic_interpol, trilinear_interpol};
char AVS_dir[128];      // Main AVS directory
char Out_dir[128];      // AVS fluid directory
char Out_name[128];     // AVS fluid filename
char Outp_dir[128];     // AVS particle directory
char Outp_name[128];    // AVS particle filename
char Post_dir[128];     // AVS post-process directory
char Post_name[128];    // AVS post-process file nameq
int fluid_veclen;    
int particle_veclen;
const int post_veclen = 3;
const char *Label = "ux uy uz";

double ex[DIM] = {1.0, 0.0, 0.0};
double ey[DIM] = {0.0, 1.0, 0.0};
double ez[DIM] = {0.0, 0.0, 1.0};
double *e1;
double *e2;
double *e3;

//
FILE *fluid_field;
FILE *post_field;
FILE *particle_field;
//
FILE *fluid_cod;
FILE *post_cod;
FILE *particle_cod;
//
FILE *fluid_data;
FILE *post_data;
FILE *particle_data;

int GTS;                // steps between avs dumps
int Num_snap;           // number of avs dumps
double A, A_XI, DX;     // Particle radius, size, units
int Ns[DIM];             // dimensions of original box
int &NX = Ns[0];
int &NY = Ns[1];
int &NZ = Ns[2];
double lbox[DIM];
int Cs[DIM];            // dimension of new box
int HCs[DIM];
int &CX = Cs[0];
int &CY = Cs[1];
int &CZ = Cs[2];
int &HCX = HCs[0];
int &HCY = HCs[1];
int &HCZ = HCs[2];
int Nspec;                 // number of species
int Nparticles;            // total number of particles
double **u, **post_u;      //
double B1_real;
double B1_app;
double B2;
double r0[DIM]; 
int r0_int[DIM];
double res0[DIM];
double v0[DIM];
double w0[DIM];
double f0[DIM];
double t0[DIM];
double Q0[DIM][DIM];
quaternion q0;
INTERPOLATION SW_INTERPOL;


inline double H(const double x){
  return x > 0 ? exp(-SQ(DX/x)) : 0;
}
inline double Phi(const double &x, const double radius){
  double HXI = A_XI / 2.0;
  double dmy = H(radius + HXI - x);
  return dmy / (dmy + H(x - radius + HXI));
}
inline void PBC_ip(int &ip, const int &Ns){
  ip = (ip + Ns) % Ns;
  assert(ip >= 0 && ip < Ns);
}
inline void PBC_ip(int *ip){
  PBC_ip(ip[0], NX);
  PBC_ip(ip[1], NY);
  PBC_ip(ip[2], NZ);
}
inline void PBC_ip(int *ip, const int &NX, const int &NY, const int &NZ){
  PBC_ip(ip[0], NX);
  PBC_ip(ip[1], NY);
  PBC_ip(ip[2], NZ);
}
inline int linear_id(const int &i, const int &j, const int &k){
  return (i * NY * NZ) + (j * NZ) + k;
}
inline int linear_id(const int &i, const int &j, const int &k, 
		     const int &NX, const int &NY, const int &NZ){
  return (i * NY * NZ) + (j * NZ) + k;
}
void get_filename(char* line, char*name){
  char dmy_char[256];
  string dmy_string;
  size_t start, end;

  dmy_string.assign(line);
  start = dmy_string.find_first_of("=") + 2;
  end = (dmy_string.substr(start)).find_first_of(" ") + start;
  strcpy(name, dmy_string.substr(start, end-start).c_str());
}

inline void read_ux(double *ux, FILE *fstream){
  float dmy;
  for(int k = 0; k < NZ; k++){
    for(int j = 0; j < NY; j++){
      for(int i = 0; i < NX; i++){
	int im = linear_id(i, j, k);
	fread(&dmy, sizeof(float), 1, fstream);
	ux[im] = (double)dmy;
      }
    }
  }
}
inline void read_pid(const int &pid, double x[DIM], FILE *fstream){
  float dmy;
  for(int d = 0; d < DIM; d++){
    for(int i = 0; i < Nparticles; i++){
      fread(&dmy, sizeof(float), 1, fstream);
      if(i == pid){
	x[DIM - 1 - d] = (double)dmy;
      }
    }
  }
}
inline void read_pid(const int &pid, double &a, FILE *fstream){
  float dmy;
  for(int i = 0; i < Nparticles; i++){
    fread(&dmy, sizeof(float), 1, fstream);
    if(i == pid){
      a = (double)dmy;
    }
  }
}
inline void read_pid(const int &pid, double Q[DIM][DIM], FILE *fstream){
  float dmy;
  for(int m = 0; m < DIM; m++){
    for(int n = 0; n < DIM; n++){
      for(int i = 0; i < Nparticles; i++){
	fread(&dmy, sizeof(float), 1, fstream);
	if(i == pid){
	  Q[n][m] = (double) dmy;
	}
      }
    }
  }
}
inline void read_u(){
  read_ux(u[0], fluid_data);
  read_ux(u[1], fluid_data);
  read_ux(u[2], fluid_data);
}
inline void read_p(const int &pid){
  double A0;
  float dmy;

  read_pid(pid, r0, particle_cod);
  read_pid(pid, A0, particle_data);
  read_pid(pid, v0, particle_data);
  read_pid(pid, w0, particle_data);
  read_pid(pid, f0, particle_data);
  read_pid(pid, t0, particle_data);
  read_pid(pid, Q0, particle_data);
  rm_rqtn(q0, Q0);

  double jax[DIM];
  rigid_body_rotation(jax, e3, q0, BODY2SPACE);
  B1_app = 3.0/2.0 * (jax[0]*v0[0] + jax[1]*v0[1] + jax[2]*v0[2]);

  Particle_cell(r0, DX, r0_int, res0);
}

void clear_avs_frame(){
  fclose(fluid_cod);
  fclose(particle_cod);

  fclose(fluid_data);
  fclose(particle_data);

  fclose(post_data);

  fluid_cod = NULL;
  fluid_data = NULL;
  particle_data = NULL;
  post_data = NULL;
}
void setup_avs_frame(){
  //find files for new frame
  size_t len, start, end;
  char* line = NULL;
  string dmy_string;

  // step header
  getline(&line, &len, particle_field);
  getline(&line, &len, fluid_field);
  fprintf(post_field, "%s", line);

  char dmy_fluid[256], fluid_path[256];
  char dmy_particle[256], particle_path[256];
  char dmy_post[256], post_path[256];

  // coord file
  for(int i = 0; i < DIM; i++){
    getline(&line, &len, fluid_field);
    get_filename(line, dmy_fluid);

    getline(&line, &len, particle_field); 
    get_filename(line, dmy_particle);


    fprintf(post_field, "coord %d file = %s.cod filetype = ascii skip = %d\n",
	    i+1, Post_name, i*2);
  }
  sprintf(fluid_path, "%s/%s", AVS_dir, dmy_fluid);
  sprintf(particle_path, "%s/%s", AVS_dir, dmy_particle);
  fluid_cod = filecheckopen(fluid_path, "r");                 //particle coordinate file
  particle_cod = filecheckopen(particle_path, "r");
  //  fprintf(stderr, "# %32s  ", particle_path);


  //INPUT field data file
  for(int i = 0; i < fluid_veclen; i++){
    getline(&line, &len, fluid_field);
    get_filename(line, dmy_fluid);
  }
  getline(&line, &len, fluid_field);//EOT
  sprintf(fluid_path, "%s/%s", AVS_dir, dmy_fluid);
  fluid_data = filecheckopen(fluid_path, "r");                //fluid data file
  //  fprintf(stderr, "%32s  ", fluid_path);

  for(int i = 0; i < particle_veclen; i++){
    getline(&line, &len, particle_field);
    get_filename(line, dmy_particle);
  }
  getline(&line, &len, particle_field);//EOT
  sprintf(particle_path, "%s/%s", AVS_dir, dmy_particle);
  particle_data = filecheckopen(particle_path, "r");          //particle data file
  //  fprintf(stderr, "%32s  ", particle_path);

  //OUTPUT field data file
  int data_size = sizeof(float)*CX*CY*CZ;
  dmy_string.assign(dmy_fluid);
  start = dmy_string.find_first_not_of(Out_dir);
  sprintf(dmy_post, "%s/post_%s", Post_dir, dmy_string.substr(start+1).c_str());
  for(int i=0; i < post_veclen; i++){
    fprintf(post_field, "variable %d file = %s filetype = binary skip = %d\n", 
	    i+1, dmy_post, i *data_size);
  }
  fprintf(post_field, "EOT\n");
  sprintf(post_path, "%s/%s", AVS_dir, dmy_post);
  post_data = filecheckopen(post_path, "w");                  //fluid post data file 
  //  fprintf(stderr, "%42s\n", post_path);
}

void initialize_avs(){
  char dmy_char[256];
  string dmy_string;
  char* line = NULL;
  size_t len, found;
  assert((double)Cs[0] * DX > A + A_XI &&
	 (double)Cs[1] * DX > A + A_XI &&
	 (double)Cs[2] * DX > A + A_XI);
  assert(Cs[0] <= Ns[0] && Cs[1] <= Ns[1] && Cs[2] <= Ns[2]);

  //open field 
  sprintf(dmy_char, "%s/%s.fld", AVS_dir, Out_name);
  fluid_field = filecheckopen(dmy_char, "r");
  getline(&line, &len, fluid_field); //Header
  getline(&line, &len, fluid_field); //ndim
  getline(&line, &len, fluid_field); //dim1
  getline(&line, &len, fluid_field); //dim2
  getline(&line, &len, fluid_field); //dim3
  getline(&line, &len, fluid_field); //nspace
  getline(&line, &len, fluid_field); //veclen

  dmy_string.assign(line);
  found = dmy_string.find("=");
  fluid_veclen = atoi(dmy_string.substr(++found).c_str());

  getline(&line, &len, fluid_field); //float data
  getline(&line, &len, fluid_field); //uniform  
  getline(&line, &len, fluid_field); //nstep
  getline(&line, &len, fluid_field); //label

  sprintf(dmy_char, "%s/%s.fld", AVS_dir, Outp_name);
  particle_field = filecheckopen(dmy_char, "r");
  getline(&line, &len, particle_field); //Header
  getline(&line, &len, particle_field); //ndim
  getline(&line, &len, particle_field); //dim1
  getline(&line, &len, particle_field); //nspace
  getline(&line, &len, particle_field); //veclen

  dmy_string.assign(line);
  found = dmy_string.find("=");
  particle_veclen = atoi(dmy_string.substr(++found).c_str());

  getline(&line, &len, particle_field); //float data
  getline(&line, &len, particle_field); //irregular  field
  getline(&line, &len, particle_field); //nstep
  getline(&line, &len, particle_field); //label

  sprintf(dmy_char, "%s/%s.fld", AVS_dir, Post_name);
  post_field = filecheckopen(dmy_char, "w");
  fprintf(post_field, "# AVS field file\n");
  fprintf(post_field, "ndim=%d\n", DIM);
  fprintf(post_field, "dim1=%d\n", Cs[0]);
  fprintf(post_field, "dim2=%d\n", Cs[1]);
  fprintf(post_field, "dim3=%d\n", Cs[2]);
  fprintf(post_field, "nspace=%d\n", DIM);
  fprintf(post_field, "veclen=%d\n", post_veclen);
  fprintf(post_field, "data=float\n");
  fprintf(post_field, "field=uniform\n");
  fprintf(post_field, "nstep=%d\n", Num_snap+1);
  fprintf(post_field, "label = %s\n", Label);

  //write post fluid .cod  file
  sprintf(dmy_char, "%s/%s.cod", AVS_dir, Post_name);
  FILE *dmy_post = filecheckopen(dmy_char, "w");
  int dmy_start = 0;
  fprintf(dmy_post, "%g\n%g\n%g\n%g\n%g\n%g\n",
	  (double)dmy_start*DX,
	  (double)(Cs[0]-1)*DX,
	  (double)dmy_start*DX,
	  (double)(Cs[1]-1)*DX,
	  (double)dmy_start*DX,
	  (double)(Cs[2]-1)*DX
	  );
  fclose(dmy_post);
}

void wrong_invocation(){
  fprintf(stderr, "usage: pavs -p PID -l Ns -i UDF [-v]\n");
  fprintf(stderr, "       -p PID\t ID (1...N) of centered particle\n");
  fprintf(stderr, "       -l Ns \t Box size length\n");
  fprintf(stderr, "       -i UDF\t Input file\n");
  fprintf(stderr, "       -v    \t Gives relative velocity (co-moving frame)\n");
  exit_job(EXIT_FAILURE);
}
void get_system_data(UDFManager *ufin, int *&p_spec, JAX *&sp_axis, double *&sp_slip, double *&sp_slipmode){
  { // particle size
    Location target("constitutive_eq");
    string str;
    ufin->get(target.sub("type"),str);
    target.down(str);
    ufin->get(target.sub("DX"), DX);
    ufin->get("A", A);
    ufin->get("A_XI", A_XI);
    fprintf(stderr, "#DX   : %8.3f\n", DX);
    fprintf(stderr, "#A    : %8.3f\n", A);
    fprintf(stderr, "#A_XI : %8.3f\n", A_XI);
    A *= DX;
    A_XI *= DX;
  }
  { // mesh size
    Location target("mesh");
    int dmy[DIM];
    ufin->get(target.sub("NPX"), dmy[0]);
    ufin->get(target.sub("NPY"), dmy[1]);
    ufin->get(target.sub("NPZ"), dmy[2]);
    for(int d = 0; d < DIM; d++){
      Ns[d] = 1 << dmy[d];
      lbox[d] = Ns[d] * DX;
    }
    fprintf(stderr, "#NS   : %8d %8d %8d\n\n", Ns[0], Ns[1], Ns[2]);
    
  }
  {  // object data
    Location target("object_type");
    string str;
    ufin->get(target.sub("type"), str);
    if(str != "spherical_particle"){
      fprintf(stderr, "Error: only spherical particles supported\n");
      exit_job(EXIT_FAILURE);
    }

    Nspec = ufin->size("object_type.spherical_particle.Particle_spec[]");
    sp_axis = (JAX*) malloc(sizeof(JAX) * Nspec);
    sp_slip = (double*) malloc(sizeof(double) * Nspec);
    sp_slipmode = (double*) malloc(sizeof(double) * Nspec);
    int * dmy_num = (int*)malloc(sizeof(int) * Nspec);
    Nparticles = 0;
    for(int i = 0; i < Nspec; i++){
      char str[256];
      sprintf(str, "object_type.spherical_particle.Particle_spec[%d]", i);
      Location target(str);
      ufin->get(target.sub("Particle_number"), dmy_num[i]);
      Nparticles += dmy_num[i];

      string str2;
      ufin->get(target.sub("janus_axis"), str2);
      if(str2 == "X"){
	sp_axis[i] = x_axis;
      }else if(str2 == "Y"){
	sp_axis[i] = y_axis;
      }else if(str2 == "Z"){
	sp_axis[i] = z_axis;
      }else{
	fprintf(stderr, "Error: Unknown axis specification\n");
	exit_job(EXIT_FAILURE);
      }

      ufin->get(target.sub("janus_propulsion"), str2);
      if(str2 == "SLIP"){
	ufin->get(target.sub("janus_slip_vel"), sp_slip[i]);
	ufin->get(target.sub("janus_slip_mode"), sp_slipmode[i]);
      }
    }
    p_spec = (int*)malloc(sizeof(int) * Nparticles);
    int count = 0;
    for(int i = 0; i < Nspec; i++)
      for(int j = 0; j < dmy_num[i]; j++)
	p_spec[count++] = i;
    assert(count == Nparticles);

    fprintf(stderr, " #species   : %8d\n", Nspec);
    fprintf(stderr, " #particles : %8d\n\n", Nparticles);
    free(dmy_num);
  }
  {  // avs output
    string str;
    Location target("output");
    ufin->get(target.sub("GTS"), GTS);
    ufin->get(target.sub("Num_snap"), Num_snap);
    target.down("ON");
    ufin->get(target.sub("Out_dir"), str);
    strcpy(AVS_dir, str.c_str());
    if(opendir(AVS_dir) == NULL){
      fprintf(stderr, "Error: AVS directory does not exist\n");
      exit_job(EXIT_FAILURE);
    }
    ufin->get(target.sub("Out_name"), str);
    strcpy(Out_name, str.c_str());
    
    ufin->get(target.sub("FileType"), str);
    if(str != "BINARY"){
      fprintf(stderr, "Error: Unknown filetype\n");
      exit_job(EXIT_FAILURE);
    }

    str = "avs";
    strcpy(Out_dir, str.c_str());
    sprintf(Outp_dir, "%s", Out_dir);
    sprintf(Outp_name, "%sp", Out_name);
    sprintf(Post_dir, "post_%s", Out_dir);
    sprintf(Post_name, "post_%s", Out_name);

    fprintf(stderr, " #Avs directory :  %15s\n", AVS_dir);
    fprintf(stderr, " #data directory : %15s %15s %15s \n", Out_dir, Outp_dir, Post_dir);
    fprintf(stderr, " #data filename  : %15s %15s %15s \n\n", Out_name, Outp_name, Post_name);
  }
}
void initialize(int argc, char *argv[], 
		UDFManager* &udf_in,
		int &pid, 
		V_OPTION &vflag){
  
  char* fname;
  int pset, lset, fset, vset;
  pset = lset = fset = vset = 0;
  vflag = total_velocity;
  SW_INTERPOL = cubic_interpol;
  for(int i = 1; i < argc; i++){
    if(strcmp(argv[i], "-p") == 0){
      if(i+1 == argc || argv[i+1][0] == '-'){
	fprintf(stderr, "Error: No particle selected\n");
	wrong_invocation();
      }
      pid = atoi(argv[++i]);
      if(--pid < 0){
	fprintf(stderr, "Incorrect id\n");
	wrong_invocation();
      }
      pset = 1;
    }else if(strcmp(argv[i],"-l") == 0){
      if(i+1 == argc || argv[i+1][0] == '-'){
	fprintf(stderr, "Error: No box size selected\n");
	wrong_invocation();
      }
      Cs[0] = atoi(argv[++i]);
      if(Cs[0] == 0){
	fprintf(stderr, "Incorrect size\n");
	wrong_invocation();
      }      
      if(Cs[0]%2 == 0){
	Cs[0]++;
      }
      Cs[1] = Cs[2] = Cs[0];
      HCs[0] = HCs[1] = HCs[2] = Cs[0] / 2;
      lset = 1;
    }else if(strcmp(argv[i], "-i") == 0){
      if(i+1 == argc || argv[i+1][0] == '-'){
	fprintf(stderr, "Error: No udf file selected\n");
	wrong_invocation();
      }
      i++;
      fname = argv[i];
      if(file_check(argv[i])){
	udf_in = new UDFManager(argv[i]);
      }else{
	fprintf(stderr, "Error: Cannot open file\n");
      }
      fset = 1;
    }else if(strcmp(argv[i], "-v") == 0){
      vflag = relative_velocity;
    }else if(strcmp(argv[i], "-nc") == 0){
      fprintf(stderr, "### LINEAR INTERPOLATION\n");
      SW_INTERPOL = trilinear_interpol;
    }
  }

  if(!pset){
    fprintf(stderr, "Error: No particle selected\n");
    wrong_invocation();
  }
  if(!lset){
    fprintf(stderr, "Error: No box size selected\n");
    wrong_invocation();
  }
  if(!fset){
    fprintf(stderr, "Error: No udf file selected\n");
    wrong_invocation();
  }

  fprintf(stderr, " # Centered Particle: %8d\n", pid + 1);
  fprintf(stderr, " # Box Size         : (%d,%d,%d)\n", 
	  Cs[0], Cs[1], Cs[2]);
  fprintf(stderr, " # HBox Size        : (%d,%d,%d)\n", 
	  HCs[0], HCs[1], HCs[2]);
  fprintf(stderr, " # UDF File         : %s\n", fname);
  fprintf(stderr, " # Relative Velocity: %s\n\n", 
	  (vflag == relative_velocity) ? "YES" : "NO");

}


inline void Particle_cell(const double *xp,
			 const double &dx,
			 int *x_int,
			 double *residue){
  const double idx = 1.0/dx;
  {
    for(int d = 0; d < DIM; d++){
      assert(xp[d] >= 0 && xp[d] < lbox[0]);
    }
    for(int d = 0; d < DIM; d++){
      double dmy = (xp[d] * idx);
      x_int[d] = (int) dmy;

      residue[d] = (dmy - (double)x_int[d]) * dx;
    }
  }
}
inline double trilinear_eval(const double pval[8], double &x, double &y, double &z){
  double pint = (pval[0] * (1.0 - x) * (1.0 - y) * (1.0 -z))
    + (pval[1] * x * (1.0 - y) * (1.0 - z))
    + (pval[2] * (1.0 - x) * y * (1.0 - z))
    + (pval[3] * x * y * (1.0 - z))
    + (pval[4] * (1.0 - x) * (1.0 - y) * z)
    + (pval[5] * x * (1.0 - y) * z)
    + (pval[6] * (1.0 - x) * y * z)
    + (pval[7] * x * y * z);
  return pint;
}
void init_interpol(int pid[8][DIM], double pval[8], double pcoeff[64], const double *field, const int*p0){
  const int ei[8][DIM] = 
    {{0, 0, 0}, 
     {1, 0, 0},
     {0, 1, 0},
     {1, 1, 0},
     {0, 0, 1},
     {1, 0, 1},
     {0, 1, 1},
     {1, 1, 1}};
  double pdx[8], pdy[8], pdz[8];
  double pdxdy[8], pdxdz[8], pdydz[8], pdxdydz[8];
  int im;

  //get function values at cube edges
  for(int l = 0; l < 8; l++){
    for(int d = 0; d < DIM; d++){
      pid[l][d] = p0[d] + ei[l][d];
    }
    PBC_ip(pid[l]);
    pval[l] = field[linear_id(pid[l][0], pid[l][1], pid[l][2])];
  }
  for(int l = 0; l < 64; l++){
    pcoeff[l] = 0.0;
  }

  //Use finite difference approximations for partial derivatives
  //Assuming unit cube (using scaled coordinates)
  if(SW_INTERPOL == cubic_interpol){
    for(int l = 0; l < 8; l++){
      pdx[l] = pdy[l] = pdz[l] = pdxdy[l] = pdxdz[l] = pdydz[l] = pdxdydz[l] = 0.0;
      int &x = pid[l][0];
      int &y = pid[l][1];
      int &z = pid[l][2];

      for(int i = -1; i <= 1; i = i + 2){
	int dmy_xi = x + i; PBC_ip(dmy_xi, NX);
	int dmy_yi = y + i; PBC_ip(dmy_yi, NY);
	int dmy_zi = z + i; PBC_ip(dmy_zi, NZ);

	pdx[l] += i * field[linear_id(dmy_xi, y, z)];
	pdy[l] += i * field[linear_id(x, dmy_yi, z)];
	pdz[l] += i * field[linear_id(x, y, dmy_zi)];

	for(int j = -1; j <= 1; j = j + 2){
	  int dmy_yj = y + j; PBC_ip(dmy_yj, NY);
	  int dmy_zj = z + j; PBC_ip(dmy_zj, NZ);

	  pdxdy[l] += i * j * field[linear_id(dmy_xi, dmy_yj, z)];
	  pdxdz[l] += i * j * field[linear_id(dmy_xi, y, dmy_zj)];
	  pdydz[l] += i * j * field[linear_id(x, dmy_yi, dmy_zj)];

	  for(int k = -1; k <= 1; k = k + 2){
	    int dmy_zk = z + k; PBC_ip(dmy_zk, NZ);

	    pdxdydz[l] += i * j * k * field[linear_id(dmy_xi, dmy_yj, dmy_zk)];
	  }//k
	}//j
      }//i
      pdx[l] /= 2.0;
      pdy[l] /= 2.0;
      pdz[l] /= 2.0;
      pdxdy[l] /= 4.0;
      pdxdz[l] /= 4.0;
      pdydz[l] /= 4.0;
      pdxdydz[l] /= 8.0;
    }//l
    tricubic_get_coeff(pcoeff, pval, pdx, pdy, pdz, pdxdy, pdxdz, pdydz, pdxdydz);
  }
}

inline double interpol(const int pid[8][DIM], const double pval[8], double pcoeff[64],
		       const double *r){
  //scaled distance from edge to r
  double x = r[0] / DX;
  double y = r[1] / DX;
  double z = r[2] / DX;
  assert(x >= 0 && y >= 0 && z >= 0);
  assert(x <= 1 && y <= 1 && z <= 1);
  if(SW_INTERPOL == trilinear_interpol){
    return trilinear_eval(pval, x, y, z);
  }else if (SW_INTERPOL == cubic_interpol){
    return tricubic_eval(pcoeff, x, y, z);
  }
}

void process_avs_frame(const V_OPTION &vflag, const int &frame){
  double rp[DIM], sp[DIM], cp[DIM];
  int ei[DIM], edge[DIM];
  int im;
  
  int interpol_pid[8][DIM];
  double interpol_pval[8];
  double interpol_coeff[64];

  for(int i = -HCX; i <= HCX; i++){
    ei[0] = i;
    for(int j = -HCY; j <= HCY; j++){
      ei[1] = j;
      for(int k = -HCZ; k <= HCZ; k++){
	ei[2] = k;

	im = ((i + HCX) * CY * CZ) + ((j + HCY) * CZ) + (k + HCZ);

	for(int d = 0; d < DIM; d++){
	  rp[d] = (double)ei[d] * DX;     //particle frame
	  sp[d] = r0[d] + rp[d];          //global frame
	  sp[d] = fmod(sp[d] + lbox[d], lbox[d]);
	  edge[d] = (int)(sp[d]/DX);
	  cp[d] = sp[d] - edge[d]*DX;

	  if(!(edge[d] >= 0 && edge[d] < Ns[d] && cp[d] >= 0)){
	    fprintf(stderr, "rp  : %f\n", rp[d]);
	    fprintf(stderr, "sp  : %f\n", sp[d]);
	    fprintf(stderr, "edge: %d\n", edge[d]);
	    fprintf(stderr, "cp  : %g\n", cp[d]);
	    assert(false);
	  }
	}

	for(int d = 0; d < DIM; d++){
	  init_interpol(interpol_pid, interpol_pval, interpol_coeff, u[d], edge);
	  post_u[d][im] = interpol(interpol_pid, interpol_pval, interpol_coeff, cp);
	  if(vflag == relative_velocity){
	    post_u[d][im] -= v0[d];
	  }
	}
      }
    }
  }
  double jax[DIM];
  rigid_body_rotation(jax, e3, q0, BODY2SPACE);
  fprintf(stderr, "# %d: B= %7.5g %7.5g %7.5g\n", 
	  frame, 
	  B1_app, B1_real, B2);
  
  binary_write(u[0], post_data);
  binary_write(u[1], post_data);
  binary_write(u[2], post_data);
}
#endif
