
////////////////////////////////////////////////////////////////////////////////
//
//                W R I T T E N   B Y   I M P E R A S   I G E N
//
//                             Version 20170201.0
//
////////////////////////////////////////////////////////////////////////////////

#include "router.igen.h"
//////////////////////////////// Callback stubs ////////////////////////////////

unsigned int dataCount[5] = 0;
unsigned int delivery[5] = 0;

unsigned int positionY(unsigned int address){
    unsigned int mask =  3;
    unsigned int masked_n = address & mask;
    unsigned int thebit = masked_n;
    return masked_n;
}

unsigned int positionX(unsigned int address){
    unsigned int mask =  3 << 2;
    unsigned int masked_n = address & mask;
    unsigned int thebit = masked_n >> 2;
    return masked_n;
}

unsigned int XYrouting(unsigned int current, unsigned int destination){
    if(positionX(current) == positionX(destination) && positionY(current) == positionY(destination)){
        return = 4; // local
    }
    else if(positionX(current) < positionX(destination)){
        return = 0; // east
    }
    else if(positionX(current) > positionX(destination)){
        return = 1; // west
    }
    else if(positionX(current) == positionX(destination) && positionX(current) < positionX(destination)){
        return = 2; // north
    }
    else if(positionX(current) == positionX(destination) && positionX(current) > positionX(destination)){
        return = 3; // south
    }
    else{
        return = 0xF; // ERROR
        bhmMessage("I", "XYRouting", "Something is not quite right! ERROR!!!\n");
    }
}

unsigned int deliveryPkt(unsigned int destPort, unsigned int my_data){
    if(destPort = 4){// LOCAL
        bhmMessage("I", "RX", "bytes=%d rx_reg2=%x UD=%d\n", bytes, my_data, (Uns32)userData);
        bport1_regs_data.rx_reg2.value = my_data;
    }
    else if(destPort = 0){ // EAST
        bhmMessage("I", "TX", "%x\n", my_data);
        ppmPacketnetWrite(handles.pktEast, &my_data, sizeof(my_data));
    }
    else if(destPort = 1){ // WEST
        bhmMessage("I", "TX", "%x\n", my_data);
        ppmPacketnetWrite(handles.pktWest, &my_data, sizeof(my_data));
    }
    else if(destPort = 2){ // NORTH
        bhmMessage("I", "TX", "%x\n", my_data);
        ppmPacketnetWrite(handles.pktNorth, &my_data, sizeof(my_data));
    }
    else if(destPort = 3){ // SOUTH
        bhmMessage("I", "TX", "%x\n", my_data);
        ppmPacketnetWrite(handles.pktSouth, &my_data, sizeof(my_data));
    }
    else{ // EAST
        bhmMessage("I", "TX", "An ilusion? What are you hiding? ERROR!%x\n", my_data);
    }
    
}

PPM_REG_READ_CB(rxRead1) {
    //ppmWriteNet(handles.INTTC, 0);
    return *(Uns32*)user;
}

PPM_REG_READ_CB(rxRead2) {
    ppmWriteNet(handles.INTTC, 0);
    return *(Uns32*)user;
}

PPM_REG_WRITE_CB(rxWrite1) {
    // YOUR CODE HERE (rxWrite1)
    *(Uns32*)user = data;
}

PPM_REG_WRITE_CB(rxWrite2) {
    // YOUR CODE HERE (rxWrite2)
    *(Uns32*)user = data;
}

PPM_PACKETNET_CB(triggerEast) {
    if (dataCount[0] == 0){
        unsigned int rx_data1 = *(unsigned int *)data;

        delivery[0] = XYrouting((Uns32)userData, rx_data1);
        
        deliveryPkt(delivery[0], rx_data1);

        dataCount[0]=1;
    }
    else{
        unsigned int rx_data2 = *(unsigned int *)data;
        dataCount[0]=0;

        deliveryPkt(delivery[0], rx_data2);

        ppmWriteNet(handles.INTTC, 1);
    }
}

PPM_PACKETNET_CB(triggerWest) {
    if (dataCount[1] == 1){
        unsigned int rx_data1 = *(unsigned int *)data;

        delivery[1] = XYrouting((Uns32)userData, rx_data1);
        
        deliveryPkt(delivery[1], rx_data1);

        dataCount[1]=1;
    }
    else{
        unsigned int rx_data2 = *(unsigned int *)data;
        dataCount[1]=0;

        deliveryPkt(delivery[1], rx_data2);

        ppmWriteNet(handles.INTTC, 1);
    }
}

PPM_PACKETNET_CB(triggerNorth) {
    if (dataCount[2] == 0){
        unsigned int rx_data1 = *(unsigned int *)data;

        delivery[2] = XYrouting((Uns32)userData, rx_data1);
        
        deliveryPkt(delivery[2], rx_data1);

        dataCount[2]=1;
    }
    else{
        unsigned int rx_data2 = *(unsigned int *)data;
        dataCount[2]=0;

        deliveryPkt(delivery[2], rx_data2);

        ppmWriteNet(handles.INTTC, 1);
    }
}

PPM_PACKETNET_CB(triggerSouth) {
    if (dataCount[3] == 0){
        unsigned int rx_data1 = *(unsigned int *)data;

        delivery[3] = XYrouting((Uns32)userData, rx_data1);
        
        deliveryPkt(delivery[3], rx_data1);

        dataCount[3]=1;
    }
    else{
        unsigned int rx_data2 = *(unsigned int *)data;
        dataCount[3]=0;

        deliveryPkt(delivery[3], rx_data2);

        ppmWriteNet(handles.INTTC, 1);
    }
}

PPM_REG_READ_CB(txRead1) {
    // YOUR CODE HERE (txRead1)
    return *(Uns32*)user;
}

PPM_REG_READ_CB(txRead2) {
    // YOUR CODE HERE (txRead2)
    return *(Uns32*)user;
}


/*if (dataCount[0] == 0){
        unsigned int rx_data1 = *(unsigned int *)data;

        delivery[0] = XYrouting((Uns32)userData, rx_data1);
        
        deliveryPkt(delivery[0], rx_data1);

        dataCount[0]=1;
    }
    else{
        unsigned int rx_data2 = *(unsigned int *)data;
        dataCount[0]=0;

        deliveryPkt(delivery[0], rx_data2);

        ppmWriteNet(handles.INTTC, 1);
    }
    */

PPM_REG_WRITE_CB(txWrite1) {
    if(dataCount[4] = 0){
        unsigned int tx_data1 = data;
        dataCount[4]=1;

        delivery[4] = XYrouting((Uns32)userData, rx_data1);

        deliveryPkt(delivery[4], tx_data1);
    }
    else{
        bhmMessage("I", "TX1", "Enviando o tx_data1 - ERRO!");
    }
    *(Uns32*)user = data;
}

PPM_REG_WRITE_CB(txWrite2) {
    if(dataCount[4] > 0){
        unsigned int tx_data2 = data;
        dataCount[4]=0;

        deliveryPkt(delivery[4], tx_data2);
    }
    else{
        bhmMessage("I", "TX1", "Enviando o tx_data2 - ERRO!");
    }
    *(Uns32*)user = data;
}

PPM_CONSTRUCTOR_CB(constructor) {
    unsigned int i;
    for(i=0;i<5;i++){
        dataCount[i] = 0;
        delivery[i] = 0;
    }
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

