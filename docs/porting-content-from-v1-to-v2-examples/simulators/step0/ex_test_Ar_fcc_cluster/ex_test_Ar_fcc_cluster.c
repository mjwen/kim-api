/*
 *
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the Common Development
 * and Distribution License Version 1.0 (the "License").
 *
 * You can obtain a copy of the license at
 * http://www.opensource.org/licenses/CDDL-1.0.  See the License for the
 * specific language governing permissions and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each file and
 * include the License file in a prominent location with the name LICENSE.CDDL.
 * If applicable, add the following below this CDDL HEADER, with the fields
 * enclosed by brackets "[]" replaced with your own identifying information:
 *
 * Portions Copyright (c) [yyyy] [name of copyright owner]. All rights reserved.
 *
 * CDDL HEADER END
 *

 *
 * Copyright (c) 2013--2018, Regents of the University of Minnesota.
 * All rights reserved.
 *
 * Contributors:
 *    Ryan S. Elliott
 *    Stephen M. Whalen
 *
 */


#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "KIM_API_C.h"
#include "KIM_API_status.h"

#define NAMESTRLEN    128

#define FCCSPACING    5.260
#define DIM           3
#define NCELLSPERSIDE 2
#define NCLUSTERPARTS (4*(NCELLSPERSIDE*NCELLSPERSIDE*NCELLSPERSIDE) + \
                       6*(NCELLSPERSIDE*NCELLSPERSIDE)                 \
                       + 3*(NCELLSPERSIDE) + 1)

#define REPORT_ERROR(LN, FL, MSG, STAT) {       \
    KIM_API_report_error(LN, FL, MSG, STAT);    \
    exit(STAT);                                 \
  }

/* Define neighborlist structure */
typedef struct
{
  int iteratorId;
  int* NNeighbors;
  int* neighborList;
} NeighList;

/* Define prototypes */
void fcc_cluster_neighborlist(int half, int numberOfParticles, double* coords,
                              double cutoff, NeighList* nl);

int get_cluster_neigh(void* kimmdl, int *mode, int *request, int* part,
                      int* numnei, int** nei1part, double** Rij);

void create_FCC_cluster(double FCCspacing, int nCellsPerSide, double *coords);


/* Main program */
int main()
{
  /* Local variable declarations */
  double const MinSpacing = 0.8*FCCSPACING;
  double const MaxSpacing = 1.2*FCCSPACING;
  double const SpacingIncr = 0.025*FCCSPACING;
  double CurrentSpacing;
  double cutpad = 0.75; /* Angstroms */
  double force_norm;
  int i;
  int status;


  /* KIM variable declarations */
  void* pkim_cluster_model;
  /* model inputs */
  int numberOfParticles_cluster = NCLUSTERPARTS;
  int numberOfSpecies = 1;
  int particleSpeciesShapeCluster[1];
  int particleSpecies_cluster_model[NCLUSTERPARTS];
  int coordinatesShapeCluster[2];
  double coords_cluster[NCLUSTERPARTS][DIM];
  NeighList nl_cluster_model;
  /* model outputs */
  double cutoff_cluster_model;
  double energy_cluster_model;
  double forces_cluster[NCLUSTERPARTS*DIM];

  char testkimfile[] = "descriptor.kim";
  char modelname[NAMESTRLEN];

  /* Get KIM Model names */
  printf("Please enter valid KIM Model name: \n");
  status = scanf("%s", modelname);
  if (1 != status)
  {
    REPORT_ERROR(__LINE__, __FILE__, "Unable to read model name",
                 status);
  }

  /* initialize the model */
  status = KIM_API_file_init(&pkim_cluster_model, testkimfile, modelname);
  if (KIM_STATUS_OK > status)
    REPORT_ERROR(__LINE__, __FILE__,"KIM_API_file_init()",status);

  /* Assign shapes & register memory */
  coordinatesShapeCluster[0] = numberOfParticles_cluster;
  coordinatesShapeCluster[1] = 3;

  KIM_API_set_shape(pkim_cluster_model, "particleSpecies", particleSpeciesShapeCluster, 1, &status);
  if (KIM_STATUS_OK > status)
    REPORT_ERROR(__LINE__, __FILE__,"KIM_API_set_shape",status);
  KIM_API_set_shape(pkim_cluster_model, "coordinates", coordinatesShapeCluster, 2, &status);
  if (KIM_STATUS_OK > status) REPORT_ERROR(__LINE__, __FILE__,"KIM_API_set_shape",status);
  KIM_API_setm_data(pkim_cluster_model, &status, 8*4,
                    "numberOfParticles", 1,                             &numberOfParticles_cluster,       1,
                    "numberOfSpecies",   1,                             &numberOfSpecies,                 1,
                    "particleSpecies",   numberOfParticles_cluster,     &particleSpecies_cluster_model,   1,
                    "coordinates",       DIM*numberOfParticles_cluster, coords_cluster,                   1,
                    "neighObject",       1,                             &nl_cluster_model,                1,
                    "cutoff",            1,                             &cutoff_cluster_model,            1,
                    "energy",            1,                             &energy_cluster_model,            1,
                    "forces",            DIM*numberOfParticles_cluster, forces_cluster,                   1);
  if (KIM_STATUS_OK > status) REPORT_ERROR(__LINE__, __FILE__,"KIM_API_setm_data",status);
  status = KIM_API_set_method(pkim_cluster_model, "get_neigh", 1, (func_ptr) &get_cluster_neigh);
  if (KIM_STATUS_OK > status) REPORT_ERROR(__LINE__, __FILE__,"KIM_API_set_method",status);

  /* call model init routine */
  status = KIM_API_model_init(pkim_cluster_model);
  if (KIM_STATUS_OK > status) REPORT_ERROR(__LINE__, __FILE__,"KIM_API_model_init", status);

  /* setup particleSpecies */
  particleSpecies_cluster_model[0] = KIM_API_get_species_code(pkim_cluster_model, "Ar", &status);
  if (KIM_STATUS_OK > status) REPORT_ERROR(__LINE__, __FILE__,"get_species_code", status);
  for (i = 1; i < NCLUSTERPARTS; ++i)
    particleSpecies_cluster_model[i] = particleSpecies_cluster_model[0];

  /* setup neighbor lists */
  /* allocate memory for list */
  nl_cluster_model.NNeighbors = (int*) malloc(NCLUSTERPARTS*sizeof(int));
  if (NULL==nl_cluster_model.NNeighbors) REPORT_ERROR(__LINE__, __FILE__,"malloc unsuccessful", -1);

  nl_cluster_model.neighborList = (int*) malloc(NCLUSTERPARTS*NCLUSTERPARTS*sizeof(int));
  if (NULL==nl_cluster_model.neighborList) REPORT_ERROR(__LINE__, __FILE__,"malloc unsuccessful", -1);

  /* ready to compute */
  printf("This is Test  : ex_test_Ar_fcc_cluster\n");
  printf("--------------------------------------------------------------------------------\n");
  printf("Results for KIM Model : %s\n",   modelname);

  printf("%20s, %20s, %20s\n", "Energy", "Force Norm", "Lattice Spacing");
  for (CurrentSpacing = MinSpacing; CurrentSpacing < MaxSpacing; CurrentSpacing += SpacingIncr)
  {
    /* update coordinates for cluster */
    create_FCC_cluster(CurrentSpacing, NCELLSPERSIDE, &(coords_cluster[0][0]));
    /* compute neighbor lists */
    fcc_cluster_neighborlist(0, NCLUSTERPARTS, &(coords_cluster[0][0]),
                             (cutoff_cluster_model + cutpad), &nl_cluster_model);

    /* call compute functions */
    status = KIM_API_model_compute(pkim_cluster_model);
    if (KIM_STATUS_OK > status) REPORT_ERROR(__LINE__, __FILE__,"compute", status);

    /* compute force norm */
    force_norm = 0.0;
    for (i=0; i < DIM*numberOfParticles_cluster; ++i)
    {
      force_norm += forces_cluster[i]*forces_cluster[i];
    }
    force_norm = sqrt(force_norm);

    /* print the results */
    printf("%20.10e, %20.10e, %20.10e\n",
           energy_cluster_model,
           force_norm,
           CurrentSpacing);
  }

  /* free memory of neighbor lists */
  free(nl_cluster_model.NNeighbors);
  free(nl_cluster_model.neighborList);

  /* call model destroy */
  status = KIM_API_model_destroy(pkim_cluster_model);
  if (KIM_STATUS_OK > status) REPORT_ERROR(__LINE__, __FILE__,"destroy", status);

  /* free pkim objects */
  KIM_API_free(&pkim_cluster_model, &status);
  if (KIM_STATUS_OK > status) REPORT_ERROR(__LINE__, __FILE__,"free", status);

  /* everything is great */
  return 0;
}

void create_FCC_cluster(double FCCspacing, int nCellsPerSide, double *coords)
{
  /* local variables */
  double FCCshifts[4][DIM];
  double latVec[DIM];
  int a;
  int i;
  int j;
  int k;
  int m;
  int n;

  /* create a cubic FCC cluster of parts */
  FCCshifts[0][0] = 0.0;            FCCshifts[0][1] = 0.0;            FCCshifts[0][2] = 0.0;
  FCCshifts[1][0] = 0.5*FCCspacing; FCCshifts[1][1] = 0.5*FCCspacing; FCCshifts[1][2] = 0.0;
  FCCshifts[2][0] = 0.5*FCCspacing; FCCshifts[2][1] = 0.0;            FCCshifts[2][2] = 0.5*FCCspacing;
  FCCshifts[3][0] = 0.0;            FCCshifts[3][1] = 0.5*FCCspacing; FCCshifts[3][2] = 0.5*FCCspacing;

  a = 0;
  for (i = 0; i < nCellsPerSide; ++i)
  {
    latVec[0] = ((double) i)*FCCspacing;
    for (j = 0; j < nCellsPerSide; ++j)
    {
      latVec[1] = ((double) j)*FCCspacing;
      for (k = 0; k < nCellsPerSide; ++k)
      {
        latVec[2] = ((double) k)*FCCspacing;
        for (m = 0; m < 4; ++m)
        {
          for (n = 0; n < DIM; ++n)
          {
            coords[a*DIM + n] = latVec[n] + FCCshifts[m][n];
          }
          a++;
        }
      }
      /* add in the remaining three faces */
      /* pos-x face */
      latVec[0] = NCELLSPERSIDE*FCCspacing;
      latVec[1] = ((double) i)*FCCspacing;
      latVec[2] = ((double) j)*FCCspacing;
      for (n = 0; n < DIM; ++n)
      {
        coords[a*DIM + n] = latVec[n];
      }
      a++;
      for (n = 0; n < DIM; ++n)
      {
        coords[a*DIM + n] = latVec[n] + FCCshifts[3][n];
      }
      a++;
      /* pos-y face */
      latVec[0] = ((double) i)*FCCspacing;
      latVec[1] = NCELLSPERSIDE*FCCspacing;
      latVec[2] = ((double) j)*FCCspacing;
      for (n = 0; n < DIM; ++n)
      {
        coords[a*DIM + n] = latVec[n];
      }
      a++;
      for (n = 0; n < DIM; ++n)
      {
        coords[a*DIM + n] = latVec[n] + FCCshifts[2][n];
      }
      a++;
      /* pos-z face */
      latVec[0] = ((double) i)*FCCspacing;
      latVec[1] = ((double) j)*FCCspacing;
      latVec[2] = NCELLSPERSIDE*FCCspacing;
      for (n = 0; n < DIM; ++n)
      {
        coords[a*DIM + n] = latVec[n];
      }
      a++;
      for (n = 0; n < DIM; ++n)
      {
        coords[a*DIM + n] = latVec[n] + FCCshifts[1][n];
      }
      a++;
    }
    /* add in the remaining three edges */
    latVec[0] = ((double) i)*FCCspacing;
    latVec[1] = NCELLSPERSIDE*FCCspacing;
    latVec[2] = NCELLSPERSIDE*FCCspacing;
    for (n = 0; n < DIM; ++n)
    {
      coords[a*DIM + n] = latVec[n];
    }
    a++;
    latVec[0] = NCELLSPERSIDE*FCCspacing;
    latVec[1] = ((double) i)*FCCspacing;
    latVec[2] = NCELLSPERSIDE*FCCspacing;
    for (n = 0; n < DIM; ++n)
    {
      coords[a*DIM + n] = latVec[n];
    }
    a++;
    latVec[0] = NCELLSPERSIDE*FCCspacing;
    latVec[1] = NCELLSPERSIDE*FCCspacing;
    latVec[2] = ((double) i)*FCCspacing;
    for (n = 0; n < DIM; ++n)
    {
      coords[a*DIM + n] = latVec[n];
    }
    a++;
  }
  /* add in the remaining corner */
  for (n = 0; n < DIM; ++n)
  {
    coords[a*DIM + n] = NCELLSPERSIDE*FCCspacing;
  }
  a++;

  return;
}


void fcc_cluster_neighborlist(int half, int numberOfParticles, double* coords,
                              double cutoff, NeighList* nl)
{
  /* local variables */
  int i;
  int j;
  int k;
  int a;

  double dx[DIM];
  double r2;
  double cutoff2;

  cutoff2 = cutoff*cutoff;

  for (i = 0; i < numberOfParticles; ++i)
  {
    a = 0;
    for (j = 0; j < numberOfParticles; ++j)
    {
      r2 = 0.0;
      for (k = 0; k < DIM; ++k)
      {
        dx[k] = coords[j*DIM + k] - coords[i*DIM + k];
        r2 += dx[k]*dx[k];
      }

      if (r2 < cutoff2)
      {
        if ((half && i < j) || (!half && i != j))
        {
          /* part j is a neighbor of part i */
          (*nl).neighborList[i*NCLUSTERPARTS + a] = j;
          a++;
        }
      }
    }
    /* part i has `a' neighbors */
    (*nl).NNeighbors[i] = a;
  }

  return;
}

int get_cluster_neigh(void* kimmdl, int *mode, int *request, int* part,
                      int* numnei, int** nei1part, double** Rij)
{
  /* local variables */
  intptr_t* pkim = *((intptr_t**) kimmdl);
  int partToReturn;
  int status;
  int* numberOfParticles;
  NeighList* nl;

  /* initialize numnei */
  *numnei = 0;

  /* unpack neighbor list object */
  numberOfParticles = (int*) KIM_API_get_data(pkim, "numberOfParticles", &status);
  if (KIM_STATUS_OK > status)
  {
    KIM_API_report_error(__LINE__, __FILE__,"get_data", status);
    return status;
  }

  nl = (NeighList*) KIM_API_get_data(pkim, "neighObject", &status);
  if (KIM_STATUS_OK > status)
  {
    KIM_API_report_error(__LINE__, __FILE__,"get_data", status);
    return status;
  }

  /* check mode and request */
  if (0 == *mode) /* iterator mode */
  {
    if (0 == *request) /* reset iterator */
    {
      (*nl).iteratorId = -1;
      return KIM_STATUS_NEIGH_ITER_INIT_OK;
    }
    else if (1 == *request) /* increment iterator */
    {
      (*nl).iteratorId++;
      if ((*nl).iteratorId >= *numberOfParticles)
      {
        return KIM_STATUS_NEIGH_ITER_PAST_END;
      }
      else
      {
        partToReturn = (*nl).iteratorId;
      }
    }
    else /* invalid request value */
    {
      KIM_API_report_error(__LINE__, __FILE__,"Invalid request in get_cluster_neigh", KIM_STATUS_NEIGH_INVALID_REQUEST);
      return KIM_STATUS_NEIGH_INVALID_REQUEST;
    }
  }
  else if (1 == *mode) /* locator mode */
  {
    if ((*request >= *numberOfParticles) || (*request < 0)) /* invalid id */
    {
      KIM_API_report_error(__LINE__, __FILE__,"Invalid part ID in get_cluster_neigh", KIM_STATUS_PARTICLE_INVALID_ID);
      return KIM_STATUS_PARTICLE_INVALID_ID;
    }
    else
    {
      partToReturn = *request;
    }
  }
  else /* invalid mode */
  {
    KIM_API_report_error(__LINE__, __FILE__,"Invalid mode in get_cluster_neigh", KIM_STATUS_NEIGH_INVALID_MODE);
    return KIM_STATUS_NEIGH_INVALID_MODE;
  }

  /* set the returned part */
  *part = partToReturn;

  /* set the returned number of neighbors for the returned part */
  *numnei = (*nl).NNeighbors[*part];

  /* set the location for the returned neighbor list */
  *nei1part = &((*nl).neighborList[(*part)*NCLUSTERPARTS]);

  /* set the pointer to Rij to appropriate value */
  *Rij = 0;

  return KIM_STATUS_OK;
}
