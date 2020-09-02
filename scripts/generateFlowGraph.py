import csv
import numpy as np
import os
import operator
import matplotlib.colors as mcolors
import matplotlib.pyplot as plt
import matplotlib.pylab as pylab
import seaborn as sns
from matplotlib.ticker import LinearLocator
from mpl_toolkits.mplot3d import Axes3D
from matplotlib import cm





DIM_X = 4
DIM_Y = 4
N_PES = DIM_X*DIM_Y
NUM_OF_GRAPHS = 5

localFlow = [0 for i in range(N_PES)]
eastFlow = [0 for i in range(N_PES)]
westFlow = [0 for i in range(N_PES)]
northFlow = [0 for i in range(N_PES)]
southFlow = [0 for i in range(N_PES)]
maxValue = 0

def getMaxValue(local,east,west,north,south,max):
    if local > max:
        max = local
    if east > max:
        max = east
    if west > max:
        max = west
    if north > max:
        max = north
    if south > max:
        max = south
    return max

def printToGraph(id, graph, quantunsPerGraph, local, east, west, north, south):
    myY = int(id/DIM_X)
    myX = int(id-(DIM_X*myY))
    centralX = (myX*3) + 1
    centralY = (myY*3) + 1
    filename = "myGraphs/graph"+str(graph)+".dat"
    os.makedirs(os.path.dirname(filename), exist_ok=True)
    with open(filename,"a+") as gfile:
        # canto esquerdo inferior (nada)
        print(str((centralX-1))+" "+str((centralY-1))+" "+str((graph+1)*quantunsPerGraph)+" "+str(999999), file=gfile)
        # meio inferior (south)
        print(str((centralX))+" "+str((centralY-1))+" "+str((graph+1)*quantunsPerGraph)+" "+str(south), file=gfile)
        # canto direito inferior (nada)
        print(str((centralX+1))+" "+str((centralY-1))+" "+str((graph+1)*quantunsPerGraph)+" "+str(999999), file=gfile)
        
        # borda esquerda (west)
        print(str((centralX-1))+" "+str((centralY))+" "+str((graph+1)*quantunsPerGraph)+" "+str(west), file=gfile)
        # central (local)
        print(str((centralX))+" "+str((centralY))+" "+str((graph+1)*quantunsPerGraph)+" "+str(999998), file=gfile)
        # borda direita (east)
        print(str((centralX+1))+" "+str((centralY))+" "+str((graph+1)*quantunsPerGraph)+" "+str(east), file=gfile)
        
        # canto esquerdo superior (nada)
        print(str((centralX-1))+" "+str((centralY+1))+" "+str((graph+1)*quantunsPerGraph)+" "+str(999999), file=gfile)
        # meio superior (north)
        print(str((centralX))+" "+str((centralY+1))+" "+str((graph+1)*quantunsPerGraph)+" "+str(north), file=gfile)
        # canto direito superior (nada)
        print(str((centralX+1))+" "+str((centralY+1))+" "+str((graph+1)*quantunsPerGraph)+" "+str(999999), file=gfile)

def printGraphFile(graph, quantunsPerGraph, graphMatrix):
    filename = "myGraphs/graph"+str(graph)+".dat"
    with os.open(filename, "w+") as gfile:
        for i in range(int(3*DIM_X)):
            for j in range(int(3*DIM_Y)):
                print(str(i)+" "+str(j)+" "+str((graph+1)*quantunsPerGraph)+" "+str(graphMatrix[i][j]), file=gfile)

def toGraph(graphMatrix, id, local, east, west, north, south):
    myY = int(id/DIM_X)
    myX = int(id-(DIM_X*myY))
    centralX = (myX*3) + 1
    centralY = (myY*3) + 1

    # canto esquerdo inferior (nada)
    graphMatrix[centralX-1][centralY-1] = 1000000
    # meio inferior (south)
    graphMatrix[centralX][centralY-1] = south
    # canto direito inferior (nada)
    graphMatrix[centralX+1][centralY-1] = 1000000

    # borda esquerda (west)
    graphMatrix[centralX-1][centralY] = west
    # central (south)
    graphMatrix[centralX][centralY] = 1010000
    # borda direita (east)
    graphMatrix[centralX+1][centralY] = east

    # canto esquerdo superior (nada)
    graphMatrix[centralX-1][centralY+1] = 1000000
    # meio superior (north)
    graphMatrix[centralX][centralY+1] = north
    # canto direito superior (nada)
    graphMatrix[centralX+1][centralY+1] = 1000000

    return graphMatrix

if __name__ == '__main__':

    with open(r"D:\\GitRepo\\OVP_NoC\\simulation\\flitsLog.txt") as csv_file:
    #with open('../simulation/flitsLog.txt') as csv_file:
        spamreader = csv.reader(csv_file, delimiter=',')
        #TODO: pegar automaticamente o numero de quantuns
        numQuantuns = 976
        quantunsPerGraph = numQuantuns/NUM_OF_GRAPHS

        # create the figure, add a 3d axis, set the viewing angle
        fig = plt.figure()
        ax = fig.add_subplot(1,1,1, projection='3d')
        ax.view_init(45,60)
        ax.pbaspect = np.array([1.0, 1.0, 3.0])
        
        pes_total = np.zeros(N_PES)
        graph_simples = np.zeros((int(1*DIM_X),int(1*DIM_Y)))
        print(graph_simples)

        # Criar um grafico pra cada camada
        for graph in range(NUM_OF_GRAPHS):
            graphMatrix = np.zeros((int(3*DIM_X),int(3*DIM_Y)))
            
            # Pega as linhas necessárias pra cada gráfico
            for _ in range(int(quantunsPerGraph)):
                for i in range(N_PES):
                    try:
                        theLine = next(spamreader)
                    except:
                        break;
                    localFlow[i] += int(theLine[2])
                    eastFlow[i]  += int(theLine[3])
                    westFlow[i]  += int(theLine[4])
                    northFlow[i] += int(theLine[5])
                    southFlow[i] += int(theLine[6])
            
            contx = 0
            conty = 0
            for i in range(N_PES):
                #print(str(graph)+";"+str(i)+";"+str(localFlow[i])+";"+str(eastFlow[i])+";"+str(westFlow[i])+";"+str(northFlow[i])+";"+str(southFlow[i]))
                graph_simples[contx][conty] = localFlow[i] + eastFlow[i] + westFlow[i] + northFlow[i] + southFlow[i]
                contx += 1
                if(contx == DIM_X):
                    conty += 1
                    contx = 0
            print("Mais um grafico:")
            print(graph_simples)

            # Depois que leu tudo, printa o gráfico no formato especifico do gnuplot
            for i in range(N_PES):
                maxValue = getMaxValue(localFlow[i], eastFlow[i], westFlow[i], northFlow[i], southFlow[i], maxValue)
                graphMatrix = toGraph(graphMatrix, i, localFlow[i], eastFlow[i], westFlow[i], northFlow[i], southFlow[i])
                #printToGraph(i, graph, quantunsPerGraph, localFlow[i], eastFlow[i], westFlow[i], northFlow[i], southFlow[i])
                #print(str(i)+" "+str((graph+1)*quantunsPerGraph)+" "+str(localFlow[i])+" "+str(eastFlow[i])+" "+str(westFlow[i])+" "+str(northFlow[i])+" "+str(southFlow[i]))
                localFlow[i] = 0
                eastFlow[i]  = 0
                westFlow[i]  = 0
                northFlow[i] = 0
                southFlow[i] = 0
            #printGraphFile(graph, quantunsPerGraph, graphMatrix)
            x = np.arange(0,3*DIM_X,1) 
            y = np.arange(0,3*DIM_Y,1)
            X, Y = np.meshgrid(x, y)
            Z = np.zeros((int(3*DIM_X),int(3*DIM_Y))) + ((graph+1)*quantunsPerGraph)
            

            # grafico simples
            x_simples = np.arange(0,int(1*DIM_X),1)
            y_simples = np.arange(0,int(1*DIM_Y),1)
            X_simples, Y_simples = np.meshgrid(x_simples,y_simples)
            Z_simples = np.zeros((int(1*DIM_X),int(1*DIM_Y))) + ((graph+1)*quantunsPerGraph)

            # here we create the surface plot, but pass V through a colormap
            # to create a different color for each patch
            #my_colors=[[0, 0xFFBA08],
            #           [1.5*(maxValue/7)/1010000, 0xFAA307], #0,0227
            #           [1.5*(maxValue/7)/1010000, 0xFAA307],
            #           [2*(maxValue/7)/1010000, 0xF48C06], #0,0064
            #           [2*(maxValue/7)/1010000, 0xF48C06],
            #           [3*(maxValue/7)/1010000, 0xE85D04], #0,0097
            #           [3*(maxValue/7)/1010000, 0xE85D04],
            #           [4*(maxValue/7)/1010000, 0xDC2F02], #0,012
            #           [4*(maxValue/7)/1010000, 0xDC2F02],
            #           [5*(maxValue/7)/1010000, 0xD00000],
            #           [5*(maxValue/7)/1010000, 0xD00000],
            #           [6*(maxValue/7)/1010000, 0x9D0208], #0,0194
            #           [6*(maxValue/7)/1010000, 0x9D0208], 
            #           [(maxValue/7)/1010000, 0x6A040F],
            #           [(maxValue/7)/1010000, 0x6A040F],
            #           [1000000/1010000, 0xd1cfc9], # nada
            #           [1, 0x000000]] # router (black)
            
            # "simples"
            #graph_simples = graph_simples/graph_simples.max()
            scam = plt.cm.ScalarMappable(norm=cm.colors.Normalize(0, graph_simples.max()), cmap='autumn') # see https://matplotlib.org/examples/color/colormaps_reference.html
            ax.plot_surface(X_simples, Y_simples, Z_simples, facecolors  = scam.to_rgba(graph_simples), antialiased = True, rstride=1, cstride=1, alpha=None)
            
            # "desenhado"
            #ax.plot_surface(X, Y, Z, facecolors=cm.jet(graphMatrix),linewidth=0, antialiased=False, shade=False)
            ax.w_zaxis.set_major_locator(LinearLocator(5))
            
        
        plt.show()
        
        
    print(maxValue)