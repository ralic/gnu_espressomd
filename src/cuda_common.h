

#ifdef __cplusplus
extern "C" {
#endif

#include <cuda.h>
#include <cuda_runtime.h>

#include <stdio.h>
#include "config.h" //this is required so that the ifdefs are actually defined


#ifndef CUDA_COMMON_H
#define CUDA_COMMON_H

/** Action number for \ref mpi_get_particles. */
#define REQ_GETPARTS  16

/**cuda streams for parallel computing on cpu and gpu */
extern cudaStream_t stream[1];

extern cudaError_t err;
extern cudaError_t _err;

/** data which must be copied from the GPU at each step run on the GPU */
typedef struct {
  /** force on the particle given to md part */
  float f[3];

} CUDA_particle_force;

/** data structure which must be copied to the GPU at each step run on the GPU */
typedef struct {
  /** particle position given from md part*/
  float p[3];
  /** particle momentum struct velocity p.m->v*/
  float v[3];
#ifdef LB_ELECTROHYDRODYNAMICS
  float mu_E[3];
#endif
#ifdef ELECTROSTATICS
  float q;
#endif
  unsigned int fixed;

} CUDA_particle_data;


/** Note the particle's seed gets its own struct since it doesn't get copied back and forth from the GPU */
typedef struct {

  unsigned int seed;

} CUDA_particle_seed;

extern CUDA_particle_data *particle_data_host;


/** This structure contains global variables associated with all of the particles and not with one individual particle */
typedef struct {
  
  /**  This is for seeding the particles' individual seeds and is initialized using irandom, beware if using for other purposes */
  unsigned int seed;
  
  unsigned int number_of_particles; 
  
  /** a boolean variable to indicate if particle info should be communicated between the cpu and gpu */
  unsigned int communication_enabled;
} CUDA_global_part_vars;


void copy_forces_from_GPU();
CUDA_global_part_vars* gpu_get_global_particle_vars_pointer();
CUDA_particle_data* gpu_get_particle_pointer();
CUDA_particle_force* gpu_get_particle_force_pointer();
CUDA_particle_seed* gpu_get_particle_seed_pointer();
void gpu_init_particle_comm();
void cuda_mpi_get_particles(CUDA_particle_data *host_result);
void copy_part_data_to_gpu();
void cuda_mpi_send_forces(CUDA_particle_force *host_forces);


/**erroroutput for memory allocation and memory copy 
 * @param err cuda error code
 * @param *file .cu file were the error took place
 * @param line line of the file were the error took place
*/

static void _cuda_safe_mem(cudaError_t err, char *file, unsigned int line){
    if( cudaSuccess != err) {                                             
      fprintf(stderr, "Cuda Memory error at %s:%u.\n", file, line);
      printf("CUDA error: %s\n", cudaGetErrorString(err));
      if ( err == cudaErrorInvalidValue )
        fprintf(stderr, "You may have tried to allocate zero memory at %s:%u.\n", file, line);
      exit(EXIT_FAILURE);
    } else {
      _err=cudaGetLastError();
      if (_err != cudaSuccess) {
        fprintf(stderr, "Error found during memory operation. Possibly however from an failed operation before. %s:%u.\n", file, line);
        printf("CUDA error: %s\n", cudaGetErrorString(err));
        if ( _err == cudaErrorInvalidValue )
          fprintf(stderr, "You may have tried to allocate zero memory before %s:%u.\n", file, line);
        exit(EXIT_FAILURE);
      }
    }
}

#define cuda_safe_mem(a) _cuda_safe_mem((a), __FILE__, __LINE__)


#define KERNELCALL(_f, _a, _b, _params) \
_f<<<_a, _b, 0, stream[0]>>>_params; \
_err=cudaGetLastError(); \
if (_err!=cudaSuccess){ \
  printf("CUDA error: %s\n", cudaGetErrorString(_err)); \
  fprintf(stderr, "error calling %s with dim %d %d %d #thpb %d in %s:%u\n", #_f, _a.x, _a.y, _a.z, _b, __FILE__, __LINE__); \
  exit(EXIT_FAILURE); \
}



#endif /* ifdef CUDA_COMMON_H */

#ifdef __cplusplus
}
#endif