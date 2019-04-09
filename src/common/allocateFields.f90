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

module allocateFieldsML
  implicit none
  private

  public allocateFields, deAllocateFields

  contains

subroutine allocateFields
  USE particleML
  USE fileInfoML
  USE snapdimML, only: nx, ny, nk, ldata, maxsiz, mprecip, nxmc, nymc
  USE snapparML, only: ncomp, mpart, mplume, icomp, iparnum, iplume
  USE snapfldML
  USE snapfilML
  USE snapgrdML
  implicit none

  logical, save :: FirstCall = .TRUE.
  integer :: AllocateStatus
  character(len=*), parameter :: errmsg = "*** Not enough memory ***"

  if ( .NOT. FirstCall) return
  FirstCall = .FALSE.

  ALLOCATE ( alevel(nk), STAT = AllocateStatus)
  IF (AllocateStatus /= 0) STOP errmsg
  ALLOCATE ( blevel(nk), STAT = AllocateStatus)
  IF (AllocateStatus /= 0) STOP errmsg
  ALLOCATE ( vlevel(nk), STAT = AllocateStatus)
  IF (AllocateStatus /= 0) STOP errmsg
  ALLOCATE ( ahalf(nk), STAT = AllocateStatus)
  IF (AllocateStatus /= 0) STOP errmsg
  ALLOCATE ( bhalf(nk), STAT = AllocateStatus)
  IF (AllocateStatus /= 0) STOP errmsg
  ALLOCATE ( vhalf(nk), STAT = AllocateStatus)
  IF (AllocateStatus /= 0) STOP errmsg


  ALLOCATE ( u1(nx,ny,nk), STAT = AllocateStatus)
  IF (AllocateStatus /= 0) STOP errmsg
  ALLOCATE ( v1(nx,ny,nk), STAT = AllocateStatus)
  IF (AllocateStatus /= 0) STOP errmsg
  ALLOCATE ( w1(nx,ny,nk), STAT = AllocateStatus)
  IF (AllocateStatus /= 0) STOP errmsg
  ALLOCATE ( t1(nx,ny,nk), STAT = AllocateStatus)
  IF (AllocateStatus /= 0) STOP errmsg
  ALLOCATE ( ps1(nx,ny), STAT = AllocateStatus)
  IF (AllocateStatus /= 0) STOP errmsg
  ALLOCATE ( bl1(nx,ny), STAT = AllocateStatus)
  IF (AllocateStatus /= 0) STOP errmsg
  ALLOCATE ( hbl1(nx,ny), STAT = AllocateStatus)
  IF (AllocateStatus /= 0) STOP errmsg
  ALLOCATE ( hlevel1(nx,ny,nk), STAT = AllocateStatus)
  IF (AllocateStatus /= 0) STOP errmsg
  ALLOCATE ( hlayer1(nx,ny,nk), STAT = AllocateStatus)

  ALLOCATE ( u2(nx,ny,nk), STAT = AllocateStatus)
  IF (AllocateStatus /= 0) STOP errmsg
  ALLOCATE ( v2(nx,ny,nk), STAT = AllocateStatus)
  IF (AllocateStatus /= 0) STOP errmsg
  ALLOCATE ( w2(nx,ny,nk), STAT = AllocateStatus)
  IF (AllocateStatus /= 0) STOP errmsg
  ALLOCATE ( t2(nx,ny,nk), STAT = AllocateStatus)
  IF (AllocateStatus /= 0) STOP errmsg
  ALLOCATE ( ps2(nx,ny), STAT = AllocateStatus)
  IF (AllocateStatus /= 0) STOP errmsg
  ALLOCATE ( bl2(nx,ny), STAT = AllocateStatus)
  IF (AllocateStatus /= 0) STOP errmsg
  ALLOCATE ( hbl2(nx,ny), STAT = AllocateStatus)
  IF (AllocateStatus /= 0) STOP errmsg
  ALLOCATE ( hlevel2(nx,ny,nk), STAT = AllocateStatus)
  IF (AllocateStatus /= 0) STOP errmsg
  ALLOCATE ( hlayer2(nx,ny,nk), STAT = AllocateStatus)

  ALLOCATE ( idata(ldata), STAT = AllocateStatus)
  IF (AllocateStatus /= 0) STOP errmsg
  ALLOCATE ( fdata(maxsiz), STAT = AllocateStatus)
  IF (AllocateStatus /= 0) STOP errmsg

  ALLOCATE ( xm(nx,ny), STAT = AllocateStatus)
  IF (AllocateStatus /= 0) STOP errmsg
  ALLOCATE ( ym(nx,ny), STAT = AllocateStatus)
  IF (AllocateStatus /= 0) STOP errmsg
  ALLOCATE ( garea(nx,ny), STAT = AllocateStatus)
  IF (AllocateStatus /= 0) STOP errmsg
  ALLOCATE ( field1(nx,ny), STAT = AllocateStatus)
  IF (AllocateStatus /= 0) STOP errmsg
  ALLOCATE ( field2(nx,ny), STAT = AllocateStatus)
  IF (AllocateStatus /= 0) STOP errmsg
  ALLOCATE ( field3(nx,ny), STAT = AllocateStatus)
  IF (AllocateStatus /= 0) STOP errmsg
  ALLOCATE ( field4(nx,ny), STAT = AllocateStatus)
  IF (AllocateStatus /= 0) STOP errmsg

  ALLOCATE ( pmsl1(nx,ny), STAT = AllocateStatus)
  IF (AllocateStatus /= 0) STOP errmsg
  ALLOCATE ( pmsl2(nx,ny), STAT = AllocateStatus)
  IF (AllocateStatus /= 0) STOP errmsg

  ALLOCATE ( precip(nx,ny,mprecip), STAT = AllocateStatus)
  IF (AllocateStatus /= 0) STOP errmsg

! the calculation-fields
  ALLOCATE ( dgarea(nx,ny), STAT = AllocateStatus)
  IF (AllocateStatus /= 0) STOP errmsg
  ALLOCATE ( avghbl(nx,ny), STAT = AllocateStatus)
  IF (AllocateStatus /= 0) STOP errmsg
  ALLOCATE ( avgprec(nx,ny), STAT = AllocateStatus)
  IF (AllocateStatus /= 0) STOP errmsg
  ALLOCATE ( accprec(nx,ny), STAT = AllocateStatus)
  IF (AllocateStatus /= 0) STOP errmsg

  ALLOCATE ( depdry(nx,ny,ncomp), STAT = AllocateStatus)
  IF (AllocateStatus /= 0) STOP errmsg
  ALLOCATE ( depwet(nx,ny,ncomp), STAT = AllocateStatus)
  IF (AllocateStatus /= 0) STOP errmsg
  ALLOCATE ( accdry(nx,ny,ncomp), STAT = AllocateStatus)
  IF (AllocateStatus /= 0) STOP errmsg
  ALLOCATE ( accwet(nx,ny,ncomp), STAT = AllocateStatus)
  IF (AllocateStatus /= 0) STOP errmsg
  ALLOCATE ( concen(nx,ny,ncomp), STAT = AllocateStatus)
  IF (AllocateStatus /= 0) STOP errmsg
  ALLOCATE ( concacc(nx,ny,ncomp), STAT = AllocateStatus)
  IF (AllocateStatus /= 0) STOP errmsg
  ALLOCATE ( avgbq1(nx,ny,ncomp), STAT = AllocateStatus)
  IF (AllocateStatus /= 0) STOP errmsg
  ALLOCATE ( avgbq2(nx,ny,ncomp), STAT = AllocateStatus)
  IF (AllocateStatus /= 0) STOP errmsg

  ALLOCATE ( avgbq(nxmc,nymc,nk-1,ncomp), STAT = AllocateStatus)
  IF (AllocateStatus /= 0) STOP errmsg

! the part particles fields
  ALLOCATE ( pdata(mpart), STAT = AllocateStatus)
  IF (AllocateStatus /= 0) STOP errmsg
  ALLOCATE ( icomp(mpart), STAT = AllocateStatus)
  IF (AllocateStatus /= 0) STOP errmsg
  ALLOCATE ( iparnum(mpart), STAT = AllocateStatus)
  IF (AllocateStatus /= 0) STOP errmsg

! the plumes
  ALLOCATE ( iplume(2,mplume), STAT = AllocateStatus)
  IF (AllocateStatus /= 0) STOP errmsg

end subroutine allocateFields


subroutine deAllocateFields
  USE particleML
  USE fileInfoML
  USE snapparML
  USE snapdimML, only: nx, ny, nk
  USE snapfldML
  USE snapfilML
  USE snapgrdML
  use snapdimML
  implicit none

  DEALLOCATE ( alevel )
  DEALLOCATE ( blevel )
  DEALLOCATE ( vlevel )
  DEALLOCATE ( ahalf )
  DEALLOCATE ( bhalf )
  DEALLOCATE ( vhalf )

  DEALLOCATE ( u1)
  DEALLOCATE ( v1)
  DEALLOCATE ( w1)
  DEALLOCATE ( t1)
  DEALLOCATE ( ps1)
  DEALLOCATE ( bl1)
  DEALLOCATE ( hbl1)
  DEALLOCATE ( hlevel1)
  DEALLOCATE ( hlayer1)

  DEALLOCATE ( u2)
  DEALLOCATE ( v2)
  DEALLOCATE ( w2)
  DEALLOCATE ( t2)
  DEALLOCATE ( ps2)
  DEALLOCATE ( bl2)
  DEALLOCATE ( hbl2)
  DEALLOCATE ( hlevel2)
  DEALLOCATE ( hlayer2)

  DEALLOCATE ( idata )
  DEALLOCATE ( fdata )

  DEALLOCATE ( xm)
  DEALLOCATE ( ym)
  DEALLOCATE ( garea)
  DEALLOCATE ( field1)
  DEALLOCATE ( field2)
  DEALLOCATE ( field3)
  DEALLOCATE ( field4)

  DEALLOCATE ( pmsl1)
  DEALLOCATE ( pmsl2)

  DEALLOCATE ( precip)

  DEALLOCATE ( dgarea )
  DEALLOCATE ( avghbl )
  DEALLOCATE ( avgprec )
  DEALLOCATE ( accprec )

  DEALLOCATE ( depdry )
  DEALLOCATE ( depwet )
  DEALLOCATE ( accdry )
  DEALLOCATE ( accwet )
  DEALLOCATE ( concen )
  DEALLOCATE ( concacc )
  DEALLOCATE ( avgbq1 )
  DEALLOCATE ( avgbq2 )

  DEALLOCATE ( avgbq )

  DEALLOCATE ( pdata )
  DEALLOCATE ( icomp )
  DEALLOCATE ( iparnum )

  DEALLOCATE ( iplume )

end subroutine deAllocateFields
end module allocateFieldsML
