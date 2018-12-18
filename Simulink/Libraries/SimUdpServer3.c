/*  Simulink UDP Server  **********************************************************************/
/*  Martin Mladenovski & Giampiero Campa November 02 , revised August 2006 ********************/

#define S_FUNCTION_NAME SimUdpServer3
#define S_FUNCTION_LEVEL 2
#define nonblockingsocket(s) {unsigned long ctl = 1;ioctlsocket( s, FIONBIO, &ctl );}

#define MAXLEN 65536

#include "simstruc.h"
#include <math.h>
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")

typedef enum {csError, csReceive} EConnState;

typedef struct SInfoTag
{
	SOCKET servsock;
	EConnState connState;
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



/* mdlCheckParameters, check parameters, this routine is called later from mdlInitializeSizes */
#define MDL_CHECK_PARAMETERS
static void mdlCheckParameters(SimStruct *S)
{
    /* Basic check : All parameters must be real positive vectors                             */
    double *dPort, *dNumOfOutputs, *dPendingConnections, *dSampleTime; 
	int iPort, iNumOfOutputs, iPendingConnections;

    /* get the port number */
    dPort=mxGetPr(ssGetSFcnParam(S,0));
	iPort = (int)floor((*dPort)+0.5);
    if ( (iPort < 1025) || (iPort > 65535) )
    { ssSetErrorStatus(S,"Port # must be from 1025 to 65535"); return; }

    /* get the number of pending connections in queue */
    dPendingConnections=mxGetPr(ssGetSFcnParam(S,1));
	iPendingConnections = (int)floor((*dPendingConnections)+0.5);
    if ( (iPendingConnections < 5) || (iPendingConnections > 1024) )
    { ssSetErrorStatus(S,"Pending Connections # must be from 5 to 1024"); return; }

	/* size of output vector */
    dNumOfOutputs = mxGetPr(ssGetSFcnParam(S,2));
	iNumOfOutputs = (int)floor((*dNumOfOutputs)+0.5);
    if ( (iNumOfOutputs < 1) || (iNumOfOutputs > MAXLEN) )
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

    if (!ssSetNumInputPorts(S,0)) return;             /* number of input ports                */

    if (!ssSetNumOutputPorts(S,2)) return;            /* number of output ports               */
    ssSetOutputPortWidth(S,0, iBufSize);              /* first output port width              */
	ssSetOutputPortDataType(S,0,SS_UINT8);            /* first output port data type          */
    ssSetOutputPortWidth(S,1,1);                      /* second output port width             */
	ssSetOutputPortDataType(S,1,SS_UINT32);           /* second output port data type         */

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
SOCKET netServerInit(unsigned short port, int max_conn, SInfo *info)
{
    SOCKET serv_sock;
	struct sockaddr_in serv_sin;    /* my address information */

	serv_sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (serv_sock == INVALID_SOCKET) {
		printf("Cannot create socket.\n");
		goto err;
	}

	/* Set up information for bind */
	/* Clear the structure so that we don't have garbage around */
	memset((void *)&serv_sin, 0, sizeof(serv_sin));

	/* AF means Address Family - same as Protocol Family for now */
	serv_sin.sin_family = AF_INET;

	/* Fill in port number in address (careful of byte-ordering) */
	serv_sin.sin_port = htons(port);

	/* Fill in IP address for interface to bind to (INADDR_ANY) */
	serv_sin.sin_addr.s_addr = htonl(INADDR_ANY);

	/* Bind to port and interface */
	if (bind(serv_sock, (struct sockaddr *)&serv_sin,
			 sizeof(serv_sin)) == SOCKET_ERROR)
    {
        printf("Cannot bind.\n");
        goto err;
	}

	info->connState = csReceive;
    return serv_sock;

err:

	info->connState = csError;
    return SOCKET_ERROR;
}

/* start the server - called inside mdlStart **************************************************/

void StartUDPServer(SimStruct *S, SInfo *info)
{
    double *dPort, *dPendingConnections; 
	int iPort, iPendingConnections;

    /* get the port number */
    dPort=mxGetPr(ssGetSFcnParam(S,0));
	iPort = (int)floor((*dPort)+0.5);

    /* get the number of pending connections in queue */
    dPendingConnections=mxGetPr(ssGetSFcnParam(S,1));
	iPendingConnections = (int)floor((*dPendingConnections)+0.5);

	info->servsock = netServerInit((unsigned short)iPort, iPendingConnections, info);
	if(info->connState == csReceive);
		nonblockingsocket(info->servsock);
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
	info->connState = csError;
	StartUDPServer(S, info);
}

/* mdlOutputs - compute the outputs ***********************************************************/
static void mdlOutputs(SimStruct *S, int_T tid)
{

	/* output ports */
	void *y1=ssGetOutputPortSignal(S,0);
	uint8_T *data=(uint8_T*) y1;

	void *y2=ssGetOutputPortSignal(S,1);
	uint32_T *length=(uint32_T*) y2;

	int iBufSize = (int)floor((*mxGetPr(ssGetSFcnParam(S,2)))+0.5);

	unsigned int i;
	SInfo *info = GetStructure(S);

	/* socket variables */
    struct sockaddr_in req_sin;
    int req_len, recvlen, maxrecvlen=iBufSize;

	/* retrieve pointer to pointers work vector */
	void **PWork = ssGetPWork(S);

	/* assign buffer pointer */
	unsigned char *buffer;
    buffer = PWork[1];

	/* get req_len */
	req_len = sizeof(struct sockaddr_in);

	switch(info->connState)
	{
	case csReceive:
        recvlen=recvfrom(info->servsock, buffer, maxrecvlen, 0, (struct sockaddr *)&req_sin, &req_len);
		length[0] = (recvlen>=0) ? recvlen : 0;
        if(recvlen>0)
        {

			/* output data */
			for(i=0;i<recvlen;i++)
				data[i] = buffer[i];

        }
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
	closesocket(info->servsock);
	free(info);
    WSACleanup();
}

/* Trailer information to set everything up for simulink usage *******************************/
#ifdef  MATLAB_MEX_FILE                      /* Is this file being compiled as a MEX-file?   */
#include "simulink.c"                        /* MEX-file interface mechanism                 */
#else
#include "cg_sfun.h"                         /* Code generation registration function        */
#endif
