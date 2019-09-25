#include <stdio.h>
#include <string.h>

#include "interrupt.h"
#include "spr_defs.h"
#include "../peripheral/whnoc/noc.h"

#define ROUTER_BASE ((unsigned int *) 0x80000000)
#define SYNC_BASE ((unsigned int *) 0x80000014)

typedef struct {
   unsigned int size;
   unsigned int hopes;
   unsigned int startTime;
   unsigned int endTime;
   unsigned int dest;
   int *message;
}packet;
packet myPacket;

typedef unsigned int  Uns32;
typedef unsigned char Uns8;
unsigned int contInstructions = 0;
#define LOG(_FMT, ...)  printf( "Info " _FMT,  ## __VA_ARGS__)

volatile static Uns32 interrupt = 0; 
volatile static Uns32 rxPacket[256]; 
volatile static Uns32 rxPointer = 0;
volatile static Uns32 txPointer = 0;
volatile static Uns32 txPacket[256];

volatile unsigned int *control = ROUTER_BASE + 0x4;  // controlTxLocal
void interruptHandler(void) {
    volatile unsigned int *rxLocal = ROUTER_BASE + 0x1;  // dataTxLocal 
 //   LOG("~~~~~~~~~~~~~~~~~>>>>>>>>>>>>>Interrupcao!\n");
    if (rxPointer == 0){
        rxPacket[rxPointer] = *rxLocal;
//	printf("rxLocal = %d\n", *rxLocal);
        rxPointer++;
        *control = ACK;
       // myPacket.dest = *rxLocal;
    }
    else if (rxPointer == 1){
        rxPacket[rxPointer] = *rxLocal;
	//printf("rxLocal = %d\n", *rxLocal);
        rxPointer++;
        *control = ACK;
    //    myPacket.size = *rxLocal;
     //   myPacket.message = (int *)malloc(myPacket.size * sizeof(int));

    }
    else if (rxPointer ==2){
    myPacket.startTime = *rxLocal;
	rxPacket[rxPointer] = *rxLocal;
	//printf("tick quando entrou na noc = %d\n",*rxLocal);
	rxPointer++;
	*control = ACK;
     
    
    }else{
    

	//printf("---------------> else\n");
        rxPacket[rxPointer] = *rxLocal;
      //  myPacket.message[rxPointer] = *rxLocal;
	//printf("msg = %d\n", *rxLocal);
        rxPointer++;
        if(rxPointer >= rxPacket[1] + 2){
	 //   printf("ENTROU NO IF");
            interrupt = 1;
	        *control = STALL;
        }
        else{
	//    printf("ENTROU NO SEGUNDO ELSE");
        	*control = ACK;
	    }
    }
}

void sendPckt(){
    volatile unsigned int *txLocal = ROUTER_BASE + 0x2; // dataRxLocal
    volatile unsigned int *controlTx = ROUTER_BASE + 0x3; // controlRxLocal
    txPointer = 0;
    while(txPointer < (txPacket[1] + 2)){
        while(*controlTx != GO){
            LOG("\n %d \n", *controlTx);
            // Waiting for space in the router buffer
        }
        *txLocal = txPacket[txPointer];
        txPointer++;
    }
}

void receivePckt(){
    while(*control!=STALL){
    }
    while(interrupt!=1){
}
}

void packetConsumed(){
    rxPointer = 0;
    interrupt = 0;
    *control = ACK;
}

int main(int argc, char **argv)
{
    volatile unsigned int *myAddress = ROUTER_BASE + 0x0;
    volatile unsigned int *PEToSync = SYNC_BASE + 0x1;	    
    volatile unsigned int *SyncToPE = SYNC_BASE + 0x0;

    LOG("Starting ROUTER22 application! \n\n");
    // Attach the external interrupt handler for 'intr0'
    int_init();
    int_add(0, (void *)interruptHandler, NULL);
    int_enable(0);

    // Enable external interrupts
    Uns32 spr = MFSPR(17);
    spr |= 0x4;
    MTSPR(17, spr);

    int start = 0;
    *myAddress = 0x24;

    *PEToSync = 0x24;
    while(start != 1){
	start = *SyncToPE >> 24;
     }

    //========================
    // YOUR CODE HERE
    //========================
    int i;
    for(i=0;i<2;i++){
        receivePckt();
        /* for(i=0;i<100;i++){
            printf("rxPacket = %d\n",myPacket.message[i]);
        }*/
       // LOG("MY_PACKET START TIME = %d", myPacket.startTime);
        LOG("-- %d\n", rxPacket[22]);
        LOG("Pacote levou %d ticks para trafegar na NoC", rxPacket[104]-myPacket.startTime);
        packetConsumed();
    }

    LOG("Application ROUTER22 done!\n\n");
    return 1;
}
