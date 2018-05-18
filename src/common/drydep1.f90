! SNAP: Servere Nuclear Accident Programme
! Copyright (C) 1992-2017   Norwegian Meteorological Institute
! 
! This file is part of SNAP. SNAP is free software: you can 
! redistribute it and/or modify it under the terms of the 
! GNU General Public License as published by the 
! Free Software Foundation, either version 3 of the License, or
! (at your option) any later version.
! 
! This program is distributed in the hope that it will be useful,
! but WITHOUT ANY WARRANTY; without even the implied warranty of
! MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
! GNU General Public License for more details.
! 
! You should have received a copy of the GNU General Public License
! along with this program.  If not, see <https://www.gnu.org/licenses/>.
!
      subroutine drydep1(n)
        USE particleML
        USE snapfldML
        USE snapparML
        USE snapdimML, only: nx,ny,nk

c
c  Purpose:  Compute dry deposition for each particle and each component
c            and store depositions in nearest gridpoint in a field
c  Method:   J.Saltbones 1994
c
c
#if defined(DRHOOK)
      USE PARKIND1  ,ONLY : JPIM     ,JPRB
      USE YOMHOOK   ,ONLY : LHOOK,   DR_HOOK
#endif
      implicit none
#if defined(DRHOOK)
      REAL(KIND=JPRB) :: ZHOOK_HANDLE ! Stack variable i.e. do not use SAVE
#endif
c
c particle loop index, n=0 means init
      INTEGER, INTENT(IN) :: n
      integer m,i,j,mm
      real    h,dep
c
#if defined(DRHOOK)
      ! Before the very first statement
      IF (LHOOK) CALL DR_HOOK('DRYDEP1',0,ZHOOK_HANDLE)
#endif
c
c      do n=1,npart // particle loop now outside
       m= icomp(n)
       if(kdrydep(m).eq.1) then
c..very rough eastimate of height,
c..using boundary layer height, then just linear in sigma/eta !!! ????
         h=pdata(n)%hbl*(1.-pdata(n)%z)/(1.-pdata(n)%tbl)
         if(h.lt.drydephgt(m)) then
            dep=drydeprat(m)*pdata(n)%rad
            pdata(n)%rad=pdata(n)%rad-dep
           i=nint(pdata(n)%x)
           j=nint(pdata(n)%y)
           mm=iruncomp(m)
!$omp atomic
            depdry(i,j,mm)=depdry(i,j,mm)+dble(dep)
         end if
       end if
c      end do
c
#if defined(DRHOOK)
c     before the return statement
      IF (LHOOK) CALL DR_HOOK('DRYDEP1',1,ZHOOK_HANDLE)
#endif
      return
      end