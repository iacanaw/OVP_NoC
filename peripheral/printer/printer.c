
////////////////////////////////////////////////////////////////////////////////
//
//                W R I T T E N   B Y   I M P E R A S   I G E N
//
//                             Version 20191106.0
//
////////////////////////////////////////////////////////////////////////////////

#include "printer.igen.h"
#include <string.h>
#include <stdio.h>
#include <string.h>

#define BUFFER_SIZE 4096

char buffer[BUFFER_SIZE];
int pointer = 0;
unsigned int myX = 0xFFFFFFF;
unsigned int myY = 0xFFFFFFF;
char logFileName[256];
int err0;

#define __bswap_constant_32(x) \
     ((((x) & 0xff000000) >> 24) | (((x) & 0x00ff0000) >>  8) |		      \
      (((x) & 0x0000ff00) <<  8) | (((x) & 0x000000ff) << 24))

unsigned int htonl(unsigned int x){
    return __bswap_constant_32(x);
}

//////////////////////////////// Callback stubs ////////////////////////////////

PPM_REG_READ_CB(readValue) {
    // YOUR CODE HERE (readValue)
    return *(Uns32*)user;
}

PPM_REG_WRITE_CB(writeValue) {
    unsigned int newValue = htonl(data);
    //////////////////////////
    // INITIALIZATION
    //////////////////////////
    if(myX == 0xFFFFFFF){
        myX = newValue;
    }
    else if(myY == 0xFFFFFFF){
        myY = newValue;
        err0 = sprintf(logFileName, "simulation/log_%dx%d.txt",myX, myY);
    }
    //////////////////////////
    // PRINTER BEHAVIORAL
    //////////////////////////
    else{
        buffer[pointer] = newValue;
        pointer++;
        if(newValue == '\0'){
            FILE *filepointer;
            filepointer = fopen(logFileName,"a");
            fprintf(filepointer,buffer);
            bhmMessage("INFO", "Printing...", buffer);
            fclose(filepointer);
            pointer = 0;
        }
        else if(pointer == BUFFER_SIZE-1){ 
            FILE *filepointer;
            filepointer = fopen(logFileName,"a");
            buffer[BUFFER_SIZE-1] = '\0';
            fprintf(filepointer,buffer);
            bhmMessage("INFO", "Printing...", buffer);
            fclose(filepointer);
            pointer = 0;
        }
    }
}

PPM_CONSTRUCTOR_CB(constructor) {
    // YOUR CODE HERE (pre constructor)
    periphConstructor();
    // YOUR CODE HERE (post constructor)
}

PPM_DESTRUCTOR_CB(destructor) {
    // YOUR CODE HERE (destructor)
}


PPM_SAVE_STATE_FN(peripheralSaveState) {
    bhmMessage("E", "PPM_RSNI", "Model does not implement save/restore");
}

PPM_RESTORE_STATE_FN(peripheralRestoreState) {
    bhmMessage("E", "PPM_RSNI", "Model does not implement save/restore");
}


