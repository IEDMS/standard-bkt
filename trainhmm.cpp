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
#include "InputUtil.h"
#include "HMMProblem.h"
#include "StripedArray.h"
using namespace std;

struct param param;
void exit_with_help();
void parse_arguments(int argc, char **argv, char *input_file_name, char *output_file_name, char *predict_file_name);
bool read_and_structure_data(const char *filename);
void cross_validate(NUMBER* metrics, const char *filename);
void cross_validate_item(NUMBER* metrics, const char *filename);
void cross_validate_nstrat(NUMBER* metrics, const char *filename);

int main (int argc, char ** argv) {
    
	clock_t tm0 = clock();
	char input_file[1024];
	char output_file[1024];
	char predict_file[1024];
    
	set_param_defaults(&param);
	parse_arguments(argc, argv, input_file, output_file, predict_file);
    
    
    if(!param.quiet)
        printf("trainhmm starting...\n");
	bool is_data_read = read_and_structure_data(input_file);
    
    if( is_data_read ) {
        if(!param.quiet)
            printf("input read, nO=%d, nG=%d, nK=%d, nI=%d\n",param.nO, param.nG, param.nK, param.nI);
        
        clock_t tm;
        
        // erase blocking labels
        zeroLabels(&param);
        // go
        if(param.cv_folds==0) { // not cross-validation
            // create problem
            HMMProblem *hmm;
//            switch(param.structure)
//            {
//                case STRUCTURE_SKILL: // Conjugate Gradient Descent
//                case STRUCTURE_GROUP: // Conjugate Gradient Descent
                    hmm = new HMMProblem(&param);
//                    break;
//            }
            
            tm = clock();
            hmm->fit();
            
            if(param.quiet == 0)
                printf("fitting is done in %8.6f seconds\n",(NUMBER)(clock()-tm)/CLOCKS_PER_SEC);
            
            // write model
            hmm->toFile(output_file);
            
            if(param.metrics>0 || param.predictions>0) {
                NUMBER* metrics = Calloc(NUMBER, 7); // LL, AIC, BIC, RMSE, RMSEnonull, Acc, Acc_nonull;
                hmm->predict(metrics, predict_file, param.dat_obs, param.dat_group, param.dat_skill, param.dat_multiskill, false/*all, not only unlabelled*/);
                if( param.metrics>0 && !param.quiet) {
                    printf("trained model LL=%15.7f, AIC=%8.6f, BIC=%8.6f, RMSE=%8.6f (%8.6f), Acc=%8.6f (%8.6f)\n",metrics[0], metrics[1], metrics[2], metrics[3], metrics[4], metrics[5], metrics[6]);
                }
                free(metrics);
            } // if predict or metrics
            
            delete hmm;
        } else { // cross-validation
            tm = clock();
            NUMBER* metrics = Calloc(NUMBER, 7); // AIC, BIC, RMSE, RMSE no null
            switch (param.cv_strat) {
                case CV_GROUP:
                    cross_validate(metrics, predict_file);
                    break;
                case CV_ITEM:
                    cross_validate_item(metrics, predict_file);
                    break;
                case CV_NSTR:
                    cross_validate_nstrat(metrics, predict_file);
                    break;
                default:
                    
                    break;
            }
            if(!param.quiet)
                printf("%d-fold cross-validation: LL=%15.7f, AIC=%8.6f, BIC=%8.6f, RMSE=%8.6f (%8.6f), Acc=%8.6f (%8.6f) computed in %8.6f seconds\n",param.cv_folds, metrics[0], metrics[1], metrics[2], metrics[3], metrics[4], metrics[5], metrics[6], (NUMBER)(clock()-tm)/CLOCKS_PER_SEC);
            free(metrics);
        }
    }
	// free data
	destroy_input_data(&param);
	
	if(param.quiet == 0)
		printf("overall time running is %8.6f seconds\n",(NUMBER)(clock()-tm0)/CLOCKS_PER_SEC);
    return 0;
}

void exit_with_help() {
	printf(
		   "Usage: trainhmm [options] input_file [[output_file] predicted_response_file]\n"
           "options:\n"
           "-s : structure.solver[.solver setting], structures: 1-by skill, 2-by user;\n"
           "     solvers: 1-Baum-Welch, 2-Gradient Descent, 3-Conjugate Gradient Descent;\n"
           "     Conjugate Gradient Descent has 3 settings: 1-Polak-Ribiere,\n"
           "     2-Fletcher–Reeves, 3-Hestenes-Stiefel.\n"
           "     For example '-s 1.3.1' would be by skill structure (classical) with\n"
           "     Conjugate Gradient Descent and Hestenes-Stiefel formula, '-s 2.1' would be\n"
           "     by student structure fit using Baum-Welch method.\n"
           "-t : tolerance of termination criterion (0.01 default)\n"
           "-i : maximum iterations (200 by default)\n"
           "-q : quiet mode, without output, 0-no (default), or 1-yes\n"
           "-n : number of hidden states, should be 2 or more (default 2)\n"
           "-0 : initial parameters comma-separated for priors, transition, and emission\n"
           "     probabilities skipping the last value from each vector (matrix row) since\n"
           "     they sum up to 1; default 0.5,1.0,0.4,0.8,0.2\n"
           "-l : lower boundaries for parameters, comma-separated for priors, transition,\n"
           "     and emission probabilities (without skips); default 0,0,1,0,0,0,0,0,0,0\n"
           "-u : upper boundaries for params, comma-separated for priors, transition,\n"
           "     and emission probabilities (without skips); default 0,0,1,0,0,0,0,0,0,0\n"
           "-c : weight of the L2 penalty, 0 (default)\n"
           "-f : fit as one skill, 0-no (default), 1 - fit as one skill and use params as\n"
           "     starting point for multi-skill, 2 - force one skill\n"
           "-m : report model fitting metrics (AIC, BIC, RMSE) 0-no (default), 1-yes. To \n"
           "     specify observation for which metrics to be reported, list it after ','.\n"
           "     For example '-m 0', '-m 1' (by default, observation 1 is assumed), '-m 1,2'\n"
           "     (compute metrics for observation 2). Incompatible with-v option.\n"
           "-v : cross-validation folds and target state to validate against, perform\n"
           "     subject-stratified cross-validation, default 0 (no cross-validation),\n"
           "     examples '-v 5,2' - 5 fold, predict state 2, '-v 10' - 10-fold predict\n"
           "     state 1 by default.\n"
           "-p : report model predictions on the train set 0-no (default), 1-yes; 2-yes,\n"
           "     plus output state probability; works with -v and -m parameters.\n"
           "-d : delimiter for multiple skills per observation; 0-single skill per\n"
           "     observation (default), otherwise -- delimiter character, e.g. '-d ~'.\n"
           "-b : treat input file as binary input file (specifications TBA).\n"
           "-B : Block PI (prior), A (transition), or B (observation) parameters from being\n"
           "     fit. E.g., '-B 0,0,0 (default) blocks none, '-B 1,0,0' blocks PI (priors).\n"
		   );
	exit(1);
}

void parse_arguments(int argc, char **argv, char *input_file_name, char *output_file_name, char *predict_file_name) {
	// parse command line options, starting from 1 (0 is path to executable)
	// go in pairs, looking at whether first in pair starts with '-', if not, stop parsing arguments
	int i;
    NPAR n;
    char *ch, *ch2;
	for(i=1;i<argc;i++)
	{
		if(argv[i][0] != '-') break; // end of options stop parsing
		if(++i>=argc)
			exit_with_help();
		switch(argv[i-1][1])
		{
			case 'e':
				param.tol = atof(argv[i]);
				if(param.tol<0) {
					fprintf(stderr,"ERROR! Fitting tolerance cannot be negative\n");
					exit_with_help();
				}
				if(param.tol>10) {
					fprintf(stderr,"ERROR! Fitting tolerance cannot be >10\n");
					exit_with_help();
				}
				break;
			case 't':
				param.time = (NPAR)atoi(argv[i]);
				if(param.time!=0 && param.time!=1) {
					fprintf(stderr,"ERROR! Time parameter should be either 0 (off) or 1(om)\n");
					exit_with_help();
				}
				break;
			case 'i':
				param.maxiter = atoi(argv[i]);
				if(param.maxiter<10) {
					fprintf(stderr,"ERROR! Maximum iterations should be at least 10\n");
					exit_with_help();
				}
				break;
			case 'q':
				param.quiet = (NPAR)atoi(argv[i]);
				if(param.quiet!=0 && param.quiet!=1) {
					fprintf(stderr,"ERROR! Quiet param should be 0 or 1\n");
					exit_with_help();
				}
				break;
			case 'n':
				param.nS = (NPAR)atoi(argv[i]);
				if(param.nS<2) {
					fprintf(stderr,"ERROR! Number of hidden states should be at least 2\n");
					exit_with_help();
				}
				//fprintf(stdout, "fit single skill=%d\n",param.quiet);
				break;
			case 's':
				param.structure = (NPAR)atoi( strtok(argv[i],".\t\n\r") );
                ch = strtok(NULL,".\t\n\r"); // could be NULL (default GD solver)
                if(ch != NULL)
                    param.solver = (NPAR)atoi(ch);
                ch = strtok(NULL,"\t\n\r"); // could be NULL (default GD solver)
                if(ch != NULL)
                    param.solver_setting = (NPAR)atoi(ch);
                if( param.structure != STRUCTURE_SKILL && param.structure != STRUCTURE_GROUP  ) {
                    fprintf(stderr, "Model Structure specified (%d) is out of range of allowed values\n",param.structure);
					exit_with_help();
                }
                if( param.solver != METHOD_BW  && param.solver != METHOD_GD &&
                   param.solver != METHOD_CGD ) {
                    fprintf(stderr, "Method specified (%d) is out of range of allowed values\n",param.solver);
					exit_with_help();
                }
                if( param.solver == METHOD_BW && ( param.solver != STRUCTURE_SKILL && param.solver != STRUCTURE_GROUP ) ) {
                    fprintf(stderr, "Baum-Welch solver does not support model structure specified (%d)\n",param.solver);
					exit_with_help();
                }
                if( param.solver == METHOD_CGD  &&
                   ( param.solver_setting != 1 && param.solver_setting != 2 &&
                    param.solver_setting != 3 )
                   ) {
                    fprintf(stderr, "Conjugate Gradient Descent setting specified (%d) is out of range of allowed values\n",param.solver_setting);
					exit_with_help();
                }
				break;
            case 'f':
                param.single_skill = (NPAR)atoi(argv[i]);
                break;
			case '0': // init_params
				int len;
				len = (int)strlen( argv[i] );
				// count delimiters
				n = 1; // start with 1
				for(int j=0;j<len;j++) {
					n += (argv[i][j]==',')?(NPAR)1:(NPAR)0;
                    if( (argv[i][j] >= 'a' && argv[i][j] <= 'z') || (argv[i][j] >= 'A' && argv[i][j] <= 'Z') ) {
                        strcpy(param.initfile, argv[i]);
                        break;
                    }
                }
                if(param.initfile[0]==0) { // init parameters parameters
                    // init params
                    free(param.init_params);
                    param.init_params = Calloc(NUMBER, (size_t)n);
                    // read params and write to params
                    param.init_params[0] = atof( strtok(argv[i],",\t\n\r") );
                    for(int j=1; j<n; j++)
                        param.init_params[j] = atof( strtok(NULL,",\t\n\r") );
                }
				break;
			case 'l': // lower poundaries
				len = (int)strlen( argv[i] );
				// count delimiters
				n = 1; // start with 1
				for(int j=0;j<len;j++)
					n += (argv[i][j]==',')?(NPAR)1:(NPAR)0;
				// init params
				free(param.param_lo);
				param.param_lo = Calloc(NUMBER, (size_t)n);
				// read params and write to params
				param.param_lo[0] = atof( strtok(argv[i],",\t\n\r") );
				for(int j=1; j<n; j++)
					param.param_lo[j] = atof( strtok(NULL,",\t\n\r") );
				break;
			case 'u': // upper poundaries
				len = (int)strlen( argv[i] );
				// count delimiters
				n = 1; // start with 1
				for(int j=0;j<len;j++)
					n += (argv[i][j]==',')?(NPAR)1:(NPAR)0;
				// init params
				free(param.param_hi);
				param.param_hi = Calloc(NUMBER, (size_t)n);
				// read params and write to params
				param.param_hi[0] = atof( strtok(argv[i],",\t\n\r") );
				for(int j=1; j<n; j++)
					param.param_hi[j] = atof( strtok(NULL,",\t\n\r") );
				break;
			case 'B': // block fitting
                // first
				param.block_fitting[0] = (NPAR)atoi( strtok(argv[i],",\t\n\r") );
                if(param.block_fitting[0]!=0 && param.block_fitting[0]!=1) {
                    fprintf(stderr,"Values of blocking the fitting flags shuld only be 0 or 1.\n");
                    exit_with_help();
                }
                // second
                ch = strtok(NULL,",\t\n\r"); // could be NULL (default GD solver)
                if(ch != NULL) {
                    param.block_fitting[1] = (NPAR)atoi(ch);
                    if(param.block_fitting[1]!=0 && param.block_fitting[1]!=1) {
                        fprintf(stderr,"Values of blocking the fitting flags shuld only be 0 or 1.\n");
                        exit_with_help();
                    }
                }
                else {
                    fprintf(stderr,"There should be 3 blockig the fitting flags specified.\n");
                    exit_with_help();
                }
                // third
                ch = strtok(NULL,",\t\n\r"); // could be NULL (default GD solver)
                if(ch != NULL) {
                    param.block_fitting[2] = (NPAR)atoi(ch);
                    if(param.block_fitting[2]!=0 && param.block_fitting[2]!=1) {
                        fprintf(stderr,"Values of blocking the fitting flags shuld only be 0 or 1.\n");
                        exit_with_help();
                    }
                }
                else {
                    fprintf(stderr,"There should be 3 blockig the fitting flags specified.\n");
                    exit_with_help();
                }
				break;
			case 'c':
				param.C = atof(argv[i]);
				if(param.C < 0) {
					fprintf(stderr,"Regularization parameter C should be above 0.\n");
					exit_with_help();
				}
				if(param.C > 1000) {
					fprintf(stderr,"Regularization parameter C is _very) high and might be impractical(%f).\n", param.C);
				}
				break;
			case 'm':
                param.metrics = atoi( strtok(argv[i],",\t\n\r"));
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
			case 'b':
                param.binaryinput = atoi( strtok(argv[i],"\t\n\r"));
                break;
			case 'v':
				param.cv_folds   = (NPAR)atoi( strtok(argv[i],",\t\n\r"));
                ch2 = strtok(NULL, ",\t\n\r");
                if(ch2!=NULL)
                    param.cv_strat = ch2[0];
                ch = strtok(NULL, "\t\n\r");
                if(ch!=NULL)
                    param.cv_target_obs = (NPAR)(atoi(ch)-1);
                
				if(param.cv_folds<2) {
					fprintf(stderr,"number of cross-validation folds should be at least 2\n");
					exit_with_help();
				}
				if(param.cv_folds>10) {
					fprintf(stderr,"please keep number of cross-validation folds less than or equal to 10\n");
					exit_with_help();
				}
                if(param.cv_strat != CV_GROUP && param.cv_strat != CV_ITEM && param.cv_strat != CV_NSTR){
					fprintf(stderr,"cross-validation stratification parameter '%c' is illegal\n",param.cv_strat);
					exit_with_help();
                }
				if(param.cv_target_obs<0) {// || param.cv_target_obs>(param.nO-1)) {
					fprintf(stderr,"target observation to be cross-validated against cannot be '%d'\n",param.cv_target_obs+1);
					exit_with_help();
				}
				break;
            case  'p':
				param.predictions = atoi(argv[i]);
				if(param.predictions<0 || param.predictions>2) {
					fprintf(stderr,"a flag of whether to report predictions for training data (-p) should be 0, 1 or 2\n");
					exit_with_help();
				}
                break;
            case  'd':
				param.multiskill = argv[i][0]; // just grab first character (later, maybe several)
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
	
	// next argument should be input file name
	if(i>=argc) // if not
		exit_with_help(); // leave
	
	strcpy(input_file_name, argv[i++]); // copy and advance
	
	if(i>=argc) { // no output file name specified
		strcpy(output_file_name,"output.hmm");
		strcpy(predict_file_name,"predict_hmm.txt"); // the predict file too
	}
	else {
		strcpy(output_file_name,argv[i++]); // copy and advance
		if(i>=argc) // no prediction file name specified
			strcpy(predict_file_name,"predict_hmm.txt"); // the predict file too
		else
			strcpy(predict_file_name,argv[i]);
	}
}

bool read_and_structure_data(const char *filename) {
    bool readok = true;
    if(param.binaryinput==0)
        readok = InputUtil::readTxt(filename, &param);
    else
        readok = InputUtil::readBin(filename, &param);
    if(! readok )
        return false;
    
	//	2. distribute data into nK skill bins
	//		create
	//          skill_group_map[nK][nG] - explicit 'sparse' map of skills and groups, here 1 means done
	//			k_numg[nK]        - number of groups per skill                 RETAIN
	
	NDAT t = 0;
    int tm = 0; // time
	NCAT g, k;
	NPAR o;
	NPAR **skill_group_map = init2D<NPAR>(param.nK, param.nG); // binary map of skills to groups
	param.k_numg = Calloc(NCAT, (size_t)param.nK);
	param.g_numk = Calloc(NCAT, (size_t)param.nG);
    NDAT *count_null_skill_group = Calloc(NDAT, (size_t)param.nG); // count null skill occurences per group
    NCAT *index_null_skill_group = Calloc(NCAT, (size_t)param.nG); // index of group in compressed array
    
	// Pass A
	for(t=0; t<param.N; t++) {
        if(param.multiskill==0)
            k = param.dat_skill->get(t);//[t];
        else
            k = param.dat_multiskill->get(t)[1]; // #0 is count, #1 is first element
		g = param.dat_group->get(t);//[t];
		// null skill : just count
		if( k < 0 ) {
            if(count_null_skill_group[g]==0) param.n_null_skill_group++;
            count_null_skill_group[g]++;
			continue;
		}
        NCAT *ar;
        int n;
        if(param.multiskill==0) {
            k = param.dat_skill->get(t);
            ar = &k;
            n = 1;
        } else {
            ar = &param.dat_multiskill->get(t)[1];
            n = param.dat_multiskill->get(t)[0];
        }
        for(int l=0; l<n; l++) {
            k = ar[l];
            if( skill_group_map[k][g] == 0 ) {
                skill_group_map[k][g] = 1;
                param.k_numg[k]++;
                param.g_numk[g]++;
            }
        }
	}
    for(k=0; k<param.nK; k++) param.nSeq += param.k_numg[k];
    param.all_data = Calloc(struct data, (size_t)param.nSeq);
    
	// Section B
	param.k_g_data = Malloc(struct data **, (size_t)param.nK);
	param.k_data = Malloc(struct data *, (size_t)param.nSeq);
    //	for(k=0; k<param.nK; k++)
    //		param.k_g_data[k] = Calloc(struct data*, param.k_numg[k]);
	param.g_k_data = Calloc(struct data **, (size_t)param.nG);
	param.g_data = Malloc(struct data *, (size_t)param.nSeq);
    //	for(g=0; g<param.nG; g++)
    //		param.g_k_data[g] = Calloc(struct data*, param.g_numk[g]);
	param.null_skills = Malloc(struct data, (size_t)param.n_null_skill_group);
    // index compressed array of null-skill-BY-group
    NCAT idx = 0;
	for(g=0; g<param.nG; g++)
        if( count_null_skill_group[g] >0 ) index_null_skill_group[g] = idx++;
    
	// Pass C
	NDAT *k_countg = Calloc(NDAT, (size_t)param.nK); // track current group in skill
	NDAT *g_countk = Calloc(NDAT, (size_t)param.nG); // track current skill in group
    // set k_countg and g_countk pointers to relative positions
    NDAT sumk=0, sumg=0;
    for(k=0; k<param.nK; k++) {
        k_countg[k] = sumk;
        param.k_g_data[k] = &param.k_data[sumk];
        sumk += param.k_numg[k];
    }
    for(g=0; g<param.nG; g++) {
        g_countk[g] = sumg;
        param.g_k_data[g] = &param.g_data[sumg];
        sumg += param.g_numk[g];
    }
    
    NDAT n_all_data = 0;
	for(t=0; t<param.N; t++) {
        NCAT *ar;
        int n;
        if(param.multiskill==0) {
            k = param.dat_skill->get(t);
            ar = &k;
            n = 1;
        } else {
            ar = &param.dat_multiskill->get(t)[1];
            n = param.dat_multiskill->get(t)[0];
        }
        for(int l=0; l<n; l++) {
            k = ar[l];
            g = param.dat_group->get(t);//[t];
            // now allocate space for the data
            if( k < 0 ) {
                NCAT gidx = index_null_skill_group[g];
                if( param.null_skills[gidx].ix != NULL) // was obs // check if we allocated it already
                    continue;
                param.null_skills[gidx].n = count_null_skill_group[g];
                param.null_skills[gidx].g = g;
                param.null_skills[gidx].cnt = 0;
                //                param.null_skills[gidx].obs = Calloc(NPAR, count_null_skill_group[g]);
                param.null_skills[gidx].ix = Calloc(NDAT, (size_t)count_null_skill_group[g]);
                // no time for null skills is necessary
                //                if(param.time)
                //                    param.null_skills[gidx].time = Calloc(int, (size_t)count_null_skill_group[g]);
                param.null_skills[gidx].alpha = NULL;
                param.null_skills[gidx].beta = NULL;
                param.null_skills[gidx].gamma = NULL;
                param.null_skills[gidx].xi = NULL;
                param.null_skills[gidx].c = NULL;
                param.null_skills[gidx].time = NULL;
                param.null_skills[gidx].p_O_param = 0.0;
                continue;
            }
            if( skill_group_map[k][g]==0)
                printf("ERROR! position [%d,%d] in skill_group_map should have been 1\n",k,g);
            else if( skill_group_map[k][g]==1 ) { // insert new sequence and grab new data
                // link
                param.k_data[ k_countg[k] ] = &param.all_data[n_all_data]; // in linear array
                param.g_data[ g_countk[g] ] = &param.all_data[n_all_data]; // in linear array
                param.all_data[n_all_data].n = 1; // init data << VV
                param.all_data[n_all_data].k = k; // init k
                param.all_data[n_all_data].g = g; // init g
                param.all_data[n_all_data].cnt = 0;
                //                param.all_data[n_all_data].obs = NULL;
                param.all_data[n_all_data].ix = NULL;
                param.all_data[n_all_data].alpha = NULL;
                param.all_data[n_all_data].beta = NULL;
                param.all_data[n_all_data].gamma = NULL;
                param.all_data[n_all_data].xi = NULL;
                param.all_data[n_all_data].c = NULL;
                param.all_data[n_all_data].p_O_param = 0.0;
                param.all_data[n_all_data].loglik = 0.0;
                k_countg[k]++; // count
                g_countk[g]++; // count
                skill_group_map[k][g] = 2; // mark
                n_all_data++;
            } else if( skill_group_map[k][g]== 2) { // update data count, LINEAR SEARCH :(
                int gidx;
                //                for(gidx=(k_countg[k]-(NCAT)1); gidx>=0 && param.k_g_data[k][gidx]->g!=g; gidx--)
                //                    ;
                for(gidx=(k_countg[k]-1); gidx>=0 && param.k_data[gidx]->g!=g; gidx--)
                    ;
                if( param.k_data[gidx]->g==g)
                    param.k_data[gidx]->n++;
                else
                    printf("ERROR! position of group %d in skill %d not found\n",g,k);
            }
        }
	}
	// recycle
	free(k_countg);
	free(g_countk);
    free(count_null_skill_group);
    
    //	3 fill data
    //		pass A
    //			fill k_g_data.data (g_k_data is already linked)
    //				using skill_group_map as marker, 3 - data grabbed
	k_countg = Calloc(NDAT, (size_t)param.nK); // track current group in skill
	g_countk = Calloc(NDAT, (size_t)param.nG); // track current skill in group
	for(t=0; t<param.N; t++) {
		g = param.dat_group->get(t);
		o = param.dat_obs->get(t);
        if(param.time)
            tm = param.dat_time->get(t);
        NCAT *ar;
        int n;
        if(param.multiskill==0) {
            k = param.dat_skill->get(t);
            ar = &k;
            n = 1;
        } else {
            ar = &param.dat_multiskill->get(t)[1];
            n = param.dat_multiskill->get(t)[0];
        }
        for(int l=0; l<n; l++) {
            k = ar[l];
            if( k < 0 ) {
                NCAT gidx = index_null_skill_group[g];
                //                param.null_skills[gidx].obs[ param.null_skills[gidx].cnt++ ] = o; // use .cnt as counter
                param.null_skills[gidx].ix[ param.null_skills[gidx].cnt++ ] = t; // use .cnt as counter
                continue;
            }
            if( skill_group_map[k][g]<2)
                printf("ERROR! position [%d,%d] in skill_group_map should have been 2\n",k,g);
            else if( skill_group_map[k][g]==2 ) { // grab data and insert first dat point
                param.k_g_data[k][ k_countg[k] ]->ix = Calloc(NDAT, (size_t)param.k_g_data[k][ k_countg[k] ]->n); // grab
                if(param.time)
                    param.k_g_data[k][ k_countg[k] ]->time = Calloc(int, (size_t)param.k_g_data[k][ k_countg[k] ]->n); // grab
                param.k_g_data[k][ k_countg[k] ]->ix[0] = t; // insert
                if(param.time)
                    param.k_g_data[k][ k_countg[k] ]->time[0] = tm; // insert
                param.k_g_data[k][ k_countg[k] ]->cnt++; // increase data counter
                k_countg[k]++; // count unique groups forward
                g_countk[g]++; // count unique skills forward
                skill_group_map[k][g] = 3; // mark
            } else if( skill_group_map[k][g]== 3) { // insert next data point and inc counter, LINEAR SEARCH :(
                NCAT gidx;
                //			for(gidx=0; gidx < k_countg[k] && param.k_g_data[k][gidx]->g!=g; gidx++)
                for(gidx=(k_countg[k]-(NCAT)1); gidx>=0 && param.k_g_data[k][gidx]->g!=g; gidx--)
                    ; // skip
                if( param.k_g_data[k][gidx]->g==g ) {
                    NDAT pos = param.k_g_data[k][ gidx ]->cnt; // copy position
                    //                    param.k_g_data[k][ gidx ]->obs[pos] = o; // insert
                    param.k_g_data[k][ gidx ]->ix[pos] = t; // insert
                    if(param.time)
                        param.k_g_data[k][ gidx ]->time[pos] = tm; // insert
                    param.k_g_data[k][ gidx ]->cnt++; // increase data counter
                }
                else
                    printf("ERROR! position of group %d in skill %d not found\n",g,k);
            }
        }
    }
	// recycle
	free(k_countg);
	free(g_countk);
    free(index_null_skill_group);
	free2D<NPAR>(skill_group_map, param.nK);
    // reset `cnt'
    for(g=0; g<param.nG; g++) // for all groups
        for(k=0; k<param.g_numk[g]; k++) // for all skills in it
            param.g_k_data[g][k]->cnt = 0;
    for(NCAT x=0; x<param.n_null_skill_group; x++)
        param.null_skills[x].cnt = 0;
    return true;
}

void cross_validate(NUMBER* metrics, const char *filename) {
    NUMBER rmse = 0.0;
    NUMBER rmse_no_null = 0.0, accuracy = 0.0, accuracy_no_null = 0.0;
    NPAR f;
    NCAT g,k;
    FILE *fid = NULL; // file for storing prediction should that be necessary
    if(param.predictions>0) {  // if we have to write the predictions file
        fid = fopen(filename,"w");
        if(fid == NULL)
        {
            fprintf(stderr,"Can't write output model file %s\n",filename);
            exit(1);
        }
    }
    // produce folds
    NPAR *folds = Calloc(NPAR, (size_t)param.nG);//[param.nG];
    srand ( (unsigned int)time(NULL) );
    for(g=0; g<param.nG; g++) folds[g] = rand() % param.cv_folds;
    // create and fit multiple problems
    HMMProblem* hmms[param.cv_folds];
    int q = param.quiet;
    param.quiet = 1;
    for(f=0; f<param.cv_folds; f++) {
//        switch(param.structure)
//        {
//            case STRUCTURE_SKILL:
//            case STRUCTURE_GROUP: 
                hmms[f] = new HMMProblem(&param);
//        }
        // block respective data - do not fit the data belonging to the fold
        for(g=0; g<param.nG; g++) // for all groups
            if(folds[g]==f) { // if in current fold
                for(k=0; k<param.g_numk[g]; k++) // for all skills in it
                    param.g_k_data[g][k]->cnt = 1; // block it
            }
        // block nulls
        for(NCAT x=0; x<param.n_null_skill_group; x++) {
            if( param.null_skills[x].g == f)
                param.null_skills[x].cnt = 1;
        }
        // now compute
        hmms[f]->fit();
        // UN-block respective data
        for(g=0; g<param.nG; g++) // for all groups
            if(folds[g]==f) { // if in current fold
                for(k=0; k<param.g_numk[g]; k++) // for all skills in it
                    param.g_k_data[g][k]->cnt = 0; // UN-block it
            }
        // UN-block nulls
        for(NCAT x=0; x<param.n_null_skill_group; x++) {
            if( param.null_skills[x].g == f)
                param.null_skills[x].cnt = 0;
        }
        if(q == 0)
            printf("fold %d is done\n",f+1);
    }
    param.quiet = (NPAR)q;
    // go trhough original data and predict
	NDAT t;
	NPAR i, j, m, o, isTarget;
	NUMBER *local_pred = init1D<NUMBER>(param.nO); // local prediction
	NUMBER pLe[param.nS];// p(L|evidence);
	NUMBER pLe_denom; // p(L|evidence) denominator
	NUMBER ***group_skill_map = init3D<NUMBER>(param.nG, param.nK, param.nS); // knowledge states
    NUMBER prob = 0, ll = 0;
    struct data dt;
	// initialize
	for(g=0; g<param.nG; g++) {
        dt.g = g;
        f = folds[g];
		for(k=0; k<param.nK; k++) {
            dt.k = k;
			for(i=0; i<param.nO; i++)
                group_skill_map[g][k][i] = hmms[f]->getPI(&dt,i);//PI[i];
		}
    }
	// deal with null skill
	for(t=0; t<param.N; t++) {
		o = param.dat_obs->get(t);//[t]; correct: obs 1 (0 code), incorect obs 2 (1 code), hence 1-code is the conversion
        isTarget = (NPAR)(param.cv_target_obs == o);
		g = param.dat_group->get(t);//[t];
        dt.g = g;
        f = folds[g];
        
        NCAT *ar;
        int n;
        if(param.multiskill==0) {
            k = param.dat_skill->get(t);
            ar = &k;
            n = 1;
        } else {
            ar = &param.dat_multiskill->get(t)[1];
            n = param.dat_multiskill->get(t)[0];
        }
        if(ar[0]<0) { // if no skill label
            rmse += pow(isTarget-hmms[f]->getNullSkillObs(param.cv_target_obs),2);
            accuracy += isTarget == (hmms[f]->getNullSkillObs(param.cv_target_obs)>=0.5);
            
            prob = safe0num(hmms[f]->getNullSkillObs(param.cv_target_obs));
            ll -= safelog(  prob)*   isTarget  +  safelog(1-prob)*(1-isTarget);
            if(param.predictions>0) // write predictions file if it was opened
                for(m=0; m<param.nO; m++)
                    fprintf(fid,"%10.8f%s",hmms[f]->getNullSkillObs(m),(m<(param.nO-1))?"\t":"\n");
            continue;
        }
        hmms[f]->producePCorrect(group_skill_map, local_pred, ar, n, &dt);
        for(int l=0; l<n; l++) {
            k = ar[l];
            dt.k = k;
            //            // produce prediction and copy to result
            //            for(m=0; m<param.nO; m++)
            //                for(i=0; i<param.nS; i++)
            //                    local_pred[m] += group_skill_map[g][k][i] * hmms[f]->getB(&dt,i,m);
            // update p(L)
            pLe_denom = 0.0;
            // 1. pLe =  (L .* B(:,o)) ./ ( L'*B(:,o)+1e-8 );
            for(i=0; i<param.nS; i++) pLe_denom += group_skill_map[g][k][i] * hmms[f]->getB(&dt,i,o);
            for(i=0; i<param.nS; i++) pLe[i] = group_skill_map[g][k][i] * hmms[f]->getB(&dt,i,o) / safe0num(pLe_denom);
            // 2. L = (pLe'*A)';
            for(i=0; i<param.nS; i++) group_skill_map[g][k][i] = 0.0;
            for(j=0; j<param.nS; j++)
                for(i=0; i<param.nS; i++)
                    group_skill_map[g][k][j] += pLe[i] * hmms[f]->getA(&dt,i,j);
        }
        //        for(m=0; m<param.nO; m++)
        //            local_pred[m] /= n;
        if(param.predictions>0) // write predictions file if it was opened
            for(m=0; m<param.nO; m++)
                fprintf(fid,"%10.8f%s",local_pred[m],(m<(param.nO-1))?"\t":"\n");
        rmse += pow(isTarget-local_pred[param.cv_target_obs],2);
        rmse_no_null += pow(isTarget-local_pred[param.cv_target_obs],2);
        accuracy += isTarget == (local_pred[param.cv_target_obs]>=0.5);
        accuracy_no_null += isTarget == (local_pred[param.cv_target_obs]>=0.5);
        prob = safe01num(local_pred[param.metrics_target_obs]);
        ll -= safelog(  prob)*   isTarget  +  safelog(1-prob)*(1-isTarget);
	} // for all data
    rmse = sqrt( rmse / param.N );
    rmse_no_null = sqrt( rmse_no_null / (param.N - param.N_null) );
    
    // delete problems
    NCAT n_par = 0;
    for(f=0; f<param.cv_folds; f++) {
        n_par += hmms[f]->getNparams();
        delete hmms[f];
    }
    n_par /= f;
    free(folds);
    free(local_pred);
    free3D<NUMBER>(group_skill_map, param.nG, param.nK);
    if(param.predictions>0) // close predictions file if it was opened
        fclose(fid);
    metrics[0] = ll;
    metrics[1] = 2*(n_par) + 2*ll;
    metrics[2] = n_par*safelog(param.N) + 2*ll;
    metrics[3] = rmse;
    metrics[4] = rmse_no_null;
    metrics[5] = accuracy / param.N;
    metrics[6] = accuracy_no_null / (param.N - param.N_null);
}

void cross_validate_item(NUMBER* metrics, const char *filename) {
    NUMBER rmse = 0.0, rmse_no_null = 0.0, accuracy = 0.0, accuracy_no_null = 0.0;
    NPAR f;
    NCAT g,k;
    NCAT I; // item
    NDAT t;
    FILE *fid = NULL; // file for storing prediction should that be necessary
    if(param.predictions>0) {  // if we have to write the predictions file
        fid = fopen(filename,"w");
        if(fid == NULL)
        {
            fprintf(stderr,"Can't write output model file %s\n",filename);
            exit(1);
        }
    }
    // produce folds
    NPAR *folds = Calloc(NPAR, (size_t)param.nI);
    NDAT *fold_counts = Calloc(NDAT, (size_t)param.cv_folds);
    //    NDAT *fold_shortcounts = Calloc(NDAT, (size_t)param.cv_folds);
    srand ( (unsigned int)time(NULL) ); // randomize
    for(I=0; I<param.nI; I++) folds[I] = rand() % param.cv_folds; // produce folds
    // count number of items in each fold
    //    for(I=0; I<param.nI; I++) fold_shortcounts[ folds[I] ]++; // produce folds
    for(t=0; t<param.N; t++) fold_counts[ folds[param.dat_item->get(t)] ]++;
    // create and fit multiple problems
    HMMProblem* hmms[param.cv_folds];
    int q = param.quiet;
    param.quiet = 1;
    for(f=0; f<param.cv_folds; f++) {
//        switch(param.structure)
//        {
//            case STRUCTURE_SKILL:
//            case STRUCTURE_GROUP:
                hmms[f] = new HMMProblem(&param);
//                break;
//        }
        // block respective data - do not fit the data belonging to the fold
        NPAR *saved_obs = Calloc(NPAR, (size_t)fold_counts[f]);
        NDAT count_saved = 0;
        for(t=0; t<param.N; t++) {
            if( folds[ param.dat_item->get(t) ] == f ) {
                saved_obs[count_saved++] = param.dat_obs->get(t);
                param.dat_obs->set(t, -1);
            }
        }
        // now compute
        hmms[f]->fit();
        
        // UN-block respective data
        count_saved = 0;
        for(t=0; t<param.N; t++)
            if( folds[ param.dat_item->get(t) ] == f )
                param.dat_obs->set(t, saved_obs[count_saved++]);
        free(saved_obs);
        if(q == 0)
            printf("fold %d is done\n",f+1);
    }
    free(fold_counts);
    param.quiet = (NPAR)q;
    // go trhough original data and predict
	NPAR i, j, m, o, isTarget;
	NUMBER *local_pred = init1D<NUMBER>(param.nO); // local prediction
	NUMBER pLe[param.nS];// p(L|evidence);
	NUMBER pLe_denom; // p(L|evidence) denominator
	NUMBER ***group_skill_map = init3D<NUMBER>(param.nG, param.nK, param.nS); // knowledge states
    NUMBER prob = 0, ll = 0;
    struct data dt;
	// initialize
	for(g=0; g<param.nG; g++) {
        dt.g = g;
        f = folds[g];
		for(k=0; k<param.nK; k++) {
            dt.k = k;
			for(i=0; i<param.nO; i++)
                group_skill_map[g][k][i] = hmms[f]->getPI(&dt,i);//PI[i];
		}
    }
	// deal with null skill
	for(t=0; t<param.N; t++) {
		o = param.dat_obs->get(t);//[t]; correct: obs 1 (0 code), incorect obs 2 (1 code), hence 1-code is the conversion
        isTarget = (NPAR)(param.cv_target_obs == o);
		g = param.dat_group->get(t);//[t];
        dt.g = g;
        f = folds[g];
        
        NCAT *ar;
        int n;
        if(param.multiskill==0) {
            k = param.dat_skill->get(t);
            ar = &k;
            n = 1;
        } else {
            ar = &param.dat_multiskill->get(t)[1];
            n = param.dat_multiskill->get(t)[0];
        }
        if(ar[0]<0) { // if no skill label
            rmse += pow(isTarget-hmms[f]->getNullSkillObs(param.cv_target_obs),2);
            accuracy += isTarget == (hmms[f]->getNullSkillObs(param.cv_target_obs)>=0.5);
            
            prob = safe0num(hmms[f]->getNullSkillObs(param.cv_target_obs));
            ll -= safelog(  prob)*   isTarget  +  safelog(1-prob)*(1-isTarget);
            if(param.predictions>0) // write predictions file if it was opened
                for(m=0; m<param.nO; m++)
                    fprintf(fid,"%10.8f%s",hmms[f]->getNullSkillObs(m),(m<(param.nO-1))?"\t":"\n");
            continue;
        }
        hmms[f]->producePCorrect(group_skill_map, local_pred, ar, n, &dt);
        for(int l=0; l<n; l++) {
            k = ar[l];
            dt.k = k;
            // produce prediction and copy to result
            pLe_denom = 0.0;
            // 1. pLe =  (L .* B(:,o)) ./ ( L'*B(:,o)+1e-8 );
            for(i=0; i<param.nS; i++) pLe_denom += group_skill_map[g][k][i] * hmms[f]->getB(&dt,i,o);
            for(i=0; i<param.nS; i++) pLe[i] = group_skill_map[g][k][i] * hmms[f]->getB(&dt,i,o) / safe0num(pLe_denom);
            // 2. L = (pLe'*A)';
            for(i=0; i<param.nS; i++) group_skill_map[g][k][i] = 0.0;
            for(j=0; j<param.nS; j++)
                for(i=0; i<param.nS; i++)
                    group_skill_map[g][k][j] += pLe[i] * hmms[f]->getA(&dt,i,j);
        }
        if(param.predictions>0) // write predictions file if it was opened
            for(m=0; m<param.nO; m++)
                fprintf(fid,"%10.8f%s",local_pred[m],(m<(param.nO-1))?"\t":"\n");
        rmse += pow(isTarget-local_pred[param.cv_target_obs],2);
        rmse_no_null += pow(isTarget-local_pred[param.cv_target_obs],2);
        accuracy += isTarget == (local_pred[param.cv_target_obs]>=0.5);
        accuracy_no_null += isTarget == (local_pred[param.cv_target_obs]>=0.5);
        prob = safe01num(local_pred[param.metrics_target_obs]);
        ll -= safelog(  prob)*   isTarget  +  safelog(1-prob)*(1-isTarget);
	} // for all data
    rmse = sqrt( rmse / param.N );
    rmse_no_null = sqrt( rmse_no_null / (param.N - param.N_null) );
    // delete problems
    NCAT n_par = 0;
    for(f=0; f<param.cv_folds; f++) {
        n_par += hmms[f]->getNparams();
        delete hmms[f];
    }
    n_par /= f;
    free(folds);
    //    free(fold_shortcounts);
    free(local_pred);
    free3D<NUMBER>(group_skill_map, param.nG, param.nK);
    if(param.predictions>0) // close predictions file if it was opened
        fclose(fid);
    metrics[0] = ll;
    metrics[1] = 2*(n_par) + 2*ll;
    metrics[2] = n_par*safelog(param.N) + 2*ll;
    metrics[3] = rmse;
    metrics[4] = rmse_no_null;
    metrics[5] = accuracy / param.N;
    metrics[6] = accuracy_no_null / (param.N - param.N_null);
}

void cross_validate_nstrat(NUMBER* metrics, const char *filename) {
    NUMBER rmse = 0.0;
    NUMBER rmse_no_null = 0.0, accuracy = 0.0, accuracy_no_null = 0.0;
    NPAR f;
    NCAT g,k;
    NCAT I; // item
    NDAT t;
    FILE *fid = NULL; // file for storing prediction should that be necessary
    if(param.predictions>0) {  // if we have to write the predictions file
        fid = fopen(filename,"w");
        if(fid == NULL)
        {
            fprintf(stderr,"Can't write output model file %s\n",filename);
            exit(1);
        }
    }
    // produce folds
    NPAR *folds = Calloc(NPAR, (size_t)param.nI);
    NDAT *fold_counts = Calloc(NDAT, (size_t)param.cv_folds);
    //    NDAT *fold_shortcounts = Calloc(NDAT, (size_t)param.cv_folds);
    srand ( (unsigned int)time(NULL) ); // randomize
    for(I=0; I<param.nI; I++) folds[I] = rand() % param.cv_folds; // produce folds
    // count number of items in each fold
    for(t=0; t<param.N; t++)  fold_counts[ folds[param.dat_item->get(t)] ]++;
    // create and fit multiple problems
    HMMProblem* hmms[param.cv_folds];
    int q = param.quiet;
    param.quiet = 1;
    for(f=0; f<param.cv_folds; f++) {
//        switch(param.structure)
//        {
//            case STRUCTURE_SKILL: // Conjugate Gradient Descent
//            case STRUCTURE_GROUP: // Conjugate Gradient Descent
                hmms[f] = new HMMProblem(&param);
//                break;
//        }
        // block respective data - do not fit the data belonging to the fold
        NPAR *saved_obs = Calloc(NPAR, (size_t)fold_counts[f]);
        NDAT count_saved = 0;
        for(t=0; t<param.N; t++) {
            if( folds[ param.dat_item->get(t) ] == f ) {
                saved_obs[count_saved++] = param.dat_obs->get(t);
                param.dat_obs->set(t, -1);
            }
        }
        // now compute
        hmms[f]->fit();
        
        // UN-block respective data
        count_saved = 0;
        for(t=0; t<param.N; t++)
            if( folds[ param.dat_item->get(t) ] == f )
                param.dat_obs->set(t, saved_obs[count_saved++]);
        free(saved_obs);
        if(q == 0)
            printf("fold %d is done\n",f+1);
    }
    free(fold_counts);
    param.quiet = (NPAR)q;
    // go trhough original data and predict
	NPAR i, j, m, o, isTarget;
	NUMBER *local_pred = init1D<NUMBER>(param.nO); // local prediction
	NUMBER pLe[param.nS];// p(L|evidence);
	NUMBER pLe_denom; // p(L|evidence) denominator
	NUMBER ***group_skill_map = init3D<NUMBER>(param.nG, param.nK, param.nS); // knowledge states
    NUMBER prob = 0, ll = 0;
    struct data dt;
	// initialize
	for(g=0; g<param.nG; g++) {
        dt.g = g;
        f = folds[g];
		for(k=0; k<param.nK; k++) {
            dt.k = k;
			for(i=0; i<param.nO; i++)
                group_skill_map[g][k][i] = hmms[f]->getPI(&dt,i);//PI[i];
		}
    }
	// deal with null skill
	for(t=0; t<param.N; t++) {
		o = param.dat_obs->get(t);//[t]; correct: obs 1 (0 code), incorect obs 2 (1 code), hence 1-code is the conversion
        isTarget = (NPAR)(param.cv_target_obs == o);
		g = param.dat_group->get(t);//[t];
        dt.g = g;
        f = folds[g];
        
        NCAT *ar;
        int n;
        if(param.multiskill==0) {
            k = param.dat_skill->get(t);
            ar = &k;
            n = 1;
        } else {
            ar = &param.dat_multiskill->get(t)[1];
            n = param.dat_multiskill->get(t)[0];
        }
        if(ar[0]<0) { // if no skill label
            rmse += pow(isTarget-hmms[f]->getNullSkillObs(param.cv_target_obs),2);
            accuracy += isTarget == (hmms[f]->getNullSkillObs(param.cv_target_obs)>=0.5);
            
            prob = safe0num(hmms[f]->getNullSkillObs(param.cv_target_obs));
            ll -= safelog(  prob)*   isTarget  +  safelog(1-prob)*(1-isTarget);
            if(param.predictions>0) // write predictions file if it was opened
                for(m=0; m<param.nO; m++)
                    fprintf(fid,"%10.8f%s",hmms[f]->getNullSkillObs(m),(m<(param.nO-1))?"\t":"\n");
            continue;
        }
        hmms[f]->producePCorrect(group_skill_map, local_pred, ar, n, &dt);
        for(int l=0; l<n; l++) {
            k = ar[l];
            dt.k = k;
            // produce prediction and copy to result
            // update p(L)
            pLe_denom = 0.0;
            // 1. pLe =  (L .* B(:,o)) ./ ( L'*B(:,o)+1e-8 );
            for(i=0; i<param.nS; i++) pLe_denom += group_skill_map[g][k][i] * hmms[f]->getB(&dt,i,o);
            for(i=0; i<param.nS; i++) pLe[i] = group_skill_map[g][k][i] * hmms[f]->getB(&dt,i,o) / safe0num(pLe_denom);
            // 2. L = (pLe'*A)';
            for(i=0; i<param.nS; i++) group_skill_map[g][k][i] = 0.0;
            for(j=0; j<param.nS; j++)
                for(i=0; i<param.nS; i++)
                    group_skill_map[g][k][j] += pLe[i] * hmms[f]->getA(&dt,i,j);
        }
        //        for(m=0; m<param.nO; m++)
        //            local_pred[m] /= n;
        if(param.predictions>0) // write predictions file if it was opened
            for(m=0; m<param.nO; m++)
                fprintf(fid,"%10.8f%s",local_pred[m],(m<(param.nO-1))?"\t":"\n");
        rmse += pow(isTarget-local_pred[param.cv_target_obs],2);
        rmse_no_null += pow(isTarget-local_pred[param.cv_target_obs],2);
        accuracy += isTarget == (local_pred[param.cv_target_obs]>=0.5);
        accuracy_no_null += isTarget == (local_pred[param.cv_target_obs]>=0.5);
        prob = safe01num(local_pred[param.metrics_target_obs]);
        ll -= safelog(  prob)*   isTarget  +  safelog(1-prob)*(1-isTarget);
	} // for all data
    rmse = sqrt( rmse / param.N );
    rmse_no_null = sqrt( rmse_no_null / (param.N - param.N_null) );
    // delete problems
    NCAT n_par = 0;
    for(f=0; f<param.cv_folds; f++) {
        n_par += hmms[f]->getNparams();
        delete hmms[f];
    }
    n_par /= f;
    free(folds);
    //    free(fold_shortcounts);
    free(local_pred);
    free3D<NUMBER>(group_skill_map, param.nG, param.nK);
    if(param.predictions>0) // close predictions file if it was opened
        fclose(fid);
    metrics[0] = ll;
    metrics[1] = 2*(n_par) + 2*ll;
    metrics[2] = n_par*safelog(param.N) + 2*ll;
    metrics[3] = rmse;
    metrics[4] = rmse_no_null;
    metrics[5] = accuracy / param.N;
    metrics[6] = accuracy_no_null / (param.N - param.N_null);
}