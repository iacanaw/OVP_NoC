////////////////////////////////////////////////////////////////////////////////
//
//                W R I T T E N   B Y   I M P E R A S   I G E N
//
//                             Version 20191106.0
//
////////////////////////////////////////////////////////////////////////////////


#include "iteratorMonoTrigger.igen.h"
/////////////////////////////// Port Declarations //////////////////////////////

iteratorReg_ab8_dataT iteratorReg_ab8_data;

handlesT handles;

// Iteration counter
unsigned long long int iterationN = 0;
unsigned long long int iteration;
unsigned int activity;

////////////////////////////// Functions ///////////////////////////////////////
/* Make a callback to each router */
void runIterations(){  
    unsigned int i=0;
    unsigned int tryAgain = 0;
    do{
        activity = 0;
        iterationN++;
        iteration = iterationN;
        ppmPacketnetWrite(handles.iterationPort0, &iteration, sizeof(iteration));
        iteration = iterationN;
        ppmPacketnetWrite(handles.iterationPort1, &iteration, sizeof(iteration));
        iteration = iterationN;
        ppmPacketnetWrite(handles.iterationPort2, &iteration, sizeof(iteration));
        iteration = iterationN;
        ppmPacketnetWrite(handles.iterationPort3, &iteration, sizeof(iteration));
        i++;
        if(tryAgain == 0 && activity == 0){
            activity++; 
            tryAgain++;// segunda chance
        }
        else{
            tryAgain = 0;
        }
    }while(activity != 0);
}

/////////////////////////////// Diagnostic level ///////////////////////////////

// Test this variable to determine what diagnostics to output.
// eg. if (diagnosticLevel >= 1) bhmMessage(I, iteratorMonoTrigger, Example);
//     Predefined macros PSE_DIAG_LOW, PSE_DIAG_MEDIUM and PSE_DIAG_HIGH may be used
Uns32 diagnosticLevel;

/////////////////////////// Diagnostic level callback //////////////////////////

static void setDiagLevel(Uns32 new) {
    diagnosticLevel = new;
}

///////////////////////////// MMR Generic callbacks ////////////////////////////

static PPM_VIEW_CB(view32) {  *(Uns32*)data = *(Uns32*)user; }

//////////////////////////////// Bus Slave Ports ///////////////////////////////

static void installSlavePorts(void) {
    handles.iteratorReg = ppmCreateSlaveBusPort("iteratorReg", 4);
    if (!handles.iteratorReg) {
        bhmMessage("E", "PPM_SPNC", "Could not connect port 'iteratorReg'");
    }

}

//////////////////////////// Memory mapped registers ///////////////////////////

static void installRegisters(void) {

    ppmCreateRegister("ab8_iterationPort",
        0,
        handles.iteratorReg,
        0,
        4,
        iterateRead,
        iterateWrite,
        view32,
        &(iteratorReg_ab8_data.iterationPort.value),
        True
    );

}

////////////////////////////////// Constructor /////////////////////////////////

PPM_CONSTRUCTOR_CB(periphConstructor) {
    installSlavePorts();
    installRegisters();
}


//////////////////////////////// Callback stubs ////////////////////////////////

/* Read callback from the extra PE */
PPM_REG_READ_CB(iterateRead) { 
    iterationN++;
    runIterations();
    return *(Uns32*)user;
}

PPM_REG_WRITE_CB(iterateWrite) {
    // YOUR CODE HERE (iterateWrite)
    *(Uns32*)user = data;
}

PPM_PACKETNET_CB(iteration0) {
    unsigned int act = *(unsigned int *)data;
    if(act > 0){
        activity++;
    }
 }

PPM_PACKETNET_CB(iteration1) {
    unsigned int act = *(unsigned int *)data;
    if(act > 0){
        activity++;
    }
 }

PPM_PACKETNET_CB(iteration2) {
    unsigned int act = *(unsigned int *)data;
    if(act > 0){
        activity++;
    }
 }

PPM_PACKETNET_CB(iteration3) {
    unsigned int act = *(unsigned int *)data;
    if(act > 0){
        activity++;
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

PPM_DOC_FN(installDocs){
    ppmDocNodeP Root1_node = ppmDocAddSection(0, "Root");
    {
        ppmDocNodeP doc2_node = ppmDocAddSection(Root1_node, "Description");
        ppmDocAddText(doc2_node, "The NoC iteratorMonoTrigger");
    }
}
///////////////////////////////////// Main /////////////////////////////////////

int main(int argc, char *argv[]) {

    diagnosticLevel = 0;
    bhmInstallDiagCB(setDiagLevel);
    constructor();

    while(1){
        bhmWaitDelay(QUANTUM_DELAY);
        runIterations();
    }


    bhmWaitEvent(bhmGetSystemEvent(BHM_SE_END_OF_SIMULATION));
    destructor();
    return 0;
}
