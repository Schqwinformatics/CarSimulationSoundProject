// Copyright (C) 2013-2014 Leonid Lezner
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "mex.h"
#include <winsock2.h>

#pragma comment(lib, "ws2_32.lib")

int init_winsock(void)
{
    WSADATA wsa;
    return WSAStartup(MAKEWORD(2,0),&wsa);
}

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
    char ip_address[15];
    int port;
    char *payload;
    double *result;
    SOCKET sock;
    SOCKADDR_IN addr;
    long rc;
    
    if(nrhs != 3)
    {
        mexErrMsgIdAndTxt("Lezner:udp_send", "3 inputs required: Address, Port, Data.");
    }
    
    if(!mxIsChar(prhs[0]))
    {
        mexErrMsgIdAndTxt("Lezner:udp_send", "First argument must be string.");
    }
        
    if(!mxIsChar(prhs[2]))
    {
        mexErrMsgIdAndTxt("Lezner:udp_send", "Third argument must be string.");
    }
    
    if(!mxIsDouble(prhs[1]) || mxGetNumberOfElements(prhs[1]) != 1)
    {
        mexErrMsgIdAndTxt("Lezner:udp_send", "Second argument must be number.");
    }    
    
    // Allocating the buffer
    payload = (char*)malloc(mxGetNumberOfElements(prhs[2]) + 1);

    // Reading the arguments
    port = (int)mxGetScalar(prhs[1]);
    mxGetString(prhs[2], payload, mxGetNumberOfElements(prhs[2]) + 1);
    mxGetString(prhs[0], ip_address, 15);
    
    //printf("Sending UDP to %s:%d: %s\n", ip_address, port, payload);
    
    if(init_winsock() != 0)
    {
        mexErrMsgIdAndTxt("Lezner:udp_send", "Error starting WinSock.");
    }
    
    sock = socket(AF_INET, SOCK_DGRAM, 0);

    if(sock == INVALID_SOCKET)
    {
        mexErrMsgIdAndTxt("Lezner:udp_send", "Error creating socket.");
    }
    
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip_address);
    
    rc = sendto(sock, payload, strlen(payload), 0, (SOCKADDR*)&addr, sizeof(SOCKADDR_IN));
    
    if(rc == SOCKET_ERROR)
    {
        mexErrMsgIdAndTxt("Lezner:udp_send", "Error sending data.");
    }
    
    closesocket(sock);

    free(payload);
}