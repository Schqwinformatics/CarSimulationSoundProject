/* Data Type Encoder and Decoder ***** Giampy **** Dec 2004 ******************************** */

#define S_FUNCTION_NAME byteview
#define S_FUNCTION_LEVEL 2

#include "simstruc.h"
#include "stdio.h"     /* for file handling */

/* mdlCheckParameters, check parameters, this routine is called later from mdlInitializeSizes */
#define MDL_CHECK_PARAMETERS
static void mdlCheckParameters(SimStruct *S)
{

    real_T *pr;                            
    int_T  i, el, nEls;

    /* Basic check : All parameters must be real positive vectors                             */
    for (i = 0; i < 3; i++) {
        if (mxIsEmpty(    ssGetSFcnParam(S,i)) || mxIsSparse(   ssGetSFcnParam(S,i)) ||
            mxIsComplex(  ssGetSFcnParam(S,i)) || !mxIsNumeric( ssGetSFcnParam(S,i))  )
                  { ssSetErrorStatus(S,"Parameters must be real finite vectors"); return; } 
        pr   = mxGetPr(ssGetSFcnParam(S,i));
        nEls = mxGetNumberOfElements(ssGetSFcnParam(S,i));
        for (el = 0; el < nEls; el++) {
            if (!mxIsFinite(pr[el])) 
                  { ssSetErrorStatus(S,"Parameters must be real finite vectors"); return; }
        }
    }

    /* Check number of elements in the first parameter */
    if ( mxGetNumberOfElements(ssGetSFcnParam(S,0)) != 1 )
    { ssSetErrorStatus(S,"The number of inputs must be a scalar"); return; }

    /* get the number of input and check it */
    pr = mxGetPr(ssGetSFcnParam(S,0));
    if ( (pr[0] < 1) )
    { ssSetErrorStatus(S,"The number of inputs must be greater than zero"); return; }

    /* Check number of elements in the second parameter */
    if ( mxGetNumberOfElements(ssGetSFcnParam(S,1)) != 1 )
    { ssSetErrorStatus(S,"The input type must be a scalar"); return; }

    /* get the input type and check it */
    pr = mxGetPr(ssGetSFcnParam(S,1));
    if ( ((int)(pr[0]-0.7) < 0) || ((int)(pr[0]-0.7) > 8) )
    { ssSetErrorStatus(S,"The input type must be an integer between 0 and 8"); return; }

    /* Check number of elements in the third parameter */
    if ( mxGetNumberOfElements(ssGetSFcnParam(S,2)) != 1 )
    { ssSetErrorStatus(S,"The output type must be a scalar"); return; }

    /* get the output type and check it */
    pr = mxGetPr(ssGetSFcnParam(S,2));
    if ( ((int)(pr[0]-0.7) < 0) || ((int)(pr[0]-0.7) > 8) )
    { ssSetErrorStatus(S,"The output type must be an integer between 0 and 8"); return; }

}



/* mdlInitializeSizes - initialize the sizes array ********************************************/
static void mdlInitializeSizes(SimStruct *S) {

	int Ni,Ti,Si,So,To,No;
	
    ssSetNumSFcnParams(S,3);                          /* number of expected parameters        */

    /* Check the number of parameters and then calls mdlCheckParameters to see if they are ok */
    if (ssGetNumSFcnParams(S) == ssGetSFcnParamsCount(S))
    { 
		mdlCheckParameters(S); 
		if (ssGetErrorStatus(S) != NULL) return; 
	} 
	else return;

    ssSetNumContStates(S,0);                          /* number of continuous states          */
    ssSetNumDiscStates(S,0);                          /* number of discrete states            */

    Ni =(int) (*mxGetPr(ssGetSFcnParam(S,0))+0.3);    /* get the number of inputs             */
    Ti =(int) (*mxGetPr(ssGetSFcnParam(S,1))-0.7);    /* get the input type                   */
    To =(int) (*mxGetPr(ssGetSFcnParam(S,2))-0.7);    /* get the output type                  */

	/* input port */
    if (!ssSetNumInputPorts(S,1)) return;             /* number of input ports                */
    ssSetInputPortWidth(S,0,Ni);                      /* first input port width               */
    ssSetInputPortDirectFeedThrough(S,0,1);           /* first port direct feedthrough flag   */
	ssSetInputPortDataType(S,0,Ti);

	/* output port */
    if (!ssSetNumOutputPorts(S,1)) return;            /* number of output ports               */
	ssSetOutputPortDataType(S,0,To);

	/* calculate input and output datatype sizes  */
    Si=64*(Ti==0)+32*(Ti==1||Ti==6||Ti==7)+16*(Ti==4||Ti==5)+8*(Ti==2||Ti==3)+8*(Ti==8);
    So=64*(To==0)+32*(To==1||To==6||To==7)+16*(To==4||To==5)+8*(To==2||To==3)+8*(To==8);

	/* calculate number of outputs */
    No=Ni*Si/So;

    if (No==0)
    { ssSetErrorStatus(S,"Not enough inputs, can't assign proper output size"); return; }

    ssSetOutputPortWidth(S,0,No);                     /* first output port width              */

	/* sample time */
    ssSetNumSampleTimes(S,0);                         /* number of sample times               */

	/* work vectors */
	ssSetNumRWork(S,0);                               /* number real work vector elements     */
    ssSetNumIWork(S,0);                               /* number int work vector elements      */
    ssSetNumPWork(S,1);                               /* number ptr work vector elements      */
    ssSetNumModes(S,0);                               /* number mode work vector elements     */
    ssSetNumNonsampledZCs(S,0);                       /* number of nonsampled zero crossing   */
}

/* mdlInitializeSampleTimes - initialize the sample times array *******************************/
static void mdlInitializeSampleTimes(SimStruct *S) {
    ssSetSampleTime(S, 0, INHERITED_SAMPLE_TIME);
    ssSetOffsetTime(S, 0, 0);
}

/* mdlStart - initialize hardware *************************************************************/
#define MDL_START
static void mdlStart(SimStruct *S) {

	int Ni,Si;
    unsigned char *B;

	/* input port data type */
	DTypeId Ti=ssGetInputPortDataType(S,0);

    /* retrieve pointer to pointers work vector */
	void **PWork = ssGetPWork(S);

    Ni =(int) (*mxGetPr(ssGetSFcnParam(S,0))+0.3);    /* get the number of inputs             */

	/* calculate input and output datatype sizes  */
    Si=64*(Ti==0)+32*(Ti==1||Ti==6||Ti==7)+16*(Ti==4||Ti==5)+8*(Ti==2||Ti==3)+8*(Ti==8);

	/* allocate memory for the buffer, in bytes */ 
    B = malloc((Ni*Si/8)*sizeof(unsigned char));

	/* store pointers in PWork so they can be accessed later */
	PWork[0] = (void*) B;

	/* check if memory allocation was ok */
	if (PWork[0]==NULL) 
		{ ssSetErrorStatus(S,"Error in mdlStart : could not allocate memory"); return; }

}

/* mdlOutputs - compute the outputs ***********************************************************/
static void mdlOutputs(SimStruct *S, int_T tid) {

	int i,Ni,Si,So;

    /* input ports */
	InputPtrsType u = ssGetInputPortSignalPtrs(S,0);
	real_T **ud;
	float **us;
	int8_T **u8;
	uint8_T **uu8;
	int16_T **u16;
	uint16_T **uu16;
	int32_T **u32;
	uint32_T **uu32;
	boolean_T **ub;

	/* output ports */
	void *y=ssGetOutputPortSignal(S,0);
	real_T *yd=(real_T*) y;
	float *ys=(float*) y;
	int8_T *y8=(int8_T*) y;
	uint8_T *yu8=(uint8_T*) y;
	int16_T *y16=(int16_T*) y;
	uint16_T *yu16=(uint16_T*) y;
	int32_T *y32=(int32_T*) y;
	uint32_T *yu32=(uint32_T*) y;
	boolean_T *yb=(boolean_T*) y;

	/* input and output port data types */
	DTypeId Ti=ssGetInputPortDataType(S,0), To=ssGetOutputPortDataType(S,0);

    /* retrieve buffer pointer */
	void **B = ssGetPWork(S);

	/* pointer to the same buffer */
	real_T *Bd;
	float *Bs;
	int8_T *B8;
	uint8_T *Bu8;
	int16_T *B16;
	uint16_T *Bu16;
	int32_T *B32;
	uint32_T *Bu32;
	boolean_T *Bb;

    /* parameters */
    Ni =(int) (*mxGetPr(ssGetSFcnParam(S,0))+0.3);    /* get the number of inputs             */
		
    /* find the input datatype size in bits, cast the pointers, and copy from input to buffer */
	switch (Ti) {
	case SS_DOUBLE:
		Si=64;
		ud=(real_T**) u;
		Bd=(real_T*) B[0];
		for(i=0;i<Ni;i++)
			Bd[i]=*ud[i];
		break;
	case SS_SINGLE:
		Si=32;
		us=(float**) u;
		Bs=(float*) B[0];
		for(i=0;i<Ni;i++)
			Bs[i]=*us[i];
		break;
	case SS_INT8:
		Si=8;
		u8=(int8_T**) u;
		B8=(int8_T*) B[0];
		for(i=0;i<Ni;i++)
			B8[i]=*u8[i];
		break;
	case SS_UINT8:
		Si=8;
		uu8=(uint8_T**) u;
		Bu8=(uint8_T*) B[0];
		for(i=0;i<Ni;i++)
			Bu8[i]=*uu8[i];
		break;
	case SS_INT16:
		Si=16;
		u16=(int16_T**) u;
		B16=(int16_T*) B[0];
		for(i=0;i<Ni;i++)
			B16[i]=*u16[i];
		break;
	case SS_UINT16:
		Si=16;
		uu16=(uint16_T**) u;
		Bu16=(uint16_T*) B[0];
		for(i=0;i<Ni;i++)
			Bu16[i]=*uu16[i];
		break;
	case SS_INT32:
		Si=32;
		u32=(int32_T**) u;
		B32=(int32_T*) B[0];
		for(i=0;i<Ni;i++)
			B32[i]=*u32[i];
		break;
	case SS_UINT32:
		Si=32;
		uu32=(uint32_T**) u;
		Bu32=(uint32_T*) B[0];
		for(i=0;i<Ni;i++)
			Bu32[i]=*uu32[i];
		break;
	case SS_BOOLEAN:
		Si=8;
		ub=(boolean_T**) u;
		Bb=(boolean_T*) B[0];
		for(i=0;i<Ni;i++)
			Bb[i]=*ub[i];
		break;
	default:
		ssSetErrorStatus(S,"Error in mdlOutput : input port type unrecognized"); 
		return;
	}


    /* find the output datatype size in bits, cast the pointer, and copy from buffer to output */
	switch (To) {
	case SS_DOUBLE:
		So=64;
		Bd=(double*) B[0];
		for(i=0;i<(Ni*Si/So);i++)
			yd[i]=Bd[i];
		break;
	case SS_SINGLE:
		So=32;
		Bs=(float*) B[0];
		for(i=0;i<(Ni*Si/So);i++)
			ys[i]=Bs[i];
		break;
	case SS_INT8:
		So=8;
		B8=(int8_T*) B[0];
		for(i=0;i<(Ni*Si/So);i++)
			y8[i]=B8[i];
		break;
	case SS_UINT8:
		So=8;
		Bu8=(uint8_T*) B[0];
		for(i=0;i<(Ni*Si/So);i++)
			yu8[i]=Bu8[i];
		break;
	case SS_INT16:
		So=16;
		B16=(int16_T*) B[0];
		for(i=0;i<(Ni*Si/So);i++)
			y16[i]=B16[i];
		break;
	case SS_UINT16:
		So=16;
		Bu16=(uint16_T*) B[0];
		for(i=0;i<(Ni*Si/So);i++)
			yu16[i]=Bu16[i];
		break;
	case SS_INT32:
		So=32;
		B32=(int32_T*) B[0];
		for(i=0;i<(Ni*Si/So);i++)
			y32[i]=B32[i];
		break;
	case SS_UINT32:
		So=32;
		Bu32=(uint32_T*) B[0];
		for(i=0;i<(Ni*Si/So);i++)
			yu32[i]=Bu32[i];
		break;
	case SS_BOOLEAN:
		So=8;
		Bb=(boolean_T*) B[0];
		for(i=0;i<(Ni*Si/So);i++)
			yb[i]=Bb[i];
		break;
	default:
		ssSetErrorStatus(S,"Error in mdlOutput : output port type unrecognized"); 
		return;
	}
	
}

/* mdlTerminate - called when the simulation is terminated ***********************************/
static void mdlTerminate(SimStruct *S) {

    /* retrieve pointer to buffer and deallocate memory */
	void **B = ssGetPWork(S);
	free(B[0]);

}

/* Trailer information to set everything up for simulink usage *******************************/
#ifdef  MATLAB_MEX_FILE                      /* Is this file being compiled as a MEX-file?   */
#include "simulink.c"                        /* MEX-file interface mechanism                 */
#else
#include "cg_sfun.h"                         /* Code generation registration function        */
#endif
