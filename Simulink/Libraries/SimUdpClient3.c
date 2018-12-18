/*  Simulink UDP Client  **********************************************************************/
/*  Martin Mladenovski & Giampiero Campa November 02 , revised August 2006 ********************/

#define S_FUNCTION_NAME SimUdpClient3
#define S_FUNCTION_LEVEL 2
#define nonblockingsocket(s) {unsigned long ctl = 1;ioctlsocket( s, FIONBIO, &ctl );}

#define MAXLEN 65536

#include <math.h>
#include <winsock2.h>
#include "simstruc.h"
#pragma comment(lib, "ws2_32.lib")

typedef enum {csError, csConnected} EConnState;

typedef struct SInfoTag
{
	SOCKET clisock;
	EConnState connState;
	struct sockaddr_in cli_sin;
	int iAddrLen;
} SInfo;


/* Create Structure Info **********************************************************************/
void CreateStructure(SimStruct *S)
{
	SInfo *info;
	void **PWork = ssGetPWork(S);

	info = (SInfo *)malloc(sizeof(SInfo));
	PWork[0] = (void *)info;
	
}

/* Get Structure Info *************************************************************************/
SInfo *GetStructure(SimStruct *S)
{
	void **PWork = ssGetPWork(S);
	return (SInfo *)PWork[0];
}

/* Get Structure Info *************************************************************************/
void DeleteStructure(SimStruct *S)
{
	SInfo *info = GetStructure(S);
	free(info);
}


/* mdlCheckParameters, check parameters, this routine is called later from mdlInitializeSizes */
#define MDL_CHECK_PARAMETERS
static void mdlCheckParameters(SimStruct *S)
{
    /* Basic check : All parameters must be real positive vectors                             */
    double *dPort, *dNumOfInputs; 
	char hostname[256];
	int iPort, iNumOfInputs;

	mxGetString(ssGetSFcnParam(S,0), hostname, 200);

    /* Check number of elements in second parameter: */
    if ( mxGetNumberOfElements(ssGetSFcnParam(S,1)) != 1 )
    { ssSetErrorStatus(S,"The parameter must be a 1 elements vector"); return; }

    /* Check port number */
    dPort=mxGetPr(ssGetSFcnParam(S,1));
	iPort = (int)floor((*dPort)+0.5);
    if ( (iPort < 1025) || (iPort > 65535) )
    { ssSetErrorStatus(S,"Port # must be from 1025 to 65535"); return; }

    /* Check number of inputs*/
    dNumOfInputs = mxGetPr(ssGetSFcnParam(S,2));
	iNumOfInputs = (int)floor((*dNumOfInputs)+0.5);
    if ( (iNumOfInputs < 1) || (iNumOfInputs > MAXLEN) )
    { 
		ssSetErrorStatus(S, "Buffer Size Limit MAXLEN exceeded"); 
		return; 
	}

}

/* mdlInitializeSizes - initialize the sizes array ********************************************/
static void mdlInitializeSizes(SimStruct *S)
{

	int iBufSize;
    
	ssSetNumSFcnParams(S,4);                          /* number of expected parameters        */

    /* Check the number of parameters and then calls mdlCheckParameters to see if they are ok */
    if (ssGetNumSFcnParams(S) == ssGetSFcnParamsCount(S))
    { mdlCheckParameters(S); if (ssGetErrorStatus(S) != NULL) return; } else return;

	iBufSize = (int)floor((*mxGetPr(ssGetSFcnParam(S,2)))+0.5);

    ssSetNumContStates(S,0);                          /* number of continuous states          */
    ssSetNumDiscStates(S,0);                          /* number of discrete states            */

    if (!ssSetNumInputPorts(S,2)) return;             /* number of input ports                */

    ssSetInputPortWidth(S,0,iBufSize);                /* first input port width               */
    ssSetInputPortDirectFeedThrough(S,0,1);           /* first port direct feedthrough flag   */
	ssSetInputPortDataType(S,0,SS_UINT8);			  /* first input port data type           */

    ssSetInputPortWidth(S,1,1);                       /* second input port width              */
    ssSetInputPortDirectFeedThrough(S,1,1);           /* second port direct feedthrough flag  */
	ssSetInputPortDataType(S,1,SS_UINT32);			  /* second input port data type          */

    if (!ssSetNumOutputPorts(S,0)) return;            /* number of output ports               */
   
    ssSetNumSampleTimes(S,0);                         /* number of sample times               */

    ssSetNumRWork(S,0);                               /* number real work vector elements     */
    ssSetNumIWork(S,0);                               /* number int_T work vector elements    */
    ssSetNumPWork(S,2);                               /* number ptr work vector elements      */
    ssSetNumModes(S,0);                               /* number mode work vector elements     */
    ssSetNumNonsampledZCs(S,0);                       /* number of nonsampled zero crossing   */
}

/* mdlInitializeSampleTimes - initialize the sample times array *******************************/
static void mdlInitializeSampleTimes(SimStruct *S)
{

	double *dSampleTime=mxGetPr(ssGetSFcnParam(S,3));
    
	/* Set things up to run with inherited sample time                                        */
    ssSetSampleTime(S, 0, dSampleTime[0]);
    ssSetOffsetTime(S, 0, 0);
}

/*************************************************************************/
SOCKET netClientInit(const char *hostname, unsigned short serv_port, SInfo *info)
{
    SOCKET cli_sock;
	struct hostent *hent;
	unsigned long srv_addr;

	info->iAddrLen = sizeof(info->cli_sin);
	/* Clear the structure so that we don't have garbage around */
	memset((void *)&(info->cli_sin), 0, info->iAddrLen);

    /* Try to find the IP address for the hostname */
 	if ((hent = gethostbyname(hostname)) != NULL)
    {
        memcpy((char *)&srv_addr, (char *)hent->h_addr_list[0], hent->h_length);
        /* Fill in IP address */
        info->cli_sin.sin_addr.s_addr = srv_addr;
 	} /* If hostname does not exist try as IP address */
    else if ((info->cli_sin.sin_addr.s_addr = inet_addr(hostname))==INADDR_NONE)
    {
        /* If cannot convert IP address than it is an error */
        printf("Bad hostname.\n");
        goto err;
    }

	/* AF means Address Family - same as Protocol Family for now */
	info->cli_sin.sin_family = AF_INET;

	/* Fill in port number in address (careful of byte-ordering) */
	info->cli_sin.sin_port = htons(serv_port);

	cli_sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (cli_sock == INVALID_SOCKET)
    {
        printf("Cannot create socket.\n");
        goto err;
	}

	info->connState = csConnected;
    return cli_sock;

err:

	info->connState = csError;
    return SOCKET_ERROR;
}

/* start the client - called inside mdlStart **************************************************/
void StartUDPClient(SimStruct *S, SInfo *info)
{
    double *dPort; 
	char hostname[256];
	int iPort;

	mxGetString(ssGetSFcnParam(S,0), hostname, 255);

    dPort=mxGetPr(ssGetSFcnParam(S,1));
	iPort = (int)floor((*dPort)+0.5);

	info->connState = csError;
	info->clisock = netClientInit(hostname, (unsigned short)iPort, info);

	if(info->connState==csConnected)
		nonblockingsocket(info->clisock);
}

/* mdlStart - initialize work vectors *********************************************************/
#define MDL_START
static void mdlStart(SimStruct *S)
{
	WSADATA wsa_data;
	int status;
	SInfo *info;

	/* get buffer size */ 
	int iBufSize = (int)floor((*mxGetPr(ssGetSFcnParam(S,2)))+0.5);

	/* retrieve pointer to pointers work vector */
	void **PWork = ssGetPWork(S);

	/* allocate buffer */
	unsigned char *buffer;
    buffer = malloc(iBufSize*sizeof(unsigned char));

	/* check if memory allocation was ok */
	if (buffer==NULL) 
		{ ssSetErrorStatus(S,"Error in mdlStart : could not allocate memory"); return; }

    /* store pointers in PWork so they can be accessed later */
	PWork[1] = (void*) buffer;

	/* Activate the Winsock DLL */
	if ((status = WSAStartup(MAKEWORD(2,2),&wsa_data)) != 0) {
		printf("%d is the WSA startup error\n",status);
		exit(1);
	}
	CreateStructure(S);
	info = GetStructure(S);
	StartUDPClient(S, info);
}

/* mdlOutputs - compute the outputs ***********************************************************/
static void mdlOutputs(SimStruct *S, int_T tid)
{

	/* input ports */
	uint8_T **u = (uint8_T**) ssGetInputPortSignalPtrs(S,0);
	uint32_T **dataLen = (uint8_T**) ssGetInputPortSignalPtrs(S,1);

	int iBufSize = (int)floor((*mxGetPr(ssGetSFcnParam(S,2)))+0.5);

	int i, retval;
	SInfo *info = GetStructure(S);
	int length = (int)floor(*dataLen[0]+0.5);
	int sock_error;
    
	/* retrieve pointer to pointers work vector */
	void **PWork = ssGetPWork(S);

	/* assign buffer pointer */
	unsigned char *buffer;
    buffer = PWork[1];

	length = (iBufSize<length) ? iBufSize : length;

	for(i=0;i<length;i++)
		buffer[i] = *u[i];

	switch(info->connState)
	{
	case csConnected:
		if(length>0)
		{
			retval = sendto(info->clisock, buffer, length, 0, &(info->cli_sin), info->iAddrLen);
			if(retval>=0)
			{
				/* printf("Sent %d bytes from %d bytes.\n", retval, length); */
			}
			else if (retval == SOCKET_ERROR)
			{
				sock_error = WSAGetLastError();
				if(sock_error == WSAESHUTDOWN || sock_error == WSAENOTCONN ||
				   sock_error == WSAEHOSTUNREACH || sock_error == WSAECONNABORTED ||
				   sock_error == WSAECONNRESET || sock_error == WSAETIMEDOUT)
				{
					closesocket(info->clisock);
					info->connState = csError;
				}
				printf("Error code: %d\n", WSAGetLastError());
			}
		}
		break;

	case csError:
		StartUDPClient(S, info);
		break;

	}
	
}

/* mdlTerminate - called when the simulation is terminated ***********************************/
static void mdlTerminate(SimStruct *S) 
{
	void **P = ssGetPWork(S);
	SInfo *info = GetStructure(S);

	/* deallocate buffer */
	free(P[1]);

	/* deallocate socket */
	closesocket(info->clisock);
	DeleteStructure(S);
    WSACleanup();
}

/* Trailer information to set everything up for simulink usage *******************************/
#ifdef  MATLAB_MEX_FILE                      /* Is this file being compiled as a MEX-file?   */
#include "simulink.c"                        /* MEX-file interface mechanism                 */
#else
#include "cg_sfun.h"                         /* Code generation registration function        */
#endif
