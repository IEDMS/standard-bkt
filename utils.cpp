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

#include "utils.h"
using namespace std;

// project of others
int compareNumber (const void * a, const void * b) {
	NUMBER cmp = ( *(NUMBER*)a - *(NUMBER*)b );
	return -1*(cmp<0) + 0 + (cmp>0)*1;
}

int compareNumberRev (const void * a, const void * b) {
    //	return ( *(NUMBER*)b - *(NUMBER*)a );
	NUMBER cmp = ( *(NUMBER*)b - *(NUMBER*)a );
	return -1*(cmp<0) + 0 + (cmp>0)*1;
}

void qsortNumber(NUMBER* ar, NPAR size) {
	qsort (ar, (size_t)size, sizeof(NUMBER), compareNumber);
}

void qsortNumberRev(NUMBER* ar, NPAR size) {
	qsort (ar, (size_t)size, sizeof(NUMBER), compareNumberRev);
}

int compareNcat (const void * a, const void * b) {
	int cmp = ( *(NCAT*)a - *(NCAT*)b );
	return -1*(cmp<0) + 0 + (cmp>0)*1;
}


void qsortNcat(NCAT* ar, NPAR size) {
	qsort (ar, (size_t)size, sizeof(NCAT), compareNcat);
}

// refer to http://arxiv.org/abs/1101.6081 for source
void projsimplex(NUMBER* y, NPAR size) {
    
	bool bget = false;
	NUMBER *s = init1D<NUMBER>((NDAT)size);
	cpy1D<NUMBER>(y, s, (NDAT)size);
	
	qsortNumberRev(s, size);
	NUMBER tmpsum = 0, tmax = 0;
    
	for(NPAR i=0; i<(size-1); i++) {
		tmpsum = tmpsum + s[i];
		tmax = (tmpsum - 1)/(i+1);
		if(tmax >= s[i+1]) {
			bget = true;
			break;
		}
	}
	if(!bget) tmax = (tmpsum + s[size-1] -1)/size;
	free(s);
    
	for(NPAR i=0; i<size; i++)
		y[i] = ((y[i]-tmax)<0)?0:(y[i]-tmax);
}


// project of my own
bool issimplex(NUMBER* ar, NPAR size) {
	NUMBER sum = 1;
	for(NPAR i=0; i<size; i++) {
		sum -= ar[i];
		if( ar[i] < 0 || ar[i] > 1)
			return false;
	}
	return fabs(sum)<SAFETY;
}

bool issimplexbounded(NUMBER* ar, NUMBER *lb, NUMBER *ub, NPAR size) {
	NUMBER sum = 1;
	for(NPAR i=0; i<size; i++) {
		sum -= ar[i];
		if( ar[i] < lb[i] || ar[i] > ub[i])
			return false;
	}
	return fabs(sum)<SAFETY;
}


void projectsimplex(NUMBER* ar, NPAR size) {
	NPAR i, num_at_hi, num_at_lo; // number of elements at lower,upper boundary
	NPAR *at_hi = Calloc(NPAR, (size_t)size);
	NPAR *at_lo = Calloc(NPAR, (size_t)size);
	NUMBER err, lambda;
	while( !issimplex(ar, size)) {
        lambda = 0;
		num_at_hi = 0;
		num_at_lo = 0;
		err = -1; // so that of the sum is over 1, the error is 1-sum
		// threshold
		for(i=0; i<size; i++) {
			at_lo[i] = (ar[i]<0)?1:0;
			ar[i] = (at_lo[i]==1)?0:ar[i];
			num_at_lo += at_lo[i];
            
			at_hi[i] = (ar[i]>1)?1:0;
			ar[i] = (at_hi[i]==1)?1:ar[i];
			num_at_hi += at_hi[i];
			
			err += ar[i];
		}
		if (size > (num_at_hi + num_at_lo) )
			lambda = err / (size - (num_at_hi + num_at_lo));
		for(i=0; i<size; i++)
			ar[i] -= (at_lo[i]==0 && at_hi[i]==0)?lambda:0;
		
	} // until satisfied
	free(at_hi);
	free(at_lo);
}

void projectsimplexbounded(NUMBER* ar, NUMBER *lb, NUMBER *ub, NPAR size) {
	NPAR i, num_at_hi, num_at_lo; // number of elements at lower,upper boundary
	NPAR *at_hi = Calloc(NPAR, (size_t)size);
	NPAR *at_lo = Calloc(NPAR, (size_t)size);
	NUMBER err, lambda;
	while( !issimplexbounded(ar, lb, ub, size)) {
        lambda = 0;
		num_at_hi = 0;
		num_at_lo = 0;
		err = -1;
		// threshold
		for(i=0; i<size; i++) {
			at_lo[i] = (ar[i]<lb[i])?1:0;
			ar[i] = (at_lo[i]==1)?lb[i]:ar[i];
			num_at_lo += at_lo[i];
			
			at_hi[i] = (ar[i]>ub[i])?1:0;
			ar[i] = (at_hi[i]==1)?ub[i]:ar[i];
			num_at_hi += at_hi[i];
			
			err += ar[i];
		}
		if (size > (num_at_hi + num_at_lo) )
			lambda = err / (size - (num_at_hi + num_at_lo));
		for(i=0; i<size; i++)
			ar[i] -= (at_lo[i]==0 && at_hi[i]==0)?lambda:0;
		
	} // until satisfied
	free(at_hi);
	free(at_lo);
}

NUMBER safe01num(NUMBER val) {
    return (val<=0)? SAFETY : ( (val>=1)? (1-SAFETY) : val );
}

NUMBER safe0num(NUMBER val) {
    return (val<SAFETY)?SAFETY:val;
}

NUMBER safelog(NUMBER val) {
	return log(val + (val<=0)*SAFETY);
}

void add1DNumbersWeighted(NUMBER* sourse, NUMBER* target, NPAR size, NUMBER weight) {
	for(NPAR i=0; i<size; i++)
		target[i] = target[i] + sourse[i]*weight;
}

void add2DNumbersWeighted(NUMBER** sourse, NUMBER** target, NPAR size1, NPAR size2, NUMBER weight) {
	for(NPAR i=0; i<size1; i++)
		for(NPAR j=0; j<size2; j++)
			target[i][j] = target[i][j] + sourse[i][j]*weight;
}

bool isPasses(NUMBER* ar, NPAR size) {
	NUMBER sum = 0;
	for(NPAR i=0; i<size; i++) {
		if( ar[i]<0 || ar[i]>1)
			return false;
		sum += ar[i];
	}
	return sum==1;
}

bool isPassesLim(NUMBER* ar, NPAR size, NUMBER *lb, NUMBER* ub) {
	NUMBER sum = 0;
	for(NPAR i=0; i<size; i++) {
		if( (lb[i]-ar[i])>SAFETY || (ar[i]-ub[i])>SAFETY )
			return false;
		sum += ar[i];
	}
	return fabs(sum-1)<SAFETY;
}


// Gentle - as per max distance to go toward extreme value of 0 or 1
NUMBER doLog10Scale1DGentle(NUMBER *grad, NUMBER *par, NPAR size) {
	NPAR i;
	NUMBER max_10_scale = 0, candidate, min_delta = 1, max_grad = 0;
	for(i=0; i<size; i++) {
		if( fabs(grad[i]) < SAFETY ) // 0 gradient
			continue;
        // scale
        if(max_grad < fabs(grad[i]))
            max_grad = fabs(grad[i]);
		candidate = ceil( log10( fabs(grad[i]) ) );
		if(candidate > max_10_scale)
			max_10_scale = candidate;
        // delta: if grad<0 - distance to 1, if grad>0 - distance to 0
        candidate = (grad[i]<0)*(1-par[i]) + (grad[i]>0)*(par[i]) +
        ( (fabs(par[i])< SAFETY) || (fabs(1-par[i])< SAFETY) ); // these terms are there to avoid already extreme 0, 1 values;
        if( candidate < min_delta)
            min_delta = candidate;
	}
    max_grad = max_grad / pow(10, max_10_scale);
    if(max_10_scale > 0)
        for(i=0; i<size; i++)
            grad[i] = ( 0.95 * min_delta / max_grad) * grad[i] / pow(10, max_10_scale);
    return ( 0.95 * min_delta / max_grad ) / pow(10, max_10_scale);
}

// Gentle - as per max distance to go toward extreme value of 0 or 1
NUMBER doLog10Scale2DGentle(NUMBER **grad, NUMBER **par, NPAR size1, NPAR size2) {
	NPAR i,j;
	NUMBER max_10_scale = 0, candidate, min_delta = 1, max_grad = 0;
	for(i=0; i<size1; i++)
		for(j=0; j<size2; j++) {
			if( fabs(grad[i][j]) < SAFETY ) // 0 gradient
				continue;
            // scale
            if(max_grad < fabs(grad[i][j]))
                max_grad = fabs(grad[i][j]);
			candidate = ceil( log10( fabs(grad[i][j]) ) );
			if(candidate > max_10_scale)
				max_10_scale = candidate;
            // delta: if grad<0 - distance to 1, if grad>0 - distance to 0
            candidate = (grad[i][j]<0)*(1-par[i][j]) + (grad[i][j]>0)*(par[i][j]) +
            ( (fabs(par[i][j])< SAFETY) || (fabs(1-par[i][j])< SAFETY) ); // these terms are there to avoid already extreme 0, 1 values
            if( candidate < min_delta)
                min_delta = candidate;
		}
    max_grad = max_grad / pow(10, max_10_scale);
    if(max_10_scale >0 )
		for(i=0; i<size1; i++)
			for(j=0; j<size2; j++)
				grad[i][j] = ( 0.95 * min_delta / max_grad) * grad[i][j] / pow(10, max_10_scale);
	return ( 0.95 * min_delta / max_grad ) / pow(10, max_10_scale);
}



// for skill of group
void zeroLabels(NCAT xdat, struct data** x_data) { // set counts in data sequences to zero
	NCAT x;
	for(x=0; x<xdat; x++)
		x_data[x][0].cnt = 0;
}

//
// The heavy end - common functionality
//

void set_param_defaults(struct param *param) {
    param->item_complexity = NULL;
	// configurable - set
	param->tol                   = 0.01;
	param->time                  = 0;
	param->maxiter               = 200;
	param->quiet                 = 0;
	param->single_skill          = 0;
	param->structure			 = 1; // default is by skill
	param->solver				 = 2; // default is Gradient Descent
	param->solver_setting		 = -1; // -1 - not set
    param->metrics               = 0;
    param->metrics_target_obs    = 0;
    param->predictions           = 0;
    param->binaryinput           = 0;
	param->C                     = 0;
    param->initfile[0]           = 0; // 1st bit is 0 - no file
	param->init_params			 = Calloc(NUMBER, (size_t)5);
	param->init_params[0] = 0.5; // PI[0]
	param->init_params[1] = 1.0; // p(not forget)
	param->init_params[2] = 0.4; // p(learn)
	param->init_params[3] = 0.8; // p(not slip)
	param->init_params[4] = 0.2; // p(guess)
	param->param_lo				= Calloc(NUMBER, (size_t)10);
	param->param_lo[0] = 0; param->param_lo[1] = 0; param->param_lo[2] = 1; param->param_lo[3] = 0; param->param_lo[4] = 0;
	param->param_lo[5] = 0; param->param_lo[6] = 0; param->param_lo[7] = 0; param->param_lo[8] = 0; param->param_lo[9] = 0;
	param->param_hi				= Calloc(NUMBER, (size_t)10);
	param->param_hi[0] = 1.0; param->param_hi[1] = 1.0; param->param_hi[2] = 1.0; param->param_hi[3] = 0.0; param->param_hi[4] = 1.0;
	param->param_hi[5] = 1.0; param->param_hi[6] = 1.0; param->param_hi[7] = 0.3; param->param_hi[8] = 0.3; param->param_hi[9] = 1.0;
	param->cv_folds = 0;
	param->cv_strat = 'g'; // default group(student)-stratified
    param->cv_target_obs = 0; // 1st state to validate agains by default, cv_folds enables cross-validation
    param->multiskill = 0; // single skill per ovservation by default
    // vocabilaries
    param->map_group_fwd = NULL;
    param->map_group_bwd = NULL;
    param->map_step_fwd = NULL;
    param->map_step_bwd = NULL;
    param->map_skill_fwd = NULL;
    param->map_skill_bwd = NULL;
	// derived from data - set to 0
	param->N  = 0; //will be dynamically set in read_data_...()
	param->nS = 2;
	param->nO = 0;
	param->nG = 0;
	param->nK = 0;
	param->nI = 0;
	// data
    param->all_data = NULL;
    param->nSeq = 0;
	param->k_numg = NULL;
	param->k_data = NULL;
	param->k_g_data = NULL;
	param->g_numk = NULL;
	param->g_data = NULL;
	param->g_k_data = NULL;
    param->N_null = 0;
    param->n_null_skill_group = 0;
    param->null_skills = NULL;
	// fitting specific - Armijo rule
	param->ArmijoC1            = 1e-4;
	param->ArmijoC2            = 0.9;
	param->ArmijoReduceFactor  = 2;//1/0.9;//
	param->ArmijoSeed          = 1; //1; - since we use smooth stepping 1 is the only thing we need
    param->ArmijoMinStep       = 0.001; //  0.000001~20steps, 0.001~10steps
}

void destroy_input_data(struct param *param) {
    if(param->item_complexity) free(param->item_complexity);
	if(param->init_params != NULL) free(param->init_params);
	if(param->param_lo != NULL) free(param->param_lo);
	if(param->param_hi != NULL) free(param->param_hi);
	
    // data - checks if pointers to data are null anyway (whether we delete linear columns of data or not)
	if(param->dat_obs != NULL) delete param->dat_obs;
	if(param->dat_group != NULL) delete param->dat_group;
	if(param->dat_item != NULL) delete param->dat_item;
	if(param->dat_skill != NULL) delete param->dat_skill;
	if(param->dat_multiskill != NULL) delete param->dat_multiskill;
	if(param->dat_time != NULL) delete param->dat_time;
    
    // not null skills
    for(NDAT kg=0;kg<param->nSeq; kg++) {
        free(param->all_data[kg].ix); // was obs;
        if(param->time)
            free(param->all_data[kg].time);
    }
    if(param->all_data != NULL) free(param->all_data); // ndat of them
    if(param->k_data != NULL)   free(param->k_data); // ndat of them (reordered by k)
    if(param->g_data != NULL)   free(param->g_data); // ndat of them (reordered by g)
    if(param->k_g_data != NULL) free(param->k_g_data); // nK of them
    if(param->g_k_data != NULL) free(param->g_k_data); // nG of them
    
	if(param->k_numg != NULL)   free(param->k_numg);
	if(param->g_numk != NULL)   free(param->g_numk);
    // null skills
    for(NCAT g=0;g<param->n_null_skill_group; g++)
        free(param->null_skills[g].ix); // was obs
    if(param->null_skills != NULL) free(param->null_skills);
    // vocabularies
    delete param->map_group_fwd;
    delete param->map_group_bwd;
    delete param->map_step_fwd;
    delete param->map_step_bwd;
    delete param->map_skill_fwd;
    delete param->map_skill_bwd;
}


//
// read/write solver info to a file
//
void writeSolverInfo(FILE *fid, struct param *param) {
    // solver id
    if( param->solver_setting>0 ) {
        fprintf(fid,"SolverId\t%d.%d.%d\n",param->structure,param->solver,param->solver_setting);
    } else {
        fprintf(fid,"SolverId\t%d.%d\n",param->structure,param->solver);
    }
	// nK
    fprintf(fid,"nK\t%d\n",param->nK);
    // nG
    fprintf(fid,"nG\t%d\n",param->nG);
    // nS
    fprintf(fid,"nS\t%d\n",param->nS);
    // nO
    fprintf(fid,"nO\t%d\n",param->nO);
}

void readSolverInfo(FILE *fid, struct param *param, NDAT *line_no) {
    string s;
    int c, i1, i2;
    // SolverId
    fscanf(fid,"SolverId\t%i.%i", &i1, &i2);
    param->structure = (NPAR) i1;
    param->solver    = (NPAR) i2;
    fscanf(fid,"SolverId\t%hhu.%hhu", &param->structure, &param->solver);
    c = fscanf(fid,".%hhu\n", &param->solver_setting);
    if( c<1 ) {
        fscanf(fid,"\n");
        param->solver_setting = -1;
    }
    (*line_no)++;
	// nK
    fscanf(fid,"nK\t%i\n",&param->nK);
    (*line_no)++;
    // nG
    fscanf(fid,"nG\t%i\n",&param->nG);
    (*line_no)++;
    // nS
    fscanf(fid,"nS\t%hhu\n",&param->nS);
    (*line_no)++;
    // nO
    fscanf(fid,"nO\t%hhu\n",&param->nO);
    (*line_no)++;
}

//
// Handling blocking labels
//
void zeroLabels(struct param* param) { // set counts in data sequences to zero
	NCAT k,g;
	for(k=0; k<param->nK; k++)
		for(g=0; g<param->k_numg[k]; g++)
			param->k_g_data[k][g]->cnt = 0;
}

//
// clear up all forward/backward/etc variables for a skill-slice
//
void RecycleFitData(NCAT xndat, struct data** x_data, struct param *param) {
	NCAT x;
	NDAT t;
	for(x=0; x<xndat; x++) {
        //        if( x_data[x][0].cnt != 0)
        //            continue;
		if( x_data[x][0].alpha != NULL ) {
			free2D<NUMBER>(x_data[x][0].alpha, x_data[x][0].n);  // only free data here
			x_data[x][0].alpha = NULL;
		}
		if( x_data[x][0].c != NULL ) {
			free(x_data[x][0].c);  // only free data here
			x_data[x][0].c = NULL;
		}
		if( x_data[x][0].beta != NULL ) {
			free2D<NUMBER>(x_data[x][0].beta,  x_data[x][0].n); // only free data here
			x_data[x][0].beta = NULL;
		}
		if( x_data[x][0].gamma != NULL ) {
			free2D<NUMBER>(x_data[x][0].gamma, x_data[x][0].n); // only free data here
			x_data[x][0].gamma = NULL;
		}
		if( x_data[x][0].xi != NULL ) {
			for(t=0;t<x_data[x][0].n; t++)
				free2D<NUMBER>(x_data[x][0].xi[t],  (NDAT)param->nS); // only free data here
			x_data[x][0].xi = NULL;
		}
	}
}

// penalties
NUMBER L2penalty(param* param, NUMBER w) {
    NUMBER penalty_offset = 0.5;
    return (param->C > 0)? 0.5*param->C*fabs((w-penalty_offset)) : 0;
}

