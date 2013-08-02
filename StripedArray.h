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

// StripedArray is an array that grows in chunks, just to save time and not do
// one extra linear scon of the data

#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <math.h>

#ifndef STRIPEDARRAY_H
#define STRIPEDARRAY_H

template <typename T>
class StripedArray {
public:
	StripedArray();
	StripedArray(unsigned int _size, bool a_complex); // predefined size
	StripedArray(FILE *f, unsigned long N);
	StripedArray(bool a_complex);
	~StripedArray();
	unsigned long getSize();
	void add(T value);
	T& operator [] (unsigned long idx);
	T get(unsigned long idx);
	void set(unsigned long idx, T value);
	void clear();
    unsigned long toBinFile(FILE* f);
private:
	unsigned long size; // linear
	unsigned long stripe_size;
	unsigned long nstripes;
	unsigned long size_last_stripe;
    bool complex; // element stored is a pointer to an array (should be deleted further)
	T** stripes;
	void addStripe();
};


template <typename T>
StripedArray<T>::StripedArray() {
	this->size = 0;
	this->stripe_size = 20000;
	this->nstripes = 0;
	this->size_last_stripe = 0;
	this->stripes = NULL;
    this->complex = false;
}

// predefined size
template <typename T>
StripedArray<T>::StripedArray(unsigned int _size, bool a_complex) {
	stripe_size = 20000;
    complex = a_complex;
	size = _size;
	nstripes = (unsigned long)ceil((double)size/stripe_size);
	size_last_stripe = (unsigned long)fmod(size, stripe_size);
	stripes = (T **) calloc(nstripes, sizeof(T*));
    unsigned long num;
    for(unsigned long i=0; i<nstripes; i++) {
        num = (i<(nstripes-1)?stripe_size:size_last_stripe);
        stripes[i] = (T*)calloc(num, sizeof(T)); // alloc data
    }
}

// from file
template <typename T>
StripedArray<T>::StripedArray(FILE *f, unsigned long N) {
	stripe_size = 20000;
    complex = false;
	size = N;
	nstripes = (unsigned long)ceil((double)N/stripe_size);
	size_last_stripe = (unsigned long)fmod(N, stripe_size);
	stripes = (T **) calloc(nstripes, sizeof(T*));
    unsigned long num;
    unsigned long nread;
    for(unsigned long i=0; i<nstripes; i++) {
        num = (i<(nstripes-1)?stripe_size:size_last_stripe);
        stripes[i] = (T*)calloc(num, sizeof(T)); // alloc data
        nread = fread (stripes[i], sizeof(T), num, f);
        if(nread != num) {
            fprintf(stderr,"Error reading data from file\n");
            return;
        }
    }
}

template <typename T>
StripedArray<T>::StripedArray(bool a_complex) {
	size = 0;
	stripe_size = 20000;
	nstripes = 0;
	size_last_stripe = 0;
	stripes = NULL;
    complex = a_complex;
}

template <typename T>
StripedArray<T>::~StripedArray() {
	for(unsigned long i=0; i<this->nstripes;i++) {
        if(this->complex)
            for(unsigned long j=0; j<( (i<(this->nstripes-1))?this->stripe_size:this->size_last_stripe );j++)
                free(  (void *)(this->stripes[i][j]) );
		free(this->stripes[i]);
    }
	free(this->stripes);
}

template <typename T>
void StripedArray<T>::add(T value) {
	if(this->size == (ULONG_MAX-1)) {
		fprintf(stderr, "Error! Maximum array size reached.\n");
		return;
	}
	
	// add stripe if necessary
	if( this->nstripes==0 || this->size_last_stripe==this->stripe_size)
		this->addStripe();
	// place data
	this->stripes[this->nstripes-1][this->size_last_stripe++] = value;
	this->size++;
}

template <typename T>
void StripedArray<T>::clear() {
	for(unsigned long i=0; i<this->nstripes;i++)
		free(this->stripes[i]);
	free(this->stripes);
	this->size = 0;
	this->stripe_size = 20000;
	this->nstripes = 0;
	this->size_last_stripe = 0;
	this->stripes = NULL;
}

template <typename T>
T& StripedArray<T>::operator [](unsigned long idx) {
	if(idx>(this->size-1)) {
		fprintf(stderr, "   %d exceeds array size %d.\n",idx,this->size);
		return NULL;
	}
	unsigned long idx_stripe = idx / this->stripe_size;
	unsigned long idx_within = idx % this->stripe_size;
	return this->stripes[idx_stripe][idx_within];
}

template <typename T>
T StripedArray<T>::get(unsigned long idx) {
	if(idx>(this->size-1)) {
		fprintf(stderr, "Exception! Element index %lu exceeds array size %lu\n",idx,this->size);
		return NULL;
	}
	unsigned long idx_stripe = idx / this->stripe_size;
	unsigned long idx_within = idx % this->stripe_size;
	return this->stripes[idx_stripe][idx_within];
}

template <typename T>
void StripedArray<T>::set(unsigned long idx, T value) {
	if(idx>(this->size-1)) {
		fprintf(stderr, "Exception! Element index %lu exceeds array size %lu\n",idx,this->size);
		return;
	}
	unsigned long idx_stripe = idx / this->stripe_size;
	unsigned long idx_within = idx % this->stripe_size;
	this->stripes[idx_stripe][idx_within] = value;
}


template <typename T>
unsigned long StripedArray<T>::getSize() {
	return this->size;
}

template <typename T>
void StripedArray<T>::addStripe() {
	if(this->size_last_stripe<this->stripe_size && this->nstripes>0) {
		fprintf(stderr, "Exception! You can only add stripes when none exits or previous is full.\n");
		return;
	}
	
	this->stripes = (T **) realloc(this->stripes,(++this->nstripes)*sizeof(T*)); // add pointer
	this->stripes[this->nstripes-1] = (T*)calloc(this->stripe_size, sizeof(T)); // alloc data
	this->size_last_stripe = 0; // reset counter
}

template <typename T>
unsigned long StripedArray<T>::toBinFile(FILE *f) {
    unsigned long num;
    unsigned long nwrit;
    unsigned long all_nwrit = 0;
    for(unsigned long i=0; i<nstripes; i++) {
        num = (unsigned int)(i<(nstripes-1))?stripe_size:size_last_stripe;
        nwrit = fwrite (stripes[i] , sizeof(T), num, f);
        all_nwrit += nwrit;
        if(num != nwrit) {
            fprintf(stderr, "Error writing. Attempted to write %lu but %lu were written.\n",num, nwrit);
            return 0;
        }
    }
    return all_nwrit;
}

#endif