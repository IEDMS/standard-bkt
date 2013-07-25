/*
 *  predicthmm.cpp
 *  HMM
 *
 *  Created by Mikhail Yudelson on 6/6/12.
 *  Copyright 2012 __MyCompanyName__. All rights reserved.
 *
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
static char *line = NULL;
static int max_line_length;
static NDAT global_predict_N;
HMMProblem *hmm;
NUMBER* metrics;
map<string,NCAT> data_map_group_fwd;
map<NCAT,string> data_map_group_bwd;
map<string,NCAT> map_step;
map<string,NCAT> model_map_skill_fwd;
map<NCAT,string> model_map_skill_bwd;
void exit_with_help();
void parse_arguments(int argc, char **argv, char *input_file_name, char *model_file_name, char *predict_file_name);
void read_predict_data(const char *filename);
void read_model(const char *filename);
static char* readline(FILE *fid);

int main (int argc, char ** argv) {
	clock_t tm0 = clock();
	printf("predicthmm starting...\n");
	set_param_defaults(&param);
	
	char input_file[1024];
	char model_file[1024];
	char predict_file[1024];
	
	parse_arguments(argc, argv, input_file, model_file, predict_file);
    param.predictions = 1; // force it on, since we, you know, predictinng :)
	read_model(model_file);
    
    if(param.binaryinput==0) {
        InputUtil::readTxt(input_file, &param);
    } else {
        InputUtil::readBin(input_file, &param);
    }
	
	if(param.quiet == 0)
		printf("input read, nO=%d, nG=%d, nK=%d\n",param.nO, param.nG, param.nK);
	
	clock_t tm = clock();
    if(param.metrics>0 || param.predictions>0) {
        metrics = Calloc(NUMBER, 7);// LL, AIC, BIC, RMSE, RMSEnonull, Acc, Acc_nonull;
    }
    hmm->predict(metrics, predict_file, param.dat_obs, param.dat_group, param.dat_skill, param.dat_multiskill, true/*only unlabelled*/);
	if(param.quiet == 0)
		printf("predicting is done in %8.6f seconds\n",(NUMBER)(clock()-tm)/CLOCKS_PER_SEC);
    if( param.metrics>0 ) {
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
		   "Usage: trainhmm [options] input_file [[output_file] predicted_response_file]\n"
		   "options:\n"
		   "(-s) : structure.solver[.solver setting], structures: 1-by skill, 2-by user,\n"
           "       5-A by skill and user, Pi,B by skill, 6-Pi,A by skill and user, B by skill;\n"
           "       solvers: 1-Baum-Welch, 2-Gradient Descent, 3-Conjugate Gradient Descent;\n"
           "       Conjugate Gradient Descent has 3 setings: 1-Polak-Ribiere, 2-Fletcher–Reeves,\n"
           "       3-Hestenes-Stiefel.\n"
           "       For example '-s 1.3.1' would be by skill structure (classical) with Conjugate\n"
           "       Gradient Descent and Hestenes-Stiefel formula, '-s 2.1' would be by student structure\n"
           "       fit using Baum-Welch method.\n"
		   "-e : tolerance (epsilon) of termination criterion (0.01 default)\n"
		   "-t : reading time from 5th column 0-do not read(default) 1-do read as seconds since epoch.\n"
		   "-i : maximum iterations (200 by default)\n"
		   "-q : quiet mode, without output, 0-no (default), or 1-yes\n"
		   "-n : number of hidden states, should be 2 or more (default 2)\n"
		   "-0 : initial params (comma-separated), {PI{n-1},A{(n*(n-1)},B{n*(observations-1)}}\n"
		   "     default 0.5,1.0,0.4,0.8,0.2\n"
		   "-l : lower boundaries for params (comma-separated), {PI{n},A{n*n},B{n*(observations)}}\n"
		   "     default 0,0,1,0,0,0,0,0,0,0\n"
		   "-u : upper boundaries for params (comma-separated), {PI{n},A{n*n},B{n*(observations)}}\n"
		   "     default 0,0,1,0,0,0,0,0,0,0\n"
		   "(-c) : weight of the L1 penalty, 0 (default) if not applicable, 1 if applicable\n"
		   "(-f) : fit as one skill, 0-no (default), 1 - fit as one skill and use params as starting\n"
           "       point for multi-skill, 2 - force one skill"
		   "-m : report model fitting metrics (AIC, BIC, RMSE) 0-no (default), 1-yes. To specify observation\n"
           "     for which metrics to be reported, list it after ';'. For example '-r 0', '-r 1' (by default, \n"
           "     observation 1 is assumed), '-r1:2' (compute metrics for observation 2). Incompatible with-v option.\n"
		   "-v : cross-validation folds and target state to validate against, perform subject-stratified\n"
		   "     cross-validation, default 0 (no cross-validation),\n"
           "     examples '-v 5;2' - 5 fold, predict state 2, '-v 10' - 10-fold predict state 1 by default.\n"
           "     Incompatible with -r option.\n"
		   "-p : report model predictions on the train set 0-no (default), 1-yes; works with any combination of\n"
           "     -v and -m params.\n"
		   "-d : multi-skill per observation delimiter 0-sinle skill per observation (default), [delimiter character].\n"
           "     For example '-d ~'.\n"
		   "-b : treat input file as binary input file (lookup format specifications in help)\n"
		   );
	exit(1);
}

void parse_arguments(int argc, char **argv, char *input_file_name, char *model_file_name, char *predict_file_name) {
	// parse command line options, starting from 1 (0 is path to executable)
	// go in pairs, looking at whether first in pair starts with '-', if not, stop parsing arguments
	int i;
    char * ch;
	for(i=1;i<argc;i++)
	{
		if(argv[i][0] != '-') break; // end of options stop parsing
		if(++i>=argc)
			exit_with_help();
		switch(argv[i-1][1])
		{
			case 'q':
				param.quiet = atoi(argv[i]);
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
			case 'm':
                param.metrics = atoi( strtok(argv[i],";\t\n\r"));
                ch = strtok(NULL, "\t\n\r");
                if(ch!=NULL)
                    param.metrics_target_obs = atoi(ch)-1;
				if(param.metrics<0 || param.metrics>1) {
					fprintf(stderr,"value for -m should be either 0 or 1.\n");
					exit_with_help();
				}
				if(param.metrics_target_obs<0) {// || param.metrics_target_obs>(param.nO-1)) {
					fprintf(stderr,"target observation to compute metrics against cannot be '%d'\n",param.metrics_target_obs+1);
					exit_with_help();
				}
                break;
            case  'd':
				param.multiskill = argv[i][0]; // just grab first character (later, maybe several)
                break;
			case 'b':
                param.binaryinput = atoi( strtok(argv[i],"\t\n\r"));
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

void read_model(const char *filename) {
	FILE *fid = fopen(filename,"r");
	if(fid == NULL)
	{
		fprintf(stderr,"Can't read model file %s\n",filename);
		exit(1);
	}
	max_line_length = 1024;
	line = Malloc(char,max_line_length);
	NDAT line_no = 0;
    //
    // read solver info
    //
    readSolverInfo(fid, &param, &line_no);
    
    //
    // create hmm Object
    //
//    switch(param.structure)
//    {
//        case STRUCTURE_SKILL: // Conjugate Gradient Descent
//        case STRUCTURE_GROUP: // Conjugate Gradient Descent
            hmm = new HMMProblem(&param);
//            break;
//    }
    //
    // read model
    //
    hmm->readModel(fid, &line_no);
    
	
    //	k=0;
    //	map<NCAT,string>::iterator it;
    //	for(k=0; k<param.nK; k++) {
    //		it	= model_map_skill_bwd.find(k);
    //		printf("%d %d %s \n", k, it->first, it->second.c_str());
    //	}
	
	fclose(fid);
	free(line);
}

