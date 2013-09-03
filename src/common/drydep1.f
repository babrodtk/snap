      subroutine drydep1
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
      include 'snapdim.inc'
      include 'snapgrd.inc'
      include 'snapfld.inc'
      include 'snappar.inc'
c
      integer m,n,i,j,mm
      real    h,dep
c
#if defined(DRHOOK)
      ! Before the very first statement
      IF (LHOOK) CALL DR_HOOK('DRYDEP1',0,ZHOOK_HANDLE)
#endif
c
      do n=1,npart
       m= icomp(n)
       if(kdrydep(m).eq.1) then
c..very rough eastimate of height,
c..using boundary layer height, then just linear in sigma/eta !!! ????
         h=pdata(5,n)*(1.-pdata(3,n))/(1.-pdata(4,n))
         if(h.lt.drydephgt(m)) then
            dep=drydeprat(m)*pdata(9,n)
            pdata(9,n)=pdata(9,n)-dep
           i=nint(pdata(1,n))
           j=nint(pdata(2,n))
           mm=iruncomp(m)
            depdry(i,j,mm)=depdry(i,j,mm)+dble(dep)
         end if
       end if
      end do
c
#if defined(DRHOOK)
c     before the return statement
      IF (LHOOK) CALL DR_HOOK('DRYDEP1',1,ZHOOK_HANDLE)
#endif
      return
      end