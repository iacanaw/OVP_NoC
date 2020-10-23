#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "interrupt.h"
#include "spr_defs.h"
#include "source/API/api.h"

#include "thermalManagement_config.h"

message theMsg;
message theMsg2;

unsigned int toPeriph[(DIM_X*DIM_Y)+2+3+1];

unsigned int Power[DIM_X*DIM_Y];
unsigned int Temperature[DIM_X*DIM_Y];
unsigned int Frequency[DIM_X*DIM_Y];

//PID control variables
unsigned int derivative[DIM_Y*DIM_X];
unsigned int integral[DIM_Y*DIM_X];
unsigned int integral_prev[INT_WINDOW][DIM_Y*DIM_X];
unsigned int Temperature_prev[DIM_Y*DIM_X];
unsigned int control_signal[DIM_Y*DIM_X];

unsigned int spiralMatrix[DIM_X*DIM_Y];
unsigned int tempSort[DIM_X*DIM_Y];

void generateSpiralMatrix()
{
    int i, cont=0, k = 0, l = 0, m=DIM_X, n=DIM_Y;

    while (k < m && l < n)
    {
        /* Print the first row from the remaining rows */
        for (i = l; i < n; ++i){
            spiralMatrix[cont] = (k<<8) | i;
            cont++;
        }
        k++;
 
        /* Print the last column from the remaining columns */
        for (i = k; i < m; ++i){
            spiralMatrix[cont] = (i<<8) | (n-1);
            cont++;
        }
        n--;
 
        /* Print the last row from the remaining rows */
        if ( k < m){
            for (i = n-1; i >= l; --i){
                spiralMatrix[cont] = ((m-1)<<8) | i;
                cont++;
            }
            m--;
        }
 
        /* Print the first column from the remaining columns */
        if (l < n){
            for (i = m-1; i >= k; --i){
                spiralMatrix[cont] = (i<<8) | l;
                cont++;
            }
            l++;    
        }
    }
}

void generateTempMatrix(unsigned int temp[DIM_X*DIM_Y]){
    unsigned int proc_address[DIM_X*DIM_Y];
    unsigned int ordered_temp[DIM_X*DIM_Y];
    int i, j;

    memcpy(ordered_temp, temp, DIM_X*DIM_Y*4);
    /*for(i=0;i<DIM_X*DIM_Y; i++){
        ordered_temp[i] = temp[i];
    }*/

    proc_address[0] = 0;

    for (i = DIM_X*DIM_Y-1; i > 0; i--){
        int coolest = 1;
        for(j = 1; j < DIM_X*DIM_Y; j++)
            if(ordered_temp[j] < ordered_temp[coolest])
                coolest = j;

        proc_address[i] = (coolest%DIM_X << 8) | coolest/DIM_X;

        spiralMatrix[i] = proc_address[i];
        tempSort[i] = coolest;

        ordered_temp[coolest] = -1;
    }

}

int how_many_tasks_PE_is_running(unsigned int srcProc, unsigned int task_addr[DIM_X*DIM_Y]){
    int i;

    for(i=0; i<DIM_X*DIM_Y; i++){
        if (task_addr[i] == srcProc && finishedTask[i] == FALSE)
            return 1;
    }

    return 0;
}

int getSomeTaskID(unsigned int srcProc, unsigned int task_addr[DIM_X*DIM_Y]){
    int i;

    for(i=0; i<DIM_X*DIM_Y; i++)
        if (task_addr[i] == srcProc)
            return i;

    return -1; 
}

int temperature_migration(unsigned int temp[DIM_X*DIM_Y], unsigned int tasks_to_map, unsigned int task_addr[DIM_X*DIM_Y]){
    int task_ID;
    unsigned int tgtProc, srcProc, srcID;
    int k=DIM_X*DIM_Y-1;
    unsigned int contNumberOfMigrations=0;
    int i, j;
    int src_vec[DIM_X*DIM_Y];// = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

    int srcProcs[DIM_X*DIM_Y];


    for(i=0; i< DIM_X*DIM_Y; i++){
        src_vec[i] = 0;
        // clear finished applications
        if(finishedTask[i]==TRUE){
            task_addr[i] = 0;
        }
    }

    for (i = 1; i < DIM_X*DIM_Y; i++){
        srcID = tempSort[i];
        int x = srcID % DIM_X;
        int y = srcID / DIM_Y;
        srcProc = x << 8 | y;
        if (temp[srcID] > 33300){
            putsvsv("Temperature migration: srcProc=", srcProc, "how_many_tasks_PE_is_running=", how_many_tasks_PE_is_running(srcProc, task_addr));
            if (how_many_tasks_PE_is_running(srcProc, task_addr)>0){
                while (k>0){
                    tgtProc = spiralMatrix[k];
                    task_ID = getSomeTaskID(srcProc, task_addr);
                    putsvsv("Temperature migration: tgtProc=", tgtProc, " task_ID=", task_ID);
                    //LOG("Temperature migration: tgtProc= %x task_ID= %d\n", tgtProc, task_ID);

                    if ((how_many_tasks_PE_is_running(tgtProc, task_addr)==0) && (tgtProc != srcProc) && (how_many_tasks_PE_is_running(tgtProc, src_vec)==0)){
                        //LOG("send_task_migration %x -> %x\n", srcProc, tgtProc);
                        prints("send_task_migration\n");

                        task_addr[task_ID] = tgtProc;
                        src_vec[task_ID] = srcProc;

                        //Save to send later
                        srcProcs[contNumberOfMigrations] = srcProc;
                        //sendTaskService(TASK_MIGRATION_SRC, srcProc, task_addr, tasks_to_map);
                        
                        contNumberOfMigrations++;
                        //setEnergySlaveAcc_total(tgtProc); //zera energia acumulada do PE destino
                        break;
                    }
                    k--;
                }
            }
        }
        if(temp[i] > 35515 && Frequency[i] == 1000){
            //LOG("AJUSTANDO A FREQUENCIA DE %x", srcProc);
            Frequency[i] = 677;
            setDVFS(srcProc, Frequency[i]);
        }
        else if(temp[i] < 35515 && Frequency[i] == 677){
            //LOG("AJUSTANDO A FREQUENCIA DE %x", srcProc);
            Frequency[i] = 1000;
            setDVFS(srcProc, Frequency[i]);
        }
    }
    if(contNumberOfMigrations>0){
        for(i = 0; i < contNumberOfMigrations; i++)
            sendTaskService(TASK_MIGRATION_SRC, srcProcs[i], task_addr, tasks_to_map);
    }
    return contNumberOfMigrations;
}

int main(int argc, char **argv)
{
    OVP_init();
    //////////////////////////////////////////////////////
    /////////////// YOUR CODE START HERE /////////////////
    //////////////////////////////////////////////////////
    int y, x, p_idx = 0;
    //int ordem[DIM_X*DIM_Y];

    FILE *testcase;
    testcase = fopen("application/scenario.yaml","r");
    char line[64];
    char *app_name;
    char *starting_time_str;
    int starting_time;
    char *task_name;
    char *task_number;
    unsigned int yaml_tasks = 0;
    unsigned int task_addr[DIM_X*DIM_Y];
    unsigned int tasks_to_map = 0;
    int finishSimulation;

    Uns32 aux = MFSPR(SPR_PICMR);
    /*Initialization*/
    generateSpiralMatrix();
    for(y=0;y<DIM_Y;y++){
        for(x=0;x<DIM_X;x++){
            Power[p_idx] = 0;
            Frequency[p_idx] = 1000;
            Temperature[p_idx] = TAMB;
            integral[p_idx] = 0;
            Temperature_prev[p_idx] = 0;
            //LOG("spiralMatrix %d - %x\n", p_idx, spiralMatrix[p_idx]);
            putsvsv("spiralMatrix ", p_idx, "- ",  spiralMatrix[p_idx]);
            p_idx++;
        }
    }

    /* YAML TEMPORARY PARSER TO MAP TASKS */
    while(fgets(line, sizeof(line), testcase)){

        if (strstr(line, "name") != NULL){
            app_name = strtok(line, ":");
            app_name = strtok(NULL, " ");
            app_name[strlen(app_name)-1] = '\0';
            yaml_tasks = 0;
            starting_time = 0; // defines the starting time to zero
        }

        if (yaml_tasks){
            task_name = strtok(line, " ");
            task_name[strlen(task_name)-1] = '\0';
            task_number = strtok(NULL, " ");
            task_number[strlen(task_number)-1] = '\0';
            task_addr[tasks_to_map] = atoi(task_number);
            tasks_to_map++;
        }

        if (strstr(line, "dynamic_mapping") != NULL){
            yaml_tasks = 1;
        }

        if(strstr(line, "start_time_ms") != NULL){
            starting_time_str = strtok(line, ":");
            starting_time_str = strtok(NULL, " ");
            starting_time_str[strlen(app_name)-1] = '\0';
            starting_time = atoi(starting_time_str);
            prints("App: ");
            prints(app_name);
            prints(" starting at: ");
            printi(starting_time);
            prints("\n");
        }
    }

    int i;
    for(i = 0; i < tasks_to_map; i++){
        task_addr[i] = spiralMatrix[DIM_X*DIM_Y-1-i];
        //LOG("Task %d mapped in processor %x\n", i, task_addr[i]);
        putsvsv("Task", i, " mapped in processor ", task_addr[i]);
    }

    for(i = tasks_to_map; i < DIM_X*DIM_Y; i++)
        task_addr[i] = 0;

    for(i = 0; i < tasks_to_map; i++)
        sendTaskService(TASK_MAPPING, task_addr[i], task_addr, tasks_to_map);

    /* Wait for every PE to send each power estimation */
    if(*timerConfig != 0){
        while(*SyncToPE != 1){ // Repete este processo enquanto houverem outras tarefas executando!
            putsv("Tasks to map: ", tasks_to_map);
            //////////////////////////////////////////////////////
            // RECEIVE THE PACKET FROM TEA WITH PE TEMPERATURES //
            //////////////////////////////////////////////////////
            while(!tempPacket){
#if USE_THERMAL
                *clockGating_flag = TRUE;
#endif
            }
#if USE_THERMAL
            *clockGating_flag = FALSE;
#endif
            tempPacket = FALSE;
            prints("TEA Packet Received: ");
            for(i = 0; i < DIM_X*DIM_Y; i++){
                //printi(deliveredMessage->msg[i]);
                printi(executedInstPacket[i]);
                Temperature[i] = executedInstPacket[i];//deliveredMessage->msg[i];
            }

            //////////////////////////
            // Migration procedures //
            //////////////////////////
            prints("\nGenerating TempMatrix\n");
            for(i = 0; i < DIM_X*DIM_Y; i++){

                if (measuredWindows >= INT_WINDOW)
                    integral[i] = integral[i] - integral_prev[measuredWindows%INT_WINDOW][i];

                integral_prev[measuredWindows%INT_WINDOW][i] = Temperature[i];

                //if (measuredWindows != 0) energy_i[i] = getEnergySlaveAcc_total(i)/measuredWindows;
                derivative[i] = Temperature[i] - Temperature_prev[i];
                integral[i] = integral[i] + Temperature[i];
                control_signal[i] = KP*Temperature[i] + KI*integral[i]/INT_WINDOW + KD*derivative[i];
                Temperature_prev[i] = Temperature[i];

                // putsv("proc ", i);
                // putsv("energy ", energy_i[i]);
                // putsv("control_signal ", control_signal[i]);
            }
            generateTempMatrix(control_signal);

            if ((measuredWindows)%20 == 0){
                prints("Starting thermal actuation analysis\n");
                temperature_migration(Temperature, tasks_to_map, task_addr);
                // for(i = 0; i < tasks_to_map; i++)
                //     sendTaskService(TASK_MIGRATION_SRC, task_addr[i], task_addr, tasks_to_map);

                // for(i = 0; i < tasks_to_map; i++){
                //     task_addr[i] = spiralMatrix[DIM_X*DIM_Y-1-i];
                //     LOG("Task %d migrate to processor %x\n", i, task_addr[i]);
                // }
                // for(i=0;i<tasks_to_map;i++)
                //     sendTaskService(TASK_MIGRATION_DEST, task_addr[i], task_addr, tasks_to_map);
            }
            else{
                prints("Skiping thermal actuation analysis\n");
            }

            // Verify if every task is finished
            finishSimulation = 1;
            for(i = 0; i < tasks_to_map; i++){
                if(finishedTask[i]==FALSE){
                    finishSimulation = 0;
                    break;
                }
            }
            if(finishSimulation){
                for(i = 1; i < N_PES; i++){
                    sendTaskService(PE_FINISH_SIMULATION, getAddress(i), 0, 0);
                }
                break;
            }

        }
        measuredWindows = 0;
        while(*SyncToPE != 1){
            putsv("Waiting everyone ", measuredWindows);
            measuredWindows++;    
        }
    }
    //////////////////////////////////////////////////////
    //////////////// YOUR CODE ENDS HERE /////////////////
    //////////////////////////////////////////////////////
    FinishApplication();
    return 1;
}
