#ifndef FLT_MAX
#define FLT_MAX 3.40282347e+38
#endif

__kernel void kmeans_kernel_c(__global float  *feature,   
  __global float *clusters,
  __global int *membership,
  int npoints,
  int nclusters,
  int nfeatures,
  int offset,
  int size,
 __global int *lockBuffer, __global unsigned int *errorBuffer, __global unsigned int *addressBuffer,  __global unsigned long *valueBuffer, __global unsigned int *groupID) {
  unsigned int point_id = get_global_id(0);
  int index = 0;

  if (point_id < npoints)
  {
    float min_dist=FLT_MAX;
    for (int i=0; i < nclusters; i++) {
      float dist = 0;
      float ans  = 0;
      for (int l=0; l<nfeatures; l++) {
        ans += (feature[l * npoints + point_id]-clusters[i*nfeatures+l])* 
          (feature[l * npoints + point_id]-clusters[i*nfeatures+l]);
      }

      dist = ans;
      if (dist < min_dist) {
        min_dist = dist;
        index    = i;
      }
    }
    membership[point_id] = index;
  }        

  return;
}


// hint: 2D transpose
__kernel void kmeans_swap(__global float  *feature,   
  __global float  *feature_swap,
  int     npoints,
  int     nfeatures,
  __global int *lockBuffer, __global unsigned int *errorBuffer, __global unsigned int *addressBuffer,  __global unsigned long *valueBuffer, __global unsigned int *groupID) {
  unsigned int tid = get_global_id(0);
  for(int i = 0; i < nfeatures; i++)
    feature_swap[i * npoints + tid] = feature[tid * nfeatures + i];
} 
