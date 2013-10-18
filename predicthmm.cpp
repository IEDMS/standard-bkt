/*
 
 Copyright (c) 2012, Michael (Mikhail) Yudelson
 All rights reserved.
 
 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:
 * Redistributions of source code must retain the above copyright
 notice, this list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright
 notice, this list of conditions and the following disclaimer in the
 documentation and/or other materials provided with the distribution.
 * Neither the name of the Michael (Mikhail) Yudelson nor the
 names of other contributors may be used to endorse or promote products
 derived from this software without specific prior written permission.
 
 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL COPYRIGHT HOLDERS AND CONTRIBUTORS BE LIABLE FOR
 ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 
 */

#include <string.h>
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <map>
#include <list>
#include "utils.h"
#include "HMMProblem.h"
#include "InputUtil.h"
using namespace std;

#define COLUMNS 4

struct param param;
//static int max_line_length;
//static NDAT global_predict_N;
NUMBER* metrics;
map<string,NCAT> data_map_group_fwd;
map<NCAT,string> data_map_group_bwd;
map<string,NCAT> map_step;
map<string,NCAT> model_map_skill_fwd;
map<NCAT,string> model_map_skill_bwd;
void exit_with_help();
void parse_arguments(int argc, char **argv, char *input_file_name, char *model_file_name, char *predict_file_name);
void read_predict_data(const char *filename);
void predict(const char *predict_file, HMMProblem *hmm);

int main (int argc, char ** argv) {
	clock_t tm0 = clock();
	printf("predicthmm starting...\n");
	set_param_defaults(&param);
	
	char input_file[1024];
	char model_file[1024];
	char predict_file[1024];
	
	parse_arguments(argc, argv, input_file, model_file, predict_file);
    param.predictions = 2; // force it on, since we, you know, predictinng :)

    // read data
    if(param.binaryinput==0) {
        InputUtil::readTxt(input_file, &param);
    } else {
        InputUtil::readBin(input_file, &param);
    }
    
    // read model header
	FILE *fid = fopen(model_file,"r");
	if(fid == NULL)
	{
		fprintf(stderr,"Can't read model file %s\n",model_file);
		exit(1);
	}
	int max_line_length = 1024;
	char *line = Malloc(char,(size_t)max_line_length);
	NDAT line_no = 0;
    struct param param_model;
    set_param_defaults(&param_model);
    bool overwrite = false;
//    if(overwrite)
        readSolverInfo(fid, &param_model, &line_no);
//    else
//        readSolverInfo(fid, &initparam, &line_no);
    
    // read model body
    HMMProblem * hmm = new HMMProblem(&param);
    hmm->readModelBody(fid, &param_model, &line_no, overwrite);
  	fclose(fid);
	free(line);

	if(param.quiet == 0)
        printf("input read, nO=%d, nG=%d, nK=%d, nI=%d\n",param.nO, param.nG, param.nK, param.nI);
	
	clock_t tm = clock();
    if(param.metrics>0 || param.predictions>0) {
        metrics = Calloc(NUMBER, (size_t)7);// LL, AIC, BIC, RMSE, RMSEnonull, Acc, Acc_nonull;
    }
    hmm->predict(metrics, predict_file, param.dat_obs, param.dat_group, param.dat_skill, param.dat_multiskill, false/*only unlabelled*/);
//    predict(predict_file, hmm);
	if(param.quiet == 0)
		printf("predicting is done in %8.6f seconds\n",(NUMBER)(clock()-tm)/CLOCKS_PER_SEC);
    // THERE IS NO METRICS, WE PREDICT UNKNOWN, however, if we force prediction of all we do
    if( param.predictions>0 ) {
        printf("predicted model LL=%15.7f, AIC=%8.6f, BIC=%8.6f, RMSE=%8.6f (%8.6f), Acc=%8.6f (%8.6f)\n",metrics[0], metrics[1], metrics[2], metrics[3], metrics[4], metrics[5], metrics[6]);
    }
    free(metrics);
    
	destroy_input_data(&param);
	
    delete hmm;
	if(param.quiet == 0)
		printf("overall time running is %8.6f seconds\n",(NUMBER)(clock()-tm0)/CLOCKS_PER_SEC);
    return 0;
}

void exit_with_help() {
	printf(
		   "Usage: predicthmm [options] input_file model_file [predicted_response_file]\n"
           "options:\n"
           "-q : quiet mode, without output, 0-no (default), or 1-yes\n"
           "-d : delimiter for multiple skills per observation; 0-single skill per\n"
           "     observation (default), otherwise -- delimiter character, e.g. '-d ~'.\n"
           "-b : treat input file as binary input file (specifications TBA).\n"
           "-p : produce model predictions for all rows, not just ones with unknown\n"
           "     observations.\n"
		   );
	exit(1);
}

void parse_arguments(int argc, char **argv, char *input_file_name, char *model_file_name, char *predict_file_name) {
	// parse command line options, starting from 1 (0 is path to executable)
	// go in pairs, looking at whether first in pair starts with '-', if not, stop parsing arguments
	int i;
//    char * ch;
	for(i=1;i<argc;i++)
	{
		if(argv[i][0] != '-') break; // end of options stop parsing
		if(++i>=argc)
			exit_with_help();
		switch(argv[i-1][1])
		{
			case 'q':
				param.quiet = (NPAR)atoi(argv[i]);
				if(param.quiet!=0 && param.quiet!=1) {
					fprintf(stderr,"ERROR! Quiet param should be 0 or 1\n");
					exit_with_help();
				}
				break;
//			case 'n':
//				param.nS = (NPAR)atoi(argv[i]);
//				if(param.nS<2) {
//					fprintf(stderr,"ERROR! Number of hidden states should be at least 2\n");
//					exit_with_help();
//				}
//				//fprintf(stdout, "fit single skill=%d\n",param.quiet);
//				break;
            case  'd':
				param.multiskill = argv[i][0]; // just grab first character (later, maybe several)
                break;
			case 'b':
                param.binaryinput = atoi( strtok(argv[i],"\t\n\r"));
                break;
            case  'p':
				param.predictions = atoi(argv[i]);
				if(param.predictions<0 || param.predictions>1) {
					fprintf(stderr,"a flag of whether to report predictions for training data (-p) should be 0 or 1\n");
					exit_with_help();
				}
                break;
			default:
				fprintf(stderr,"unknown option: -%c\n", argv[i-1][1]);
				exit_with_help();
				break;
		}
	}   
    if(param.cv_target_obs>0 && param.metrics>0) { // correct for 0-start coding
        fprintf(stderr,"values for -v and -m cannot be both non-zeros\n");
        exit_with_help();
    }
	
	// next argument should be input (training) file name
	if(i>=argc) // if not
		exit_with_help(); // leave
	
	strcpy(input_file_name, argv[i++]); // copy and advance
	
	if(i>=argc) { // no model file name specified
        fprintf(stderr,"no model file specified\n");
		exit_with_help(); // leave
	}
	else {
		strcpy(model_file_name,argv[i++]); // copy and advance
		if(i>=argc) // no prediction file name specified
			strcpy(predict_file_name,"predict_hmm.txt"); // the predict file too
		else
			strcpy(predict_file_name,argv[i]);
	}
}
