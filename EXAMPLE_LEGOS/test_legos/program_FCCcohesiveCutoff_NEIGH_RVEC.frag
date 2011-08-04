!*******************************************************************************
!**
!**  PROGRAM TEST_NAME_STR
!**
!**  KIM compliant program to find (using the Golden section search algorithm)
!**  the minimum energy of one atom in a periodic FCC crystal (spec="SPECIES_NAME_STR") as a 
!**  function of lattice spacing.
!**
!**  Works with the following NBC methods:
!**        NEIGH-RVEC-F
!**
!**  Authors: Valeriu Smirichinski, Ryan S. Elliott, Ellad B. Tadmor
!**
!**  Copyright 2011 Ellad B. Tadmor, Ryan S. Elliott, and James P. Sethna
!**  All rights reserved.
!**
!*******************************************************************************


!-------------------------------------------------------------------------------
!
! Main program
!
!-------------------------------------------------------------------------------
program TEST_NAME_STR
  use KIMservice
  implicit none

!============================== VARIABLE DEFINITIONS ==========================

  ! parameters controlling behavior of test
  !
  character(len=80),        parameter :: testname    = "TEST_NAME_STR"
  character(len=2),         parameter :: specname    = 'SPECIES_NAME_STR'
  double precision,         parameter :: TOL         = 1.0d-8
  double precision,         parameter :: FCCspacing  = FCC_SPACING_STR
  double precision,         parameter :: MinSpacing  = 0.800d0*FCCspacing
  double precision,         parameter :: MaxSpacing  = 1.200d0*FCCspacing
  integer,                  parameter :: DIM         = 3

  ! significant local variables
  !
  integer,          allocatable :: NLRvecLocs(:)     ! neighbor list pointers
  integer,          allocatable :: neighborList(:,:) ! neighbor list storage
  double precision, allocatable :: RijList(:,:,:)    ! Rij vector list storage

  integer              :: NNeighbors         ! maximum number of neighbors for an atom

  double precision     :: FinalSpacing       ! crystal lattice parameter

  double precision     :: FinalEnergy        ! energy per atom of crystal 
                                             ! at current spacing

  integer              :: CellsPerCutoff     ! number of unit cells along
                                             ! box (of size cutoff) side

  double precision     :: MaxCutoff          ! maximum value for cutoff radius


  ! KIM variables
  !
  character(len=80)         :: modelname     ! KIM-compliant model name

  integer(kind=kim_intptr)  :: pkim          ! pointer to KIM API object

  integer                   :: ier           ! error flag

  real*8 coordum(DIM,1);   pointer(pcoor,coordum)         ! coordinate
  real*8, pointer  :: coords(:,:)

  double precision param_cutoff; pointer(pparam_cutoff,param_cutoff) ! parameter cutoff radius

  integer(kind=kim_intptr) :: N                           ! number of atoms


  ! other variables
  !
  double precision, external  ::  get_model_cutoff_firsttime
  integer,          external  ::  check_model_parameters

!========================= END VARIABLE DEFINITIONS ==========================


  ! Read in KIM Model name to use
  !
  print *, "Please enter a valid KIM model name: "
  read(*,*) modelname


  ! We'll use just one atom for this calculation!
  !
  N = 1


  ! Setup the KIM API object
  !
  call setup_KIM_API_object(pkim, testname, modelname, N, specname)


  ! check for PARAM_FREE_cutoff
  ier = check_model_parameters(pkim)
  if (ier.ne.1) then
     ! PARAM_FREE_cutoff is not provided by the Model
     call report_error(__LINE__, "exiting...", ier);
     stop
  endif
  !
  ! access the PARAM_FREE_cutoff parameter
  !
  pparam_cutoff = kim_api_get_data_f(pkim, "PARAM_FREE_cutoff", ier)
  if (ier.le.0) then
     call report_error(__LINE__, "kim_api_get_data_f", ier)
     stop
  endif
  
  ! Set MaxCutoff to be 2.0 more than the Model's normal cutoff
  !
  MaxCutoff = param_cutoff + 2.0d0


  ! Set up for first iteration of the loop over the cutoff radius
  param_cutoff = param_cutoff - 2.0d0
  ier = kim_api_model_reinit_f(pkim)
  if (ier.le.0) then
     call report_error(__LINE__, "kim_api_model_reinit_f", ier)
     stop
  endif

  ! allocate storage for neighbor lists
  ! and store necessary pointers in KIM API object
  !
  
  ! determine maximum number of neighbors we will need
  !
  CellsPerCutoff = ceiling(param_cutoff/MinSpacing+ 0.05d0) ! the 0.05 is a saftey factor
  NNeighbors = 4*((2*CellsPerCutoff + 1)**3)
  !
  ! allocate memory for the neighbor list and Rij vectors
  !
  allocate(neighborList(NNeighbors+1,N))
  allocate(RijList(3,NNeighbors+1,N))
  allocate(NLRvecLocs(3))
  NLRvecLocs(1) = loc(neighborList)
  NLRvecLocs(2) = loc(RijList)
  NLRvecLocs(3) = NNeighbors+1
  call setup_neighborlist_Rij_KIM_access(pkim, NLRvecLocs)
  ! 
  
  ! loop over an increasing cutoff radius
  do while (param_cutoff .le. MaxCutoff)
     !
     ! find equilibrium spacing by minimizing coheseive energy with respect
     ! to the periodic box size for the current cutoff value
     !
     call NEIGH_RVEC_compute_equilibrium_spacing(pkim, &
          DIM,CellsPerCutoff,MinSpacing,MaxSpacing,    &
          TOL,N,NNeighbors,neighborlist,RijList,       &
          .false.,FinalSpacing,FinalEnergy)
     
     ! print results to screen
     !
     print '(80(''*''))'
     print *, "Results for KIM Model: ", modelname
     print *,
     print *,"Found minimum energy configuration to within", TOL
     print *,
     print *,"cutoff = ", param_cutoff
     print *,
     print *,"Energy/atom = ", FinalEnergy, "; Spacing = ", FinalSpacing
     print '(80(''*''))'

     !
     ! increment cutoff
     !
     param_cutoff = param_cutoff + 1.0
     ier = kim_api_model_reinit_f(pkim)
     if (ier.le.0) then
        call report_error(__LINE__, "kim_api_model_reinit_f", ier)
        stop
     endif

  enddo

  ! Don't forget to free and/or deallocate
  !
  deallocate(neighborList)
  deallocate(RijList)
  deallocate(NLRvecLocs)
  call free_KIM_API_object(pkim)

  stop

end program TEST_NAME_STR

!-------------------------------------------------------------------------------
!
! check_model_parameters : 
!
!    Scan the Model's parameters and return 1 in PARAM_FREE_cutoff is in the
!    list and 0 if it is not in the list
!
!-------------------------------------------------------------------------------
integer function check_model_parameters(pkim)
  use KIMservice
  implicit none

  !-- Transferred variables
  integer(kind=kim_intptr), intent(in) :: pkim

  !-- Local variables
  character(len=KEY_CHAR_LENGTH) :: listOfParameters(1); pointer(plistOfParameters,listOfParameters)
  integer nParams
  integer paramIndex
  integer i
  integer ier

  plistOfParameters = kim_api_get_listparams_f(pkim, nParams, ier)
  paramIndex = 0
  print *,"The model has defined the following parameters:"
  do i=1,nParams
     print *, i, listOfParameters(i)
     if (index(listOfParameters(i),"PARAM_FREE_cutoff").eq.1) then
        paramIndex = i
     endif
  enddo
  call free(plistOfParameters) ! deallocate memory

  if (paramIndex .gt. 0) then
     print *,"PARAM_FREE_cutoff IS in the list, at index", paramIndex
     check_model_parameters = 1
  else
     print *,"PARAM_FREE_cutoff is NOT in the parameter list."
     check_model_parameters = 0
  endif

  return

end function check_model_parameters