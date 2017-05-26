//
//
// CDDL HEADER START
//
// The contents of this file are subject to the terms of the Common Development
// and Distribution License Version 1.0 (the "License").
//
// You can obtain a copy of the license at
// http://www.opensource.org/licenses/CDDL-1.0.  See the License for the
// specific language governing permissions and limitations under the License.
//
// When distributing Covered Code, include this CDDL HEADER in each file and
// include the License file in a prominent location with the name LICENSE.CDDL.
// If applicable, add the following below this CDDL HEADER, with the fields
// enclosed by brackets "[]" replaced with your own identifying information:
//
// Portions Copyright (c) [yyyy] [name of copyright owner]. All rights reserved.
//
// CDDL HEADER END
//
//
//
// Copyright (c) 2013--2017, Regents of the University of Minnesota.
// All rights reserved.
//
// Contributors:
//    Ryan S. Elliott
//    Stephen M. Whalen
//
//

//
// Release: This file is part of the kim-api.git repository.
//

#include <stdlib.h>
#include <iomanip>
#include <string>
#include "KIM_SpeciesName.hpp"
#include "KIM_Model.hpp"
#include "KIM_Compute.hpp"
#include "KIM_UTILITY_Compute.hpp"
#include "KIM_Logger.hpp"

#define NAMESTRLEN    128

#define FCCSPACING    5.260
#define DIM           3
#define NCELLSPERSIDE 2
#define NCLUSTERPARTS (4*(NCELLSPERSIDE*NCELLSPERSIDE*NCELLSPERSIDE) + \
                       6*(NCELLSPERSIDE*NCELLSPERSIDE)                 \
                       + 3*(NCELLSPERSIDE) + 1)

#define REPORT_ERROR(LN, FL, MSG, STAT) {                 \
    KIM::report_error(LN, FL, MSG, STAT);                 \
    exit(STAT);                                           \
  }

/* Define neighborlist structure */
typedef struct
{
  int iteratorId;
  int* NNeighbors;
  int* neighborList;
} NeighList;

/* Define prototypes */
char const * const descriptor();
void fcc_cluster_neighborlist(int half, int numberOfParticles, double* coords,
                              double cutoff, NeighList* nl);

int get_cluster_neigh(KIM::Model const * const model,
                      int const particleNumber, int * const numberOfNeighbors,
                      int const ** const neighborsOfParticle);

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
  int i;
  int status;


  /* model inputs */
  int numberOfParticles_cluster = NCLUSTERPARTS;
  int numberOfSpecies = 1;
  int particleSpecies_cluster_model[NCLUSTERPARTS];
  double coords_cluster[NCLUSTERPARTS][DIM];
  NeighList nl_cluster_model;
  /* model outputs */
  double cutoff_cluster_model = -15;
  double energy_cluster_model;

  std::string modelname;

  /* Get KIM Model names */
  printf("Please enter valid KIM Model name: \n");
  std::cin >> modelname;


  /* initialize the model */
  KIM::Model * kim_cluster_model;
  status = KIM::Model::create(descriptor(),modelname, &kim_cluster_model);
  if (KIM_STATUS_OK > status)
    REPORT_ERROR(__LINE__, __FILE__,"KIM_create_model_interface()",status);

  status = KIM::UTILITY::COMPUTE::setm_data(
      kim_cluster_model,
      KIM::COMPUTE::ARGUMENT_NAME::numberOfParticles, 1,                       &numberOfParticles_cluster,       1,
      KIM::COMPUTE::ARGUMENT_NAME::numberOfSpecies, 1,                         &numberOfSpecies,                 1,
      KIM::COMPUTE::ARGUMENT_NAME::particleSpecies, numberOfParticles_cluster, particleSpecies_cluster_model,   1,
      KIM::COMPUTE::ARGUMENT_NAME::coordinates, DIM*numberOfParticles_cluster, coords_cluster,                   1,
      KIM::COMPUTE::ARGUMENT_NAME::neighObject, 1,                             &nl_cluster_model,                1,
      KIM::COMPUTE::ARGUMENT_NAME::cutoff, 1,                                  &cutoff_cluster_model,            1,
      KIM::COMPUTE::ARGUMENT_NAME::energy, 1,                                  &energy_cluster_model,            1,
      KIM::COMPUTE::ARGUMENT_NAME::End);
  if (KIM_STATUS_OK > status) REPORT_ERROR(__LINE__, __FILE__,"KIM_API_setm_data",status);
  status = kim_cluster_model->set_method(KIM::COMPUTE::ARGUMENT_NAME::get_neigh, 1, KIM::COMPUTE::LANGUAGE_NAME::Cpp, (KIM::func *) &get_cluster_neigh);
  if (KIM_STATUS_OK > status) REPORT_ERROR(__LINE__, __FILE__,"KIM_API_set_method",status);

  /* call model init routine */
  status = kim_cluster_model->init();
  if (KIM_STATUS_OK > status) REPORT_ERROR(__LINE__, __FILE__,"KIM_API_model_init", status);

  /* setup particleSpecies */
  status =  kim_cluster_model->get_species_code(KIM::SPECIES_NAME::Ar,
                                                &(particleSpecies_cluster_model[0]));
  if (KIM_STATUS_OK > status) REPORT_ERROR(__LINE__, __FILE__,"get_species_code", status);
  for (i = 1; i < NCLUSTERPARTS; ++i)
    particleSpecies_cluster_model[i] = particleSpecies_cluster_model[0];
  /* setup neighbor lists */
  /* allocate memory for list */
  nl_cluster_model.NNeighbors = new int[NCLUSTERPARTS];
  if (NULL==nl_cluster_model.NNeighbors) REPORT_ERROR(__LINE__, __FILE__,"new unsuccessful", -1);

  nl_cluster_model.neighborList = new int[NCLUSTERPARTS*NCLUSTERPARTS];
  if (NULL==nl_cluster_model.neighborList) REPORT_ERROR(__LINE__, __FILE__,"new unsuccessful", -1);

  /* ready to compute */
  std::cout << std::setiosflags(std::ios::scientific) << std::setprecision(10);
  std::cout << "--------------------------------------------------------------------------------\n";
  std::cout << "This is Test  : ex_test_Ar_fcc_cluster\n";
  std::cout << "MODEL is : " << modelname << std::endl;

  for (CurrentSpacing = MinSpacing; CurrentSpacing < MaxSpacing; CurrentSpacing += SpacingIncr)
  {
    /* update coordinates for cluster */
    create_FCC_cluster(CurrentSpacing, NCELLSPERSIDE, &(coords_cluster[0][0]));
    /* compute neighbor lists */
    fcc_cluster_neighborlist(0, NCLUSTERPARTS, &(coords_cluster[0][0]),
                             (cutoff_cluster_model + cutpad), &nl_cluster_model);

    /* call compute functions */
    status = kim_cluster_model->compute();
    if (KIM_STATUS_OK > status) REPORT_ERROR(__LINE__, __FILE__,"compute", status);

    /* print the results */
    std::cout << "Energy for " << NCLUSTERPARTS << " parts = "
              << std::setw(20) << energy_cluster_model
              << std::setw(20) << CurrentSpacing
              << std::endl;
  }


  /* call model destroy */
  status = kim_cluster_model->destroy_model();
  if (KIM_STATUS_OK > status) REPORT_ERROR(__LINE__, __FILE__,"destroy", status);

  KIM::Model::destroy(&kim_cluster_model);

  /* free memory of neighbor lists */
  delete [] nl_cluster_model.NNeighbors;
  delete [] nl_cluster_model.neighborList;

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

int get_cluster_neigh(KIM::Model const * const model,
                      int const particleNumber, int * const numberOfNeighbors,
                      int const ** const neighborsOfParticle)
{
  /* local variables */
  int status;
  int* numberOfParticles;
  NeighList* nl;

  /* initialize numNeigh */
  *numberOfNeighbors = 0;

  /* unpack neighbor list object */
  status = model->get_data(KIM::COMPUTE::ARGUMENT_NAME::numberOfParticles, (void **) &numberOfParticles);
  if (KIM_STATUS_OK > status)
  {
    KIM::report_error(__LINE__, __FILE__,"get_data", status);
    return status;
  }

  status = model->get_data(KIM::COMPUTE::ARGUMENT_NAME::neighObject, (void **) &nl);
  if (KIM_STATUS_OK > status)
  {
    KIM::report_error(__LINE__, __FILE__,"get_data", status);
    return status;
  }

  if ((particleNumber >= *numberOfParticles) || (particleNumber < 0)) /* invalid id */
  {
    KIM::report_error(__LINE__, __FILE__,"Invalid part ID in get_cluster_neigh", KIM_STATUS_PARTICLE_INVALID_ID);
    return KIM_STATUS_PARTICLE_INVALID_ID;
  }

  /* set the returned number of neighbors for the returned part */
  *numberOfNeighbors = (*nl).NNeighbors[particleNumber];

  /* set the location for the returned neighbor list */
  *neighborsOfParticle = &((*nl).neighborList[(particleNumber)*NCLUSTERPARTS]);

  return KIM_STATUS_OK;
}

char const * const descriptor()
{
  return
      "KIM_API_Version := 1.6.0\n"
      "Unit_length := A\n"
      "Unit_energy := eV\n"
      "Unit_charge := e\n"
      "Unit_temperature := K\n"
      "Unit_time := ps\n"
      "\n"
      "PARTICLE_SPECIES:\n"
      "Ar spec 1\n"
      "\n"
      "CONVENTIONS:\n"
      "ZeroBasedLists flag\n"
      "\n"
      "MODEL_INPUT:\n"
      "numberOfParticles integer none\n"
      "numberOfSpecies integer none\n"
      "particleSpecies integer none\n"
      "coordinates double length\n"
      "get_neigh method none\n"
      "neighObject pointer none\n"
      "\n"
      "MODEL_OUTPUT:\n"
      "destroy method none\n"
      "compute method none\n"
      "reinit method none\n"
      "cutoff double length\n"
      "energy double energy\n";
}
