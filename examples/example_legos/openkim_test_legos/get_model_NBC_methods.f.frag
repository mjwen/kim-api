!-------------------------------------------------------------------------------
!
!  Get the number and identities of NBCs supported by the Model.
!
!-------------------------------------------------------------------------------
subroutine Get_Model_NBC_methods(modelname, max_NBCs, model_NBCs, num_NBCs, ier)
use, intrinsic :: iso_c_binding
use KIM_API_F03
implicit none

!-- Transferred variables
character(len=KIM_KEY_STRING_LENGTH), intent(in)   :: modelname
integer(c_int),                       intent(in)   :: max_NBCs
character(len=KIM_KEY_STRING_LENGTH), intent(out)  :: model_NBCs(max_NBCs)
integer(c_int),                       intent(out)  :: num_NBCs
integer(c_int),                       intent(out)  :: ier

!-- Local variables
integer(c_int) :: index

!-- KIM variables
type(c_ptr) pkim

! Initialize error flag
ier = KIM_STATUS_OK

! Generate empty KIM object containing generic model info
ier = kim_api_model_info(pkim, modelname)
if (ier.lt.KIM_STATUS_OK) return

! Identify supported NBCs by seeking index to each of the NBCs in the KIM model
num_NBCs = 0

! NEIGH_RVEC_H
index = kim_api_get_index(pkim, "NEIGH_RVEC_H", ier)
if (index.ge.0) then
   num_NBCs = num_NBCs + 1
   if (num_NBCs>max_NBCs) then
      ier = KIM_STATUS_FAIL
      return
   endif
   model_NBCs(num_NBCs) = "NEIGH_RVEC_H"
endif

! NEIGH_PURE_H
index = kim_api_get_index(pkim, "NEIGH_PURE_H", ier)
if (index.ge.0) then
   num_NBCs = num_NBCs + 1
   if (num_NBCs>max_NBCs) then
      ier = KIM_STATUS_FAIL
      return
   endif
   model_NBCs(num_NBCs) = "NEIGH_PURE_H"
endif

! NEIGH_RVEC_F
index = kim_api_get_index(pkim, "NEIGH_RVEC_F", ier)
if (index.ge.0) then
   num_NBCs = num_NBCs + 1
   if (num_NBCs>max_NBCs) then
      ier = KIM_STATUS_FAIL
      return
   endif
   model_NBCs(num_NBCs) = "NEIGH_RVEC_F"
endif

! NEIGH_PURE_F
index = kim_api_get_index(pkim, "NEIGH_PURE_F", ier)
if (index.ge.0) then
   num_NBCs = num_NBCs + 1
   if (num_NBCs>max_NBCs) then
      ier = KIM_STATUS_FAIL
      return
   endif
   model_NBCs(num_NBCs) = "NEIGH_PURE_F"
endif

! MI_OPBC_H
index = kim_api_get_index(pkim, "MI_OPBC_H", ier)
if (index.ge.0) then
   num_NBCs = num_NBCs + 1
   if (num_NBCs>max_NBCs) then
      ier = KIM_STATUS_FAIL
      return
   endif
   model_NBCs(num_NBCs) = "MI_OPBC_H"
endif

! MI_OPBC_F
index = kim_api_get_index(pkim, "MI_OPBC_F", ier)
if (index.ge.0) then
   num_NBCs = num_NBCs + 1
   if (num_NBCs>max_NBCs) then
      ier = KIM_STATUS_FAIL
      return
   endif
   model_NBCs(num_NBCs) = "MI_OPBC_F"
endif

! CLUSTER
index = kim_api_get_index(pkim, "CLUSTER", ier)
if (index.ge.0) then
   num_NBCs = num_NBCs + 1
   if (num_NBCs>max_NBCs) then
      ier = KIM_STATUS_FAIL
      return
   endif
   model_NBCs(num_NBCs) = "CLUSTER"
endif

! free temporary storage
call kim_api_free(pkim,ier)

return

end subroutine Get_Model_NBC_methods
