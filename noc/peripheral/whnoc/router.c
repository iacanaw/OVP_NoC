
////////////////////////////////////////////////////////////////////////////////
//
//                W R I T T E N   B Y   I M P E R A S   I G E N
//
//                             Version 20170201.0
//
////////////////////////////////////////////////////////////////////////////////

#include "router.igen.h"
#include "noc.h"

// Local Address
unsigned int myAddress = 0xFFFFFFFF;

// Stores the requested output port for a given packet
unsigned int delivery[N_PORTS] = {ND,ND,ND,ND,ND};

// Countdown value, informing how many flits are left to be transmitted 
unsigned int flitCount[N_PORTS] = {HEADER,HEADER,HEADER,HEADER,HEADER};

// Stores the current packet size
unsigned int packetSize[N_PORTS] = {EMPTY,EMPTY,EMPTY,EMPTY,EMPTY};

// One buffer for each port
unsigned int buffers[N_PORTS][BUFFER_SIZE];

// Buffer read/write control pointers
unsigned int last[N_PORTS] = {0,0,0,0,0};
unsigned int first[N_PORTS] = {0,0,0,0,0};

// Stores the control status of each port
unsigned int control[N_PORTS] = {GO,GO,GO,GO,GO};

// Routing Table
unsigned int routingTable[N_PORTS] = {ND,ND,ND,ND,ND};

////////////////////////////////////////////////////////////
/////////////////////// FUNCTIONS //////////////////////////
////////////////////////////////////////////////////////////

// Extract the Y position for a given address
unsigned int positionY(unsigned int address){
    unsigned int mask =  0xF;
    unsigned int masked_n = address & mask;
    unsigned int thebit = masked_n;
    return thebit;
}

// Extract the X position for a given address
unsigned int positionX(unsigned int address){
    unsigned int mask =  0xF0;
    unsigned int masked_n = address & mask;
    unsigned int thebit = masked_n >> 4;
    return thebit;
}

// Calculates the output port for a given local address and a destination address
// using a XY routing algorithm
unsigned int XYrouting(unsigned int current, unsigned int dest){
    //bhmMessage("I", "XYRouting", "Local %d -- Dest %d\n",current, destination);
    //bhmMessage("I", "XYRouting", "LocalX %d -- LocalY %d  ---- DestX %d -- DestY %d\n",positionX(current), positionY(current), positionX(destination), positionY(destination));
    unsigned int destination = dest >> 24;
    if(positionX(current) == positionX(destination) && positionY(current) == positionY(destination)){
        return LOCAL;
    }
    else if(positionX(current) < positionX(destination)){
        return EAST;
    }
    else if(positionX(current) > positionX(destination)){
        return WEST;
    }
    else if(positionX(current) == positionX(destination) && positionY(current) < positionY(destination)){
        return NORTH;
    }
    else if(positionX(current) == positionX(destination) && positionY(current) > positionY(destination)){
        return SOUTH;
    }
    else{
        bhmMessage("I", "XYRouting", "Something is not quite right! ERROR!!!\n");
        return ND; // ERROR
    }
}

void bufferStatusUpdate(unsigned int port){
    unsigned int status = 0;
    // -- No available space in this buffer!
    if ((first[port] == 0 && last[port] == (BUFFER_SIZE-1)) || (first[port] == (last[port]+1))){
        status = STALL;
    }
    // -- There is space in this buffer!
    else{
        status = GO;
    }

    // Transmitt the new buffer status to the neighbor
    if (port == LOCAL){
        localPort_regs_data.controlLocal.value = status;
    }
    else if (port == EAST){
        ppmPacketnetWrite(handles.portControlEast, &status, sizeof(status));
    }
    else if (port == WEST){
        ppmPacketnetWrite(handles.portControlWest, &status, sizeof(status));
    }
    else if (port == NORTH){
        ppmPacketnetWrite(handles.portControlNorth, &status, sizeof(status));
    }
    else if (port == SOUTH){
        ppmPacketnetWrite(handles.portControlSouth, &status, sizeof(status));
    }

}

void bufferPush(unsigned int port, unsigned int flit){
    // Write a new flit in the buffer
    buffer[port][last[port]] = flit;
    if(last[port] < BUFFER_SIZE-1){
        last[port]++;
    }
    else if(last[port] == BUFFER_SIZE-1){
        last[port] = 0;
    }

    // Update the buffer status
    bufferStatusUpdate(port);
}

unsigned int bufferPop(unsigned int port){
    unsigned int value;
    // Read the first flit from the buffer
    value = buffer[port][first[port]];
    
    // Increments the "first" pointer
    if(first[port] < BUFFER_SIZE-1){
        first[port]++;
    }
    else if(first[port] == BUFFER_SIZE-1){
        first[port] = 0;
    }
    
    // Decreses the flitCount
    flitCount[port] = flitCount[port] - 1;

    // If the flitCount goes to EMPTY then the transmission is done!
    if (flitCount[port] == EMPTY){
        
        // Updates the routing table, releasing the output port
        routingTable[port] = ND;

        // Inform that the next flit will be a header
        flitCount[port] = HEADER;
    }
    // If it is the packet size flit then we store the value for the countdown
    else if (flitCount[port] == SIZE){
        flitCount[port] = value;
    }

    // Update the buffer status
    bufferStatusUpdate(port);
    
    return value;
}

unsigned int isEmpty(unsigned int port){
    if(last[port] != first[port]){
        return 0; // no, it is not empty
    }
    else{
        return 1; // yes, it is empty
    }
}

void sendFlits(){
    int port = 0;
    int checkport = 0;
    int to = ND;
    int allowed = 1;
    unsigned int data;
    // For each port do:
    for(port = EAST; port <= LOCAL; port++){

        // If this port is not transmitting anything
        if(routingTable[port] == ND){

            // And has something to transmit
            if (!isEmpty(port)){

                // Discover to wich port the packet must go
                to = XYrouting(myAddress, buffers[port][first]);

                // Verify if any other port is using the selected one
                for(checkport = 0; checkport <= LOCAL; checkport++){
                    if (routingTable[checkport] == to){
                        allowed = 0;
                        break;
                    }
                }
                if(allowed == 1){
                    routingTable[port] = to;
                }
            }
        }

        // If the port is transmitting something then send a flit ahead
        if(routingTable[port] < ND && !isEmpty(port)) {

            // Transmission to the local IP
            if(routingTable[port] == LOCAL && control[LOCAL] == DONE){
                localPort_regs_data.dataTxLocal.value = bufferPop(port);
                ppmWriteNet(handles.INTTC, 1);
            }

            // Transmit it to the EAST router
            else if(routingTable[port] == EAST){
                // While the receiver router has space AND 
                // there is flits to send AND 
                // still connected to the output port
                while(control[routingTable[port]] == GO && !isEmpty(port) && routingTable[port] == EAST){
                    data = bufferPop(port);
                    // Send a flit!
                    ppmPacketnetWrite(handles.portDataEast, &data, sizeof(data));
                }
            }

            // Transmit it to the WEST router
            else if(routingTable[port] == WEST){
                // While the receiver router has space AND 
                // there is flits to send AND 
                // still connected to the output port
                while(control[routingTable[port]] == GO && !isEmpty(port) && routingTable[port] == WEST){
                    data = bufferPop(port);
                    // Send a flit!
                    ppmPacketnetWrite(handles.portDataWest, &data, sizeof(data));
                }
            }

            // Transmit it to the NORTH router
            else if(routingTable[port] == NORTH){
                // While the receiver router has space AND 
                // there is flits to send AND 
                // still connected to the output port
                while(control[routingTable[port]] == GO && !isEmpty(port) && routingTable[port] == NORTH){
                    data = bufferPop(port);
                    // Send a flit!
                    ppmPacketnetWrite(handles.portDataNorth, &data, sizeof(data));
                }
            }

            // Transmit it to the SOUTH router
            else if(routingTable[port] == SOUTH){
                // While the receiver router has space AND 
                // there is flits to send AND 
                // still connected to the output port
                while(control[routingTable[port]] == GO && !isEmpty(port) && routingTable[port] == SOUTH){
                    data = bufferPop(port);
                    // Send a flit!
                    ppmPacketnetWrite(handles.portDataSouth, &data, sizeof(data));
                }
            }

        }
    }
}

void controlUpdate(unsigned int port, unsigned int ctrlData){
    // Stores the new neighbor status
    control[port] = ctrlData;
}

////////////////////////////////////////////////////////////
//////////////////// Callback stubs ////////////////////////
////////////////////////////////////////////////////////////

PPM_REG_READ_CB(addressRead) {
    return *(Uns32*)user;
}

PPM_REG_WRITE_CB(addressWrite) {
    // Stores the local address - it is sent by the local IP during the initialization
    myAddress = (unsigned int)data >> 24;
    int x = positionX(myAddress);
    int y = positionY(myAddress);
    bhmMessage("INFO", "MY_ADRESS", "My Address: %d %d", x, y);
    *(Uns32*)user = data;
}

PPM_PACKETNET_CB(controlEast) {
    // Updates the neighbor port status
    controlUpdate(EAST, (unsigned int *)data);
 
    // Runs the transmittion function
    sendFlits();
}

PPM_PACKETNET_CB(controlNorth) {
    // Updates the neighbor port status
    controlUpdate(NORTH, (unsigned int *)data);
 
    // Runs the transmittion function
    sendFlits();
}

PPM_PACKETNET_CB(controlSouth) {
    // Updates the neighbor port status
    controlUpdate(SOUTH, (unsigned int *)data);
 
    // Runs the transmittion function
    sendFlits();
}

PPM_PACKETNET_CB(controlWest) {
    // Updates the neighbor port status
    controlUpdate(WEST, (unsigned int *)data);
 
    // Runs the transmittion function
    sendFlits();
}

PPM_PACKETNET_CB(dataEast) {
    // Stores the new incoming flit
    bufferPush(EAST, (unsigned int *)data);

    // Runs the transmittion function
    sendFlits();
}

PPM_PACKETNET_CB(dataNorth) {
    // Stores the new incoming flit
    bufferPush(NORTH, (unsigned int *)data);

    // Runs the transmittion function
    sendFlits();
}

PPM_PACKETNET_CB(dataSouth) {
    // Stores the new incoming flit
    bufferPush(SOUTH, (unsigned int *)data);

    // Runs the transmittion function
    sendFlits();
}

PPM_PACKETNET_CB(dataWest) {
    // Stores the new incoming flit
    bufferPush(WEST, (unsigned int *)data);

    // Runs the transmittion function
    sendFlits();
}

PPM_REG_READ_CB(ctrlRead) {
    // Runs the transmittion function
    sendFlits();

    return *(Uns32*)user;
}

PPM_REG_WRITE_CB(ctrlWrite) {
    // Updates the local port status
    controlUpdate(LOCAL, (unsigned int *)data);

    // Runs the transmittion function
    sendFlits();

    *(Uns32*)user = data;
}

PPM_REG_READ_CB(rxRead) {
    return *(Uns32*)user;
}

PPM_REG_WRITE_CB(rxWrite) {
    // Stores the new incoming flit
    bufferPush(LOCAL, data);

    // Runs the transmittion function
    sendFlits();

    *(Uns32*)user = data;
}

PPM_REG_READ_CB(txRead) {
    // Once that the IP reads the flit, turn down the interruption 
    ppmWriteNet(handles.INTTC, 0); 

    // Runs the transmittion function
    sendFlits();
    return *(Uns32*)user;
}

PPM_REG_WRITE_CB(txWrite) {
    *(Uns32*)user = data;
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

