#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "interrupt.h"
#include "spr_defs.h"
#include "source/API/api.h"

#include "pingpong_config.h"

message pongping;

int main(int argc, char **argv)
{ 
    OVP_init();
    //////////////////////////////////////////////////////
    /////////////// YOUR CODE START HERE /////////////////
    //////////////////////////////////////////////////////
    
    int i;
    ReceiveMessage(&pongping, ping_addr);
    for(i=0;i<N_PINGPONG;i++){
        LOG("3-PONG: %d\n",pongping.msg[0]);
        pongping.msg[0] = pongping.msg[0] + 1;
        SendMessage(&pongping, ping_addr);
        ReceiveMessage(&pongping, ping_addr);
    }
    LOG("3-PRINT FINAL DO PACOTE COMPLETO:\n");
    for(i=0;i<pongping.size;i++){
        LOG("3-- %d\n",pongping.msg[i]);
    }


    //////////////////////////////////////////////////////
    //////////////// YOUR CODE ENDS HERE /////////////////
    //////////////////////////////////////////////////////
    finishApplication();
    return 1;
}
