/*
 ******************************************************************
 Copyright (c) 2012, Michael (Mikhail) Yudelson
 
 Redistribution and/or use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:
 
 * Redistributions of source code should retain the above copyright notice, this
 list of conditions and the following disclaimer.
 * Redistributions in binary form should reproduce the above copyright notice,
 this list of conditions and the following disclaimer in the documentation
 and/or other materials provided with the distribution.
 
 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT  LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 ******************************************************************
 */


//  FitBit is a helper class for gradient descent

#ifndef __HMM__FitBit__
#define __HMM__FitBit__

#include <iostream>
#include "utils.h"

enum FIT_BIT_SLOT {
    FBS_PAR       = 1, // e.g. PI
    FBS_PARm1     = 2, // e.g. PIm1
    FBS_GRAD      = 3, // e.g. gradPI
    FBS_GRADm1    = 4, // e.g. gradPIm1
    FBS_PARcopy   = 5, // e.g. PIcopy
    FBS_DIRm1     = 6  // e.g. gradPIdirm1
};

enum FIT_BIT_VAR {
    FBV_PI = 1, // PI
    FBV_A  = 2, // A
    FBV_B  = 3  // B
};

class FitBit {
public:
    NUMBER *PI; // usually pointer #1
    NUMBER **A; // usually pointer
    NUMBER **B; // usually pointer
    NUMBER *PIm1; // previous value #2
    NUMBER **Am1; // previous value
    NUMBER **Bm1; // previous value
    NUMBER *gradPI; // gradient #3
    NUMBER **gradA; // gradient
    NUMBER **gradB; // gradient
    NUMBER *gradPIm1; // previous gradient #4
    NUMBER **gradAm1; // previous gradient
    NUMBER **gradBm1; // previous gradient
    NUMBER *PIcopy; // previous value #5
    NUMBER **Acopy; // previous value
    NUMBER **Bcopy; // previous value
    NUMBER *dirPIm1; // previous step direction #6
    NUMBER **dirAm1; // previous step direction
    NUMBER **dirBm1; // previous step direction
    
    FitBit(NPAR a_nS, NPAR a_nO, NCAT a_nK, NCAT a_nG, NUMBER a_tol);
    ~FitBit();
    void init(enum FIT_BIT_SLOT fbs);
    void linkPar(NUMBER *a_PI, NUMBER **a_A, NUMBER **a_B);
    void toZero(enum FIT_BIT_SLOT fbs);
    void destroy(enum FIT_BIT_SLOT fbs);
    void copy(enum FIT_BIT_SLOT sourse_fbs, enum FIT_BIT_SLOT target_fbs);
    void add(enum FIT_BIT_SLOT sourse_fbs, enum FIT_BIT_SLOT target_fbs);
    bool checkConvergence();
    void doLog10ScaleGentle(enum FIT_BIT_SLOT fbs);
private:
    NPAR nO, nS;
    NCAT nG, nK; // copies
    NUMBER tol;

    void init(NUMBER* &PI, NUMBER** &A, NUMBER** &B);
    void toZero(NUMBER *PI, NUMBER **A, NUMBER **B);
    void destroy(NUMBER* &PI, NUMBER** &A, NUMBER** &B);
    void get(enum FIT_BIT_SLOT fbs, NUMBER* &a_PI, NUMBER** &a_A, NUMBER** &a_B);
    void add(NUMBER *soursePI, NUMBER **sourseA, NUMBER **sourseB, NUMBER *targetPI, NUMBER **targetA, NUMBER **targetB);
    void copy(NUMBER* &soursePI, NUMBER** &sourseA, NUMBER** &sourseB, NUMBER* &targetPI, NUMBER** &targetA, NUMBER** &targetB);
};

#endif /* defined(__HMM__FitBit__) */
