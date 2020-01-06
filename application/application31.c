#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "interrupt.h"
#include "spr_defs.h"
#include "api.h"

#include "transpose5x5_config.h"

message theMessage;

int main(int argc, char **argv)
{
    OVP_init();
    //////////////////////////////////////////////////////
    /////////////// YOUR CODE START HERE /////////////////
    //////////////////////////////////////////////////////
    int i;
    theMessage.size = 128;
    for(i=0;i<theMessage.size;i++){
        theMessage.msg[i] = (i+1)*2;
    }
    for(i=0;i<N_MESSAGES;i++){
        SendMessage(&theMessage, transpose4_addr);
        ReceiveMessage(&theMessage, transpose4_addr);
    }
    
    //////////////////////////////////////////////////////
    //////////////// YOUR CODE ENDS HERE /////////////////
    //////////////////////////////////////////////////////
    finishApplication();
    return 1;
}
