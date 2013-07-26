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

//
//  InputUtil.h
//  This class is a utility that reads input text for HMM, converts it
//  to a more compact binary file, and reads the binary file too
//  HMM

#ifndef __HMM__InputUtil__
#define __HMM__InputUtil__

#include "utils.h"
#define bin_input_file_verstion 1

class InputUtil {
public:
    static bool readTxt(const char *fn, struct param * param); // read txt into param
    static bool readBin(const char *fn, struct param * param); // read bin into param
    static bool toBin(struct param * param, const char *fn);// writes data in param to bin file
private:
    static void writeString(FILE *f, string str);
    static string readString(FILE *f);
    static unsigned long writeMultiSkill(FILE *f, struct param * param);
    static unsigned long  readMultiSkill(FILE *f, struct param * param);
};
#endif /* defined(__HMM__InputUtil__) */
