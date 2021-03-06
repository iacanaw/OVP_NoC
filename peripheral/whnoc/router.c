////////////////////////////////////////////////////////////////////////////////
//
//                W R I T T E N   B Y   I M P E R A S   I G E N
//
//                             Version 20170201.0
//
////////////////////////////////////////////////////////////////////////////////

#include "router.igen.h"
#include "noc.h"
#include <stdlib.h>
#include<stdio.h>


#define __bswap_constant_32(x) \
     ((((x) & 0xff000000) >> 24) | (((x) & 0x00ff0000) >>  8) |		      \
      (((x) & 0x0000ff00) <<  8) | (((x) & 0x000000ff) << 24))

unsigned int htonl(unsigned int x){
    return __bswap_constant_32(x);
}

////////////////////////////////////////////////////////////////////////////////

#define __cswap_constant_32(x) \
     ((((x) & 0xff000000) >> 24) | (((x) & 0x00ff0000) >>  8) |		      \
      (((x) & 0x0000ff00) <<  8) | (((x) & 0x000000ff) << 24))

unsigned int ntohl(unsigned int x){
    return __cswap_constant_32(x);
}

// Local Address
unsigned int myAddress = 0xFFFFFFFF;
// local ID Router
int myID = 0xFFFFFFFF;
// Number of packets delivered to the Local port
unsigned int localDeliveredPckts = 0;

// Countdown value per Packet, informing how many flits are left to be transmitted 
unsigned int flitCountOut[N_PORTS] = {HEADER,HEADER,HEADER,HEADER,HEADER};
unsigned int flitCountIn = HEADER;

// Define a struct that stores each flit and the time that it arrive in the router
typedef struct{
    unsigned int data;
    unsigned long long int inTime;
}flit;

// One buffer for each port
flit buffers[N_PORTS][BUFFER_SIZE];
flit incomingFlit;

// Buffer read/write control pointers
unsigned int last[N_PORTS] = {0,0,0,0,0};
unsigned int first[N_PORTS] = {0,0,0,0,0};
unsigned int localStatus = GO;

// Stores the control status of each port
unsigned int control[N_PORTS+1] = {GO,GO,GO,GO,GO,GO};
unsigned int txCtrl;

// Routing Table
unsigned int routingTable[N_PORTS] = {ND,ND,ND,ND,ND};

#if ARBITER_RR
// Priority list 
unsigned int priority[N_PORTS] = {0,1,2,3,4};
#endif

#if ARBITER_HERMES
// The Arbiter implemented in Hermes NoC
unsigned int nextPort = ND;
unsigned int actualPort = LOCAL;
#endif

#if ARBITER_TTL
// Time to Live Arbitration
unsigned int contPriority[N_PORTS] = {0,0,0,0,1};
#endif

// Stores the atual router tick status
unsigned short int myTickStatus = ITERATION_OFF;

// Stores the prebuffer autorization tick
unsigned short int myIterationStatusLocal = ITERATION_OFF_LOCAL;

// Gets the actual "time" of the system
unsigned long long int currentTime; // %llu

// Stores the "time" when a packet is transmitted to the network
unsigned long long int enteringTime;

// Maybe LEGACY!
unsigned int lastTickLocal = 0;

// PreBuffer variables
unsigned int preBufferPackets[PREBUFFER_SIZE];
unsigned int preBuffer_packetDest;
static unsigned int preBuffer_last = 0;
static unsigned int preBuffer_first = 0;



#if LOG_OUTPUTFLITS
// flit counter of each port, is reseted in each quantum in router.igen.c
int contFlits[N_PORTS];
#endif

// counter flits of each packet
int contFlitsPacket[N_PORTS] = {0,0,0,0,0};

// size of currentPacket
int sizeCurrentPacket[N_PORTS] = {0,0,0,0,0};


////////////////////////////////////////////////////////////
/////////////////////// FUNCTIONS //////////////////////////
////////////////////////////////////////////////////////////

// Informs the ticker that this router needs ticks
void turn_TickOn(){
    unsigned short int iterAux = ITERATION_ON;
    myTickStatus = ITERATION_ON;
    ppmPacketnetWrite(handles.iterationsPort, &iterAux, sizeof(iterAux));
}

// Informs the ticker that this router does not needs ticks
void turn_TickOff(){
    unsigned short int iterAux = ITERATION_OFF;
    myTickStatus = ITERATION_OFF;
    ppmPacketnetWrite(handles.iterationsPort, &iterAux, sizeof(iterAux));
}

// Informs the ticker that the local port needs ticks
void informIteratorLocalOn(){
    unsigned short int iterAux = ITERATION_BLOCKED_LOCAL;
    myIterationStatusLocal = ITERATION_BLOCKED_LOCAL;
    ppmPacketnetWrite(handles.iterationsPort, &iterAux, sizeof(iterAux));
}

// Informs the ticker that the local port does not needs ticks
void informIteratorLocalOff(){
    unsigned short int iterAux = ITERATION_OFF_LOCAL;
    myIterationStatusLocal = ITERATION_OFF_LOCAL;
    ppmPacketnetWrite(handles.iterationsPort, &iterAux, sizeof(iterAux));
}

// Inform the iterator that the PE is waiting a packet 
void informIterator(){
    unsigned short int iterAux = ITERATION;
    ppmPacketnetWrite(handles.iterationsPort, &iterAux, sizeof(iterAux));
}

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
    unsigned int destination = ntohl(dest);
    //bhmMessage("INFO", "XY", "dest: %d --- %d", dest, destination);
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

// Updates the buffer status
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
        // Local variable to communicate the status to the preBuffer
        localStatus = status;
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

void bufferPush(unsigned int port){
    // Write a new flit in the buffer
    buffers[port][last[port]] = incomingFlit;
    if(last[port] < BUFFER_SIZE-1){
        last[port]++;
    }
    else if(last[port] == BUFFER_SIZE-1){
        last[port] = 0;
    }

    // Inform the ticker that this router has something to send
    if(myTickStatus == ITERATION_OFF) turn_TickOn(); 

    // Update the buffer status
    bufferStatusUpdate(port);
}

unsigned int isEmpty(unsigned int port){
    if(last[port] != first[port]){
        return 0; // no, it is not empty
    }
    else{
        return 1; // yes, it is empty
    }
}

//verify if the preBuffer is empty 
unsigned int preBuffer_isEmpty(){
	if(preBuffer_first != (preBuffer_last)){
		return 0; // no, it is not empty
	}
    else{
		return 1; // yes, it is empty
	}
}

unsigned int bufferPop(unsigned int port){
    unsigned long long int value;

    // Read the first flit from the buffer
    value = buffers[port][first[port]].data;

    // Increments the "first" pointer
    if(first[port] < BUFFER_SIZE-1){
        first[port]++;
    }
    else if(first[port] == BUFFER_SIZE-1){
        first[port] = 0;
    }
    
    // Decreases the flitCountOut
    flitCountOut[port] = flitCountOut[port] - 1;
    
    // If the flitCountOut goes to EMPTY then the transmission is done!
    if (flitCountOut[port] == EMPTY){
        
        //Log info about the end of transmittion of a packet
        if (routingTable[port] == LOCAL){
            localDeliveredPckts++;
            bhmMessage("I", "Router", "Packet delivered at %d-(%d,%d) - total delivered: %d\n",myID, positionX(myAddress), positionY(myAddress), localDeliveredPckts);
        }

        // Updates the routing table, releasing the output port
        routingTable[port] = ND;

        if(flitCountOut[port] == OUT_TIME){
            value = ntohl(currentTime); // << 24;
        }

        // Inform that the next flit will be a header
        flitCountOut[port] = HEADER;

        // If every buffer is empty this router does not need to be ticked
        if(myTickStatus == ITERATION_ON && isEmpty(EAST) && isEmpty(WEST) && isEmpty(NORTH) && isEmpty(SOUTH) && isEmpty(LOCAL) && preBuffer_isEmpty()){
            turn_TickOff();
        }
        
        #if ARBITER_RR
        // Reset it's priority
        priority[port] = 0;
        #endif

        /*#if ARBITER_TTL //////// ACHO QUE DA PRA TIRAR ISSO. JÁ PARECE ESTAR GARANTIDO PELA FUNCAO searchAndAllocate
        // Reset it's priority
        contPriority[port] = 0;
        #endif*/    
    }
    // If it is the packet size flit then we store the value for the countdown
    else if (flitCountOut[port] == SIZE){
        flitCountOut[port] = htonl(value);
    }
    
    // Update the buffer status
    bufferStatusUpdate(port);
    
    return value;
}

#if ARBITER_RR
// Select the port with the biggest priority
unsigned int selectPort(){
    unsigned int selected = ND; // Starts selecting none;
    unsigned int selPrio = 0; 
    int k;
    for(k = 0; k <= LOCAL; k++){
        // Increases the priority every time that this function runs
        priority[k] = priority[k] + 1;
        // If the port has a request then...
        if(!isEmpty(k) && routingTable[k]==ND){
            if(priority[k] > selPrio){
                selected = k;
                selPrio = priority[k];
            }
        }
    }
    return selected;
}

// Allocates the output port to the givel selPort if it is available
void allocate(unsigned int port){
    unsigned int header, to, checkport, allowed;
    // In the first place, verify if the port is not connected to any thing and has something to transmitt 
    if(port != ND && routingTable[port] == ND && !isEmpty(port)){
        // Discover to wich port the packet must go
        header = buffers[port][first[port]].data;
        to = XYrouting(myAddress, header);
        // Verify if any other port is using the selected one
        allowed = 1;
        for(checkport = 0; checkport <= LOCAL; checkport++){
            if (routingTable[checkport] == to){
                allowed = 0;
                // If the port can't get routed, then turn it's priority down
                if(priority[port]>5) priority[port] = priority[port] - 5;
            }
        }
        if(allowed == 1){
            routingTable[port] = to;
            // Once one port is attended, then reset it's priority.
            priority[port] = 1;
        }
    }
}
#endif //ARBITER_RR


#if ARBITER_TTL
// Checks if a given output port is available
int portIsAvailable(int port){ 
    int checkport;
    for(checkport=0; checkport<N_PORTS; checkport++){
        if (routingTable[checkport] == port){
            return 0; // The port is unvailable
        }
    }
    return 1; // The output port is available
}

// Select the longest waiting port which has a output port available 
unsigned int selLongestWaitingPortAvailable(){
    int max = 0;
    int port = 0;
    int selPort = ND;
    //Checks for each port if the port is waiting more than max and is not routed and the output port is available
    for(port=0;port<N_PORTS;port++){
        if((contPriority[port]>max) && (routingTable[port]==ND) && (portIsAvailable(XYrouting(myAddress,buffers[port][first[port]].data)))){      
            max = contPriority[port];
            selPort = port;
        }
    }
    return selPort;
}

// Route the longest waiting buffer to a given output port
void searchAndAllocate(){
    int port = 0;
    int selectedPort = 0;

    // Increases the priority for ports that has something to send 
    for(port=0;port<N_PORTS;port++){
        if((!isEmpty(port)) && (routingTable[port] == ND)){
            contPriority[port] ++; 
        }
    }
    /*Returns the port with the bigger priority */
    selectedPort = selLongestWaitingPortAvailable();

    // If some port was selected
    if(selectedPort!=ND){
        // Updates the routingTable and reset the port priority
        routingTable[selectedPort] = XYrouting(myAddress,buffers[selectedPort][first[selectedPort]].data);
        contPriority[selectedPort] = 0;
    }
}
#endif // ARBITER_TTL

#if ARBITER_HERMES
// Select the port with the biggest priority
void selectPort(){
    switch (actualPort){
        case LOCAL:
            if(!isEmpty(EAST) && routingTable[EAST]==ND) nextPort = EAST;
            else if(!isEmpty(WEST) && routingTable[WEST]==ND) nextPort = WEST;
            else if(!isEmpty(NORTH) && routingTable[NORTH]==ND) nextPort = NORTH;
            else if(!isEmpty(SOUTH) && routingTable[SOUTH]==ND) nextPort = SOUTH;
            else nextPort = LOCAL;
        break;

        case EAST:
            if(!isEmpty(WEST) && routingTable[WEST]==ND) nextPort = WEST;
            else if(!isEmpty(NORTH) && routingTable[NORTH]==ND) nextPort = NORTH;
            else if(!isEmpty(SOUTH) && routingTable[SOUTH]==ND) nextPort = SOUTH;
            else if(!isEmpty(LOCAL) && routingTable[LOCAL]==ND) nextPort = LOCAL;
            else nextPort = EAST;
        break;

        case WEST:
            if(!isEmpty(NORTH) && routingTable[NORTH]==ND) nextPort = NORTH;
            else if(!isEmpty(SOUTH) && routingTable[SOUTH]==ND) nextPort = SOUTH;
            else if(!isEmpty(LOCAL) && routingTable[LOCAL]==ND) nextPort = LOCAL;
            else if(!isEmpty(EAST) && routingTable[EAST]==ND) nextPort = EAST;
            else nextPort = WEST;
        break;

        case NORTH:
            if(!isEmpty(SOUTH) && routingTable[SOUTH]==ND) nextPort = SOUTH;
            else if(!isEmpty(LOCAL) && routingTable[LOCAL]==ND) nextPort = LOCAL;
            else if(!isEmpty(EAST) && routingTable[EAST]==ND) nextPort = EAST;
            else if(!isEmpty(WEST) && routingTable[WEST]==ND) nextPort = WEST;
            else nextPort = NORTH;
        break;

        case SOUTH:
            if(!isEmpty(LOCAL) && routingTable[LOCAL]==ND) nextPort = LOCAL;
            else if(!isEmpty(EAST) && routingTable[EAST]==ND) nextPort = EAST;
            else if(!isEmpty(WEST) && routingTable[WEST]==ND) nextPort = WEST;
            else if(!isEmpty(NORTH) && routingTable[NORTH]==ND) nextPort = NORTH;
            else nextPort = SOUTH;
        break;

        default:
            nextPort = LOCAL;
    }
    actualPort = nextPort;
}

// Allocates the output port to the givel selPort if it is available
void allocate(){
    unsigned int header, to, checkport, allowed;
    // In the first place, verify if the port is not connected to any thing and has something to transmitt 
    if(actualPort != ND && routingTable[actualPort] == ND && !isEmpty(actualPort)){
        // Discover to wich port the packet must go
        header = buffers[actualPort][first[actualPort]].data;
        to = XYrouting(myAddress, header);
        // Verify if any other port is using the selected one
        allowed = 1;
        for(checkport = 0; checkport <= LOCAL; checkport++){
            if (routingTable[checkport] == to){
                allowed = 0;
            }
        }
        if(allowed == 1){
            routingTable[actualPort] = to;
        }
    }
}
#endif // ARBITER_HERMES

void transmitt(){
    unsigned int port, flit;
    // For each port...
    for(port = 0; port <= LOCAL; port++){
   
        // Verify if the port is routed and if it has something to transmmit 
        if((routingTable[port] < ND) && (!isEmpty(port))){

            // Checks if at least one tick has passed since the flit arrived in this router
            if ((currentTime > buffers[port][first[port]].inTime)||((port == LOCAL) && (lastTickLocal != currentTime))) { // MAYBE lastTickLocal IS LEGACY!
                 
                // Transmission to the local IP
                if(routingTable[port] == LOCAL && txCtrl == ACK){
                    // Refresh lastTickLocal
                    lastTickLocal = currentTime;
                    flit = bufferPop(port);
                    
                    #if LOG_OUTPUTFLITS
                    contFlits[LOCAL]= contFlits[LOCAL]++;
                    #endif
                    
                    // Set the TX to REQUEST
                    txCtrl = REQ;
                    localPort_regs_data.controlTxLocal.value = htonl(REQ);
                    // Set the flit value in the memory mapped reg
                    localPort_regs_data.dataTxLocal.value = flit;
                    // Interrupt the processor (this will occur one time per packet)
                    ppmWriteNet(handles.INTTC, 1);
                }

                // Transmit it to the EAST router
                else if(routingTable[port] == EAST){
                    /*  If the receiver router has space AND there is flits to send AND still connected to the output port*/
                    if(control[routingTable[port]] == GO && !isEmpty(port) && routingTable[port] == EAST){
                        // Gets a flit from the buffer 
                        flit = bufferPop(port);

                        #if LOG_OUTPUTFLITS
                        contFlits[EAST]= contFlits[EAST]++;
                        #endif
                        // Send the flit transmission time followed by the data
                        ppmPacketnetWrite(handles.portControlEast, &currentTime, sizeof(currentTime));
                        ppmPacketnetWrite(handles.portDataEast, &flit, sizeof(flit));
                    }
                }

                // Transmit it to the WEST router
                else if(routingTable[port] == WEST){
                    /*  If the receiver router has space AND there is flits to send AND still connected to the output port*/
                    if(control[routingTable[port]] == GO && !isEmpty(port) && routingTable[port] == WEST){
                        // Gets a flit from the buffer 
                        flit = bufferPop(port);
                        
                        #if LOG_OUTPUTFLITS
                        contFlits[WEST]= contFlits[WEST]++;
                        #endif
                        // Send the flit transmission time followed by the data
                        ppmPacketnetWrite(handles.portControlWest, &currentTime, sizeof(currentTime));
                        ppmPacketnetWrite(handles.portDataWest, &flit, sizeof(flit));
                    }
                }

                // Transmit it to the NORTH router
                else if(routingTable[port] == NORTH){
                    /*  If the receiver router has space AND there is flits to send AND still connected to the output port*/
                    if(control[routingTable[port]] == GO && !isEmpty(port) && routingTable[port] == NORTH){
                        // Gets a flit from the buffer 
                        flit = bufferPop(port);

                        #if LOG_OUTPUTFLITS
                        contFlits[NORTH]= contFlits[NORTH]++;
                        #endif
                        // Send the flit transmission time followed by the data
                        ppmPacketnetWrite(handles.portControlNorth, &currentTime, sizeof(currentTime));
                        ppmPacketnetWrite(handles.portDataNorth, &flit, sizeof(flit));
                    }
                }

                // Transmit it to the SOUTH router
                else if(routingTable[port] == SOUTH){
                    /*  If the receiver router has space AND there is flits to send AND still connected to the output port*/
                    if(control[routingTable[port]] == GO && !isEmpty(port) && routingTable[port] == SOUTH){
                        // Gets a flit from the buffer 
                        flit = bufferPop(port);

                        #if LOG_OUTPUTFLITS
                        contFlits[SOUTH]= contFlits[SOUTH]++;
                        #endif
                        // Send the flit transmission time followed by the data
                        ppmPacketnetWrite(handles.portControlSouth, &currentTime, sizeof(currentTime));
                        ppmPacketnetWrite(handles.portDataSouth, &flit, sizeof(flit));
                    }
                }
            }
        }
    }
}

// Stores the control signal for a given port
void controlUpdate(unsigned int port, unsigned int ctrlData){
    control[port] = ctrlData;
}

// Pre-Buffer status update - warns the IP is there is space or not inside the buffer
void preBuffer_statusUpdate(){
    unsigned int status;
    if((preBuffer_first == 0 && preBuffer_last == PREBUFFER_SIZE-1) || (preBuffer_first == (preBuffer_last+1))){
        status = STALL;
    }
    else{
        status = GO;
    }
    localPort_regs_data.controlRxLocal.value = status;
}

// Stores a flit that is incomming from the local IP
void preBuffer_push(unsigned int newFlit){
    contFlitsPacket[LOCAL]++;
    preBufferPackets[preBuffer_last] = newFlit;

    // preBuffer_last++
    if(preBuffer_last < PREBUFFER_SIZE-1){
        preBuffer_last++;
    }
    else if(preBuffer_last == PREBUFFER_SIZE-1){
        preBuffer_last = 0;
    }

    // Register the size of the incomming packet
    if(contFlitsPacket[LOCAL] == 2){ // register the packet size
        sizeCurrentPacket[LOCAL] = htonl(newFlit);
    }else if(contFlitsPacket[LOCAL] == sizeCurrentPacket[LOCAL]+2){ // if we are receiving the last flit
        contFlitsPacket[LOCAL] = 0;
    }else if(contFlitsPacket[LOCAL]==3){ // inform the iterator that this router has a packet in the prebuffer
        if(myIterationStatusLocal == ITERATION_OFF_LOCAL){
            informIteratorLocalOn();
            ppmPacketnetWrite(handles.iterationsPort, &newFlit, sizeof(newFlit));              
        }                  
    } 

    //Update the prebuffer status
    preBuffer_statusUpdate();
}

void preBuffer_pop(){
    unsigned int difX, difY;
    
    if(!preBuffer_isEmpty() && localStatus == GO){  
        ////////////////////////
        // Control insertions //
        ////////////////////////
        if(flitCountIn == HEADER){
            enteringTime = currentTime;
            preBuffer_packetDest = htonl(preBufferPackets[preBuffer_first]); //>> 24;
        }
        // Decrease the flitCount
        flitCountIn = flitCountIn - 1;

        // Register the size of a new packet to insert some control information in the tail
        if (flitCountIn == SIZE){
            flitCountIn = htonl(preBufferPackets[preBuffer_first]);
        }
        else if(flitCountIn == HOPES){
            // Calculate the number of hopes to achiev the destination address
            // Calculate the X dif
            if(positionX(myAddress)>positionX(preBuffer_packetDest)) difX = positionX(myAddress) - positionX(preBuffer_packetDest);
            else difX = positionX(preBuffer_packetDest) - positionX(myAddress);
            // Calculate the Y dif
            if(positionY(myAddress)>positionY(preBuffer_packetDest)) difY = positionY(myAddress) - positionY(preBuffer_packetDest);
            else difY = positionY(preBuffer_packetDest) - positionY(myAddress);
            // Adds both difs to determine the amount of hops
            preBufferPackets[preBuffer_first] = ntohl(difX + difY);
        }
        else if(flitCountIn == IN_TIME){
            preBufferPackets[preBuffer_first] = ntohl(enteringTime);
        }
        else if(flitCountIn == OUT_TIME){
            flitCountIn = HEADER;
            myIterationStatusLocal = ITERATION_OFF_LOCAL;
            informIteratorLocalOff();
            // Informs that there is another packet inside the prebuffer to send
            if(preBuffer_last != (preBuffer_first+1)){
                informIteratorLocalOn();
                ppmPacketnetWrite(handles.iterationsPort, &preBufferPackets[preBuffer_first+4], sizeof(preBufferPackets[preBuffer_first+4]));// LOOKS DANGEROUS
            }
        }
        // Register the flit data
        incomingFlit.data = preBufferPackets[preBuffer_first];
        // Register the time that this flit is recieved by the local buffer
        incomingFlit.inTime = currentTime;
        // Stores the flit in the local buffer
        bufferPush(LOCAL);

        // preBuffer_first++
        if(preBuffer_first < PREBUFFER_SIZE-1){
            preBuffer_first++;
        }
        else if(preBuffer_first == PREBUFFER_SIZE-1){
            preBuffer_first = 0;
        }

        // Update the prebuffer status
        preBuffer_statusUpdate();
    }
}

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////// Interaction Function //////////////////////////
////////////////////////////////////////////////////////////////////////////////

void iterate(){
    // Send a flit from the PREBUFFER to the local buffer
    if(myIterationStatusLocal == ITERATION_RELEASED_LOCAL){
        preBuffer_pop();
    }
    ////////////////////////////////////////////
    // Arbitration Process - defined in noc.h //
    ////////////////////////////////////////////
    #if ARBITER_RR
    unsigned int selPort;
    // Defines which port will be attended by the allocator
    selPort = selectPort();
    // Allocates the output port to the givel selPort if it is available
    allocate(selPort);
    #endif
    ////////////////////////////////////////////
    #if ARBITER_TTL
    // Search and allocate the packet which is waiting more time
    searchAndAllocate();
    #endif
    ////////////////////////////////////////////
    #if ARBITER_HERMES
    selectPort();
    allocate();
    #endif
    ////////////////////////////////////////////
    ////////////////////////////////////////////

    ////////////////////////////////////////////
    // Runs the transmittion of one flit to each direction (if there is a connection stablished)
    transmitt(); 
}

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////// Callback stubs ////////////////////////////////


PPM_REG_READ_CB(addressRead) {
    return *(Uns32*)user;
}

PPM_REG_WRITE_CB(addressWrite) {
    // By define - the address start with 0xF...F
    if(myAddress == 0xFFFFFFFF){
        // Stores the first write in this register as local address
        myAddress = htonl((unsigned int)data);
        int x = positionX(myAddress);
        int y = positionY(myAddress);
        bhmMessage("INFO", "MY_ADRESS", "My Address: %d %d", x, y);
        // Calculates the router ID
        myID = (DIM_X*y)+x;
        bhmMessage("INFO","MYADRESS","MY ID = %d", myID);
    }
    else{ // Display an error message when another write operation is made in this register!!
        bhmMessage("INFO", "MY_ADRESS", "ERROR: The address can not be changed!");
    }
    *(Uns32*)user = data;
}

PPM_PACKETNET_CB(controlEast) {
    // When receving a control value... (4 bytes info)
    if(bytes == 4){
        unsigned int ctrl = *(unsigned int *)data;
        controlUpdate(EAST, ctrl);
    }
    // When receving a time for the incoming flit... (8 bytes info)
    else if(bytes == 8){
        incomingFlit.inTime = *(unsigned long long int *)data;
    }
}

PPM_PACKETNET_CB(controlNorth) {
    // When receving a control value... (4 bytes info)
    if(bytes == 4){
        unsigned int ctrl = *(unsigned int *)data;
        controlUpdate(NORTH, ctrl);
    }
    // When receving a time for the incoming flit... (8 bytes info)
    else if(bytes == 8){
        incomingFlit.inTime = *(unsigned long long int *)data;
    }
}

PPM_PACKETNET_CB(controlSouth) {
    // When receving a control value... (4 bytes info)
    if(bytes == 4){
        unsigned int ctrl = *(unsigned int *)data;
        controlUpdate(SOUTH, ctrl);
    }
    // When receving a time for the incoming flit... (8 bytes info)
    else if(bytes == 8){
        incomingFlit.inTime = *(unsigned long long int *)data;
    }
}

PPM_PACKETNET_CB(controlWest) {
    // When receving a control value... (4 bytes info)
    if(bytes == 4){
        unsigned int ctrl = *(unsigned int *)data;
        controlUpdate(WEST, ctrl);
    }
    // When receving a time for the incoming flit... (8 bytes info)
    else if(bytes == 8){
        incomingFlit.inTime = *(unsigned long long int *)data;
    }
}

PPM_PACKETNET_CB(dataEast) {
    unsigned int newFlit = *(unsigned int *)data;
    incomingFlit.data = newFlit;
    bufferPush(EAST);
}

PPM_PACKETNET_CB(dataNorth) {
    unsigned int newFlit = *(unsigned int *)data;
    incomingFlit.data = newFlit;
    bufferPush(NORTH);
}

PPM_PACKETNET_CB(dataSouth) {
    unsigned int newFlit = *(unsigned int *)data;
    incomingFlit.data = newFlit;
    bufferPush(SOUTH);
}

PPM_PACKETNET_CB(dataWest) {
    unsigned int newFlit = *(unsigned int *)data;
    incomingFlit.data = newFlit;
    bufferPush(WEST);
}

// READ OPERATION IN THE REGISTER: controlRxLocal
PPM_REG_READ_CB(rxCtrlRead) {
    return *(Uns32*)user;   
}

// WRITE OPERATION IN THE REGISTER: controlRxLocal
PPM_REG_WRITE_CB(rxCtrlWrite) {
    *(Uns32*)user = data;
}

// READ OPERATION IN THE REGISTER: dataRxLocal
PPM_REG_READ_CB(rxRead) {
    return *(Uns32*)user;
}

// WRITE OPERATION IN THE REGISTER: dataRxLocal
PPM_REG_WRITE_CB(rxWrite) {
    // Stores the incomming data in the pre_buffer
    preBuffer_push(data);
    *(Uns32*)user = data;
}

// READ OPERATION IN THE REGISTER: controlTxLocal
PPM_REG_READ_CB(txCtrlRead) {
    // Inform the iterator that some IP waiting a packet...
    informIterator();
    return *(Uns32*)user;
}

// WRITE OPERATION IN THE REGISTER: controlTxLocal
PPM_REG_WRITE_CB(txCtrlWrite) {
    txCtrl = htonl(data);
    // If the last flit was read then the router should remove the interruption signal
    if(txCtrl==STALL){ 
        ppmWriteNet(handles.INTTC,0);
    }
    *(Uns32*)user = data;
}

// READ OPERATION IN THE REGISTER: dataTxLocal
PPM_REG_READ_CB(txRead) {
    return *(Uns32*)user;
}

// WRITE OPERATION IN THE REGISTER: dataTxLocal
PPM_REG_WRITE_CB(txWrite) {
    *(Uns32*)user = data;
}

PPM_PACKETNET_CB(iterationPort) {

    // Stores the actual iteration time
    currentTime = *(unsigned long long int *)data;

    //Checks if it is a local iteration
    if((currentTime >> 31) == 1){
        myIterationStatusLocal = ITERATION_RELEASED_LOCAL;
        currentTime = (unsigned long long int )(0x7FFFFFFFULL & currentTime);
    }

    //Runs iterate
    iterate();
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

   
