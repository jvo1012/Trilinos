C    Copyright(C) 2014-2017 National Technology & Engineering Solutions of
C    Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
C    NTESS, the U.S. Government retains certain rights in this software.
C
C    Redistribution and use in source and binary forms, with or without
C    modification, are permitted provided that the following conditions are
C    met:
C
C    * Redistributions of source code must retain the above copyright
C       notice, this list of conditions and the following disclaimer.
C
C    * Redistributions in binary form must reproduce the above
C      copyright notice, this list of conditions and the following
C      disclaimer in the documentation and/or other materials provided
C      with the distribution.
C
C    * Neither the name of NTESS nor the names of its
C      contributors may be used to endorse or promote products derived
C      from this software without specific prior written permission.
C
C    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
C    "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
C    LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
C    A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
C    OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
C    SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
C    LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
C    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
C    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
C    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
C    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
C

C $Id: inintr.f,v 1.1 1990/11/30 11:09:46 gdsjaar Exp $
C $Log: inintr.f,v $
C Revision 1.1  1990/11/30 11:09:46  gdsjaar
C Initial revision
C
C
CC* FILE: [.MAIN]ININTR.FOR
CC* MODIFIED BY: TED BLACKER
CC* MODIFICATION DATE: 7/6/90
CC* MODIFICATION: COMPLETED HEADER INFORMATION
C
      SUBROUTINE ININTR (ML, MS, IFOUND, NEWINT, IIN, N19, N20, NINT,
     &   NLPS, IFLINE, ILLIST, LINKL, LINKS, ADDLNK)
C***********************************************************************
C
C  SUBROUTINE ININTR = ENTERS INTERVALS INTO THE DATABASE
C
C***********************************************************************
C
      DIMENSION NINT (ML), IIN (IFOUND)
      DIMENSION NLPS (MS), IFLINE (MS), ILLIST (MS*3)
      DIMENSION LINKL (2, ML), LINKS (2, MS)
C
      LOGICAL ADDLNK
C
      DO 110 I = 1, IFOUND
         JJ = IIN (I)
C
C  APPLY INTERVALS TO A SIDE
C
         IF (JJ.LT.0) THEN
            JJ = - JJ
            CALL LTSORT (MS, LINKS, JJ, IPNTR, ADDLNK)
            IF ( (JJ.GT.N20) .OR. (IPNTR.LE.0)) THEN
               WRITE (*, 10000) JJ
            ELSEIF (NEWINT.LT.0) THEN
               WRITE (*, 10010) JJ, IIN (2)
            ELSE
               DO 100 JJJ = IFLINE (IPNTR), IFLINE (IPNTR) +
     &            NLPS (IPNTR)-1
                  CALL LTSORT (ML, LINKL, ILLIST (JJJ), JPNTR,
     &               ADDLNK)
                  IF ( (ILLIST (JJJ) .GT. N19) .OR.
     &               (IPNTR.LE.0)) THEN
                     WRITE (*, 10020) ILLIST (JJJ)
                  ELSE
                     NINT (JPNTR) = NEWINT
                  ENDIF
  100          CONTINUE
            ENDIF
         ELSE
C
C  INPUT INTERVALS ON A LINE
C
            CALL LTSORT (ML, LINKL, JJ, IPNTR, ADDLNK)
            IF ( (JJ.GT.N19) .OR. (IPNTR.LE.0)) THEN
               WRITE (*, 10020) JJ
            ELSEIF (NEWINT .LT. 0) THEN
               WRITE (*, 10030) JJ, NEWINT
            ELSE
               NINT (IPNTR) = NEWINT
            ENDIF
         ENDIF
  110 CONTINUE
C
      RETURN
C
10000 FORMAT (' SIDE NO:', I5, ' IS NOT IN THE DATABASE', /,
     &   ' THUS NO INTERVALS CAN BE ENTERED')
10010 FORMAT (' LINES IN SIDE NO:', I5,
     &   ' CANNOT HAVE INTERVALS OF:', I8)
10020 FORMAT (' LINE NO:', I5, ' IS NOT IN THE DATABASE', /,
     &   ' THUS NO INTERVAL CAN BE ENTERED')
10030 FORMAT (' LINE NO:', I5, ' CANNOT HAVE AN INTERVAL OF:', I8)
C
      END
