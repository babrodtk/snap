! SNAP: Servere Nuclear Accident Programme
! Copyright (C) 1992-2017   Norwegian Meteorological Institute

! This file is part of SNAP. SNAP is free software: you can
! redistribute it and/or modify it under the terms of the
! GNU General Public License as published by the
! Free Software Foundation, either version 3 of the License, or
! (at your option) any later version.

! This program is distributed in the hope that it will be useful,
! but WITHOUT ANY WARRANTY; without even the implied warranty of
! MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
! GNU General Public License for more details.

! You should have received a copy of the GNU General Public License
! along with this program.  If not, see <https://www.gnu.org/licenses/>.

module ftestML
  implicit none
  private

  public ftest

  contains

!> Purpose: Test field, print min,mean,max values.
subroutine ftest(name,k1,k2,nx,ny,nk,field,iundef)
  use iso_fortran_env, only: real64
  USE snapdebug, only: iulog

  implicit none

  integer, value :: k1, k2
  integer, intent(in) :: nx, ny, nk, iundef
  real, intent(in) :: field(nx,ny,nk)
  character(len=*), intent(in) :: name

  integer :: kstep,i,j,k,ndef
  real :: fmin,fmax,fmean
  real, parameter :: undef = 1.0e35
  real, parameter :: ud = undef*0.9

  real(real64) :: fsum

  if(k1 < 1 .OR. k1 > nk) k1=1
  if(k2 < 1 .OR. k2 > nk) k2=nk
  kstep=+1
  if(k1 > k2) kstep=-1

  do k=k1,k2,kstep
    fmin = huge(fmin)
    fmax = -huge(fmax)
    fsum=0.
    if(iundef == 0) then
      fmin = minval(field(:,:,k))
      fmax = maxval(field(:,:,k))
      fsum = sum(field(:,:,k))
      ndef=nx*ny
    else
      ndef=0
    ! OMP PARALLEL DO PRIVATE(j,i) REDUCTION(max : fmax)
    ! OMP&            REDUCTION(min : fmin) REDUCTION( + : fsum, ndef)
    ! OMP&            COLLAPSE(2)
      do j=1,ny
        do i=1,nx
          if(field(i,j,k) < ud) then
            fmin=min(fmin,field(i,j,k))
            fmax=max(fmax,field(i,j,k))
            fsum=fsum+field(i,j,k)
            ndef=ndef+1
          end if
        end do
      end do
    ! OMP END PARALLEL DO
    end if
    if(ndef > 0) then
      fmean=fsum/dble(ndef)
    else
      fmin=0.
      fmax=0.
      fmean=0.
    end if
    if(k1 /= k2) then
      write(iulog,fmt='(5x,a8,1x,i3,3(1x,e13.5))') &
      name,k,fmin,fmean,fmax
    else
      write(iulog,fmt='(5x,a8,1x,3x,3(1x,e13.5))') &
      name,  fmin,fmean,fmax
    end if
  end do
  flush(iulog)

  return
end subroutine ftest
end module ftestML
