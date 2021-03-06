
imodelnewperipheral -name router \
                    -constructor constructor \
                    -destructor  destructor \
                    -vendor gaph \
                    -library peripheral \
                    -version 1.0 

iadddocumentation -name Description \
                  -text "A OVP Wormhole Router"

#########################################
## A slave port on the processor bus
#########################################
imodeladdbusslaveport -name localPort \
                      -size 20 \
                      -mustbeconnected

#########################################
## The whole address space of the slave port (four integers)
## is taken up by four registers
#########################################
imodeladdaddressblock -name regs   \
                      -port localPort \
                      -offset 0x0  \
                      -width 32    \
                      -size 20

imodeladdmmregister  -name myAddress \
                     -readfunction   addressRead \
                     -writefunction  addressWrite \
                     -offset 0 

imodeladdmmregister  -name dataTxLocal \
                     -readfunction   txRead \
                     -writefunction  txWrite \
                     -offset 4

imodeladdmmregister  -name dataRxLocal \
                     -readfunction   rxRead \
                     -writefunction  rxWrite \
                     -offset 8

imodeladdmmregister  -name controlRxLocal \
                     -readfunction   rxCtrlRead \
                     -writefunction  rxCtrlWrite \
                     -offset 12

imodeladdmmregister  -name controlTxLocal \
                     -readfunction   txCtrlRead \
                     -writefunction  txCtrlWrite \
                     -offset 16 


#########################################
## Data ports between routers
#########################################
imodeladdpacketnetport \
    -name portDataEast \
    -maxbytes 4 \
    -updatefunction dataEast \
    -updatefunctionargument 0x00

imodeladdpacketnetport \
    -name portDataWest \
    -maxbytes 4 \
    -updatefunction dataWest \
    -updatefunctionargument 0x00

imodeladdpacketnetport \
    -name portDataNorth \
    -maxbytes 4 \
    -updatefunction dataNorth \
    -updatefunctionargument 0x00

imodeladdpacketnetport \
    -name portDataSouth \
    -maxbytes 4 \
    -updatefunction dataSouth \
    -updatefunctionargument 0x00

#########################################
## To Tick
#########################################

imodeladdpacketnetport \
    -name iterationsPort \
    -maxbytes 8 \
    -updatefunction iterationPort \
    -updatefunctionargument 0x00

#########################################
## Control ports between routers
#########################################
imodeladdpacketnetport \
    -name portControlEast \
    -maxbytes 8 \
    -updatefunction controlEast \
    -updatefunctionargument 0x00

imodeladdpacketnetport \
    -name portControlWest \
    -maxbytes 8 \
    -updatefunction controlWest \
    -updatefunctionargument 0x00

imodeladdpacketnetport \
    -name portControlNorth \
    -maxbytes 8 \
    -updatefunction controlNorth \
    -updatefunctionargument 0x00

imodeladdpacketnetport \
    -name portControlSouth \
    -maxbytes 8 \
    -updatefunction controlSouth \
    -updatefunctionargument 0x00


#########################################
## Processor interrupt line
#########################################
imodeladdnetport -name INTTC -type output
iadddocumentation -name Description -text "Interrupt Request"
