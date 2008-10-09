/*********************************************************************
	Module: FLANN Matlab MEX interface
	Author: Marius Muja (2008)
***********************************************************************/

#include "mex.h"
#include "flann.h"
#include <stdio.h>
#include <string.h>

extern "C" {
int __data_start;  // hack to solve unresolved symbol problem
}


void _find_nearest_neighbors(int nOutArray, mxArray *OutArray[], int nInArray, const mxArray *InArray[])
{
	/* Check the number of input arguments */ 
	if(nInArray != 4) {
		mexErrMsgTxt("Incorrect number of input arguments, expecting:\n"
		"selector, dataset, testset, nearest_neighbors, params");
	}

	/* Check the number of output arguments */ 
	if (nOutArray != 1 && nOutArray != 2) {
		mexErrMsgTxt("Incorrect number of outputs.");
	}
		
	const mxArray* datasetMat = InArray[0];
	const mxArray* testsetMat = InArray[1];
	
	if (!(mxIsSingle(datasetMat) && mxIsSingle(testsetMat))) {
		mexErrMsgTxt("Need single precision datasets for now...");
	}	 

	int dcount = mxGetN(datasetMat);
	int length = mxGetM(datasetMat);
	int tcount = mxGetN(testsetMat);

	if (mxGetM(testsetMat) != length) {
		mexErrMsgTxt("Dataset and testset features should have the same size.");
	}
	
	const mxArray* nnMat = InArray[2];
	
	if (mxGetM(nnMat)!=1 || mxGetN(nnMat)!=1) {
		mexErrMsgTxt("Number of nearest neighbors should be a scalar.");
	}
	int nn = (int)(*mxGetPr(nnMat));		

	float* dataset = (float*) mxGetData(datasetMat);
	float* testset = (float*) mxGetData(testsetMat);
	int* result = (int*)malloc(tcount*nn*sizeof(int));
	
	const mxArray* pMat = InArray[3];

	int pSize = mxGetN(pMat)*mxGetM(pMat);
	
	double* pp = mxGetPr(pMat);

	IndexParameters p;
	if (*pp<0) {
		p.target_precision = pp[1];
		if (p.target_precision>1 || p.target_precision<0) {
			mexErrMsgTxt("Target precision must be between 0 and 1");
		}
		p.build_weight = pp[2];
		p.memory_weight = pp[3];
	}
	else {
		/* pp contains index & search parameters */
		p.checks = (int) pp[0];
		p.algorithm = (int)pp[1];
		p.trees=(int)pp[2];
		p.branching=(int)pp[3];
		p.iterations=(int)pp[4];
		p.centers_init = (int) pp[5];
		p.target_precision = -1;		
	}	
	/* do the search */
	flann_find_nearest_neighbors(dataset,dcount,length,testset, tcount, result, nn, &p, NULL);
	
	/* Allocate memory for Output Matrix */ 
	OutArray[0] = mxCreateDoubleMatrix(nn, tcount, mxREAL);	
	
	/* Get pointer to Output matrix and store result*/ 
	double* pOut = mxGetPr(OutArray[0]);
	for (int i=0;i<tcount*nn;++i) {
		pOut[i] = result[i]+1; // matlab uses 1-based indexing
	}
	free(result);
	
	if (nOutArray > 1) {
		OutArray[1] = mxCreateDoubleMatrix(1, 6, mxREAL);
		double* pParams = mxGetPr(OutArray[1]);
		
		pParams[0] = p.checks;
		pParams[1] = p.algorithm;
		pParams[2] = p.trees;
		pParams[3] = p.branching;
		pParams[4] = p.iterations;
		pParams[5] = p.centers_init;
	}
}

void _find_nearest_neighbors_index(int nOutArray, mxArray *OutArray[], int nInArray, const mxArray *InArray[])
{
	/* Check the number of input arguments */ 
	if(nInArray != 4) {
		mexErrMsgTxt("Incorrect number of input arguments");
	}

	/* Check if there is one Output matrix */ 
	if(nOutArray != 1) {
		mexErrMsgTxt("One output required.");
	}
		
	const mxArray* indexMat = InArray[0];
	FLANN_INDEX indexID = (FLANN_INDEX) *mxGetPr(indexMat);
	
	const mxArray* testsetMat = InArray[1];
	
	if (!mxIsSingle(testsetMat)) {
		mexErrMsgTxt("Need single precision datasets for now...");
	}	 

	int tcount = mxGetN(testsetMat);

	const mxArray* nnMat = InArray[2];
	
	if (mxGetM(nnMat)!=1 || mxGetN(nnMat)!=1) {
		mexErrMsgTxt("Number of nearest neighbors should be a scalar.");
	}
	int nn = (int)(*mxGetPr(nnMat));		

	float* testset = (float*) mxGetData(testsetMat);
	int* result = (int*)malloc(tcount*nn*sizeof(int));
	
	const mxArray* pMat = InArray[3];

	int ppSize = mxGetN(pMat)*mxGetM(pMat);
	double* pp = mxGetPr(pMat);

	flann_find_nearest_neighbors_index(indexID,testset, tcount, result, nn, (int)pp[0], NULL);
		
	/* Allocate memory for Output Matrix */ 
	OutArray[0] = mxCreateDoubleMatrix(nn, tcount, mxREAL);	
	
	/* Get pointer to Output matrix and store result*/ 
	double* pOut = mxGetPr(OutArray[0]);
	for (int i=0;i<tcount*nn;++i) {
		pOut[i] = result[i]+1; // matlab uses 1-based indexing
	}
	free(result);
}

static void _build_index(int nOutArray, mxArray *OutArray[], int nInArray, const mxArray *InArray[])
{
	/* Check the number of input arguments */ 
	if(nInArray != 2) {
		mexErrMsgTxt("Incorrect number of input arguments");
	}

	/* Check the number of output arguments */ 
	if (nOutArray == 0 || nOutArray > 3) {
		mexErrMsgTxt("Incorrect number of outputs.");
	}
		
	const mxArray* datasetMat = InArray[0];
	
	if (!mxIsSingle(datasetMat)) {
		mexErrMsgTxt("Need single precision datasets for now...");
	}	 

	int dcount = mxGetN(datasetMat);
	int length = mxGetM(datasetMat);	
	float* dataset = (float*) mxGetData(datasetMat);
	
	const mxArray* pMat = InArray[1];
	int pSize = mxGetN(pMat)*mxGetM(pMat);
	
	double* pp = mxGetPr(pMat);

	FLANN_INDEX indexID;
	IndexParameters p;
	if (*pp<0) {
		p.target_precision = pp[1];
		if (p.target_precision>1 || p.target_precision<0) {
			mexErrMsgTxt("Target precision must be between 0 and 1");
		}
		p.build_weight = pp[2];
		p.memory_weight = pp[3];
	}
	else {
		/* pp contains index & search parameters */
		p.checks = (int) pp[0];
		p.algorithm = (int)pp[1];
		p.trees=(int)pp[2];
		p.branching=(int)pp[3];
		p.iterations=(int)pp[4];
		p.centers_init = (int) pp[5];
		p.target_precision = -1;		
	}	
	
	float speedup = -1;
	indexID = flann_build_index(dataset,dcount,length, &speedup, &p, NULL);
		
	/* Allocate memory for Output Matrix */ 
	OutArray[0] = mxCreateDoubleMatrix(1, 1, mxREAL);	
	
	/* Get pointer to Output matrix and store result*/ 
	double* pOut = mxGetPr(OutArray[0]);
	pOut[0] = indexID;

	if (nOutArray > 1) {
		OutArray[1] = mxCreateDoubleMatrix(1, 6, mxREAL);
		double* pParams = mxGetPr(OutArray[1]);
		
		pParams[0] = p.checks;
		pParams[1] = p.algorithm;
		pParams[2] = p.trees;
		pParams[3] = p.branching;
		pParams[4] = p.iterations;
		pParams[5] = p.centers_init;
	}
	if (nOutArray > 2) {
		OutArray[2] = mxCreateDoubleMatrix(1, 1, mxREAL);
		double* pSpeedup = mxGetPr(OutArray[2]);
		
		*pSpeedup = speedup;
		
	}

}

static void _free_index(int nOutArray, mxArray *OutArray[], int nInArray, const mxArray *InArray[])
{
	/* Check the number of input arguments */ 
	if(! (nInArray == 1 && (mxGetN(InArray[0])*mxGetM(InArray[0]))==1)) {
		mexErrMsgTxt("Expecting a single scalar argument: the index ID");
	}
	double* indexPtr = mxGetPr(InArray[0]);
	flann_free_index((FLANN_INDEX)indexPtr[0], NULL);
}

void mexFunction(int nOutArray, mxArray *OutArray[], int nInArray, const mxArray *InArray[])
{
	static int started = 0;
	if (!started) {
   		flann_init();
   		started = 1;
   	}
	
	if(nInArray == 0 || !mxIsChar(InArray[0])) {
		mexErrMsgTxt("Expecting first argument to be one of:\n"
		"find_nn\n"
		"build_index\n"
		"index_find_nn\n"
		"free_index");
	}
	
	char selector[64];
	mxGetString(InArray[0],selector,64);
		
	if (strcmp(selector,"find_nn")==0) {
		_find_nearest_neighbors(nOutArray,OutArray, nInArray-1, InArray+1);
	}
	else if (strcmp(selector,"build_index")==0) {
		_build_index(nOutArray,OutArray, nInArray-1, InArray+1);
	}
	else if (strcmp(selector,"index_find_nn")==0) {
		_find_nearest_neighbors_index(nOutArray,OutArray, nInArray-1, InArray+1);
	}
	else if (strcmp(selector,"free_index")==0) {
		_free_index(nOutArray,OutArray, nInArray-1, InArray+1);
	}
	
}