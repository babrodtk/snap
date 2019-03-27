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
module snaptabML
  use snapdimML, only: mpretab,mprepro
  implicit none
  private
!
!..include file  - fixed tables and constants
!                  (independant of input data)
!
!
!..pmult:  multiply pressure by this value to get index in pitab
!..pitab:  Exner function, pitab(0:130) for p=0,10,20,...1300 hPa
!..g    :  ...
!..r    :  ...
!..cp   :  ...
!
      real, parameter, public :: pmult=0.1,g=9.81,r=287.0,cp=1004.0
      real, save, public :: pitab(0:130)
!
!
!..premult: multiply precipitation intensity (mm/hour) by this value
!	    to get index in pretab
!..pretab:  precipitation parobability table (for wet depositions)
!..nprepro:     no. of steps in input precipitation probability table
!..prepro(1,n): precipitation intensity (mm/hour)
!..prepro(2,n): probability for precipitation (0. - 1.)
!
      real, save, public :: premult,pretab(0:mpretab)
      integer, save, public :: nprepro
      real, save, public :: prepro(2,mprepro+1)
!
end module snaptabML
