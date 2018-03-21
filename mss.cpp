// Project: Merge-splitting sort
// Author: Nikola Valesova, xvales02
// Date: 8. 4. 2018

#include <mpi.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <math.h>
#include <algorithm>


using namespace std;

#define TAG 0


void printResult(vector<int> ordered) {
    //cout<<cycles<<endl;
    for (int i = 0; i < ordered.size(); i++)
        cout << ordered[i] << endl;
}


vector<int> mergeLists(int *myNums, int *neighNums, int myCount, int neighCount) {
    int myPtr = 0;
    int neighPtr = 0;
    vector<int> ordered;

    while (myPtr < myCount && neighPtr < neighCount) {
        if (myNums[myPtr] < neighNums[neighPtr])
            ordered.push_back(myNums[myPtr++]);
        else
            ordered.push_back(neighNums[neighPtr++]);
    }

    while (myPtr < myCount)
        ordered.push_back(myNums[myPtr++]);

    while (neighPtr < neighCount)
        ordered.push_back(neighNums[neighPtr++]);

    return ordered;
}


int main(int argc, char *argv[])
{
    int numprocs;               //pocet procesoru
    int myid;                   //muj rank
    int neighCount;             // number of values being returned from neighbour
    int myCount;                // my count of given numbers to order
    MPI_Status stat;            //struct- obsahuje kod- source, tag, error
    vector<int> ordered;

    //MPI INIT
    MPI_Init(&argc, &argv);                          // inicializace MPI 
    MPI_Comm_size(MPI_COMM_WORLD, &numprocs);       // zjisti­me, kolik procesu bezi
    MPI_Comm_rank(MPI_COMM_WORLD, &myid);           // zjistime id sveho procesu 
 
    //NACTENI SOUBORU
    /* -proc s rankem 0 nacita vsechny hodnoty
     * -postupne rozesle jednotlive hodnoty vsem i sobe
    */
    if (myid == 0) {
        vector<int> numbers;
        int invar = 0;                                  //invariant- urcuje cislo proc, kteremu se bude posilat
        fstream fin;                                    //cteni ze souboru
        int numsPerProc;                                // numbers per processor
        int procsWithMoreNums;                          // number of processors that will have more numbers
        int sendFromIdx = 0;
        fin.open("numbers", ios::in);                   

        while (fin.good())
            numbers.push_back(fin.get());

        for (vector<int>::size_type i = 0; i != numbers.size() - 1; i++)
            cout << numbers[i] << " ";
        cout << endl;

        numsPerProc = ceil((float(numbers.size() - 1)) / numprocs);
        procsWithMoreNums = (numbers.size() - 1) % numprocs;  // tolik procs dostane plnej pocet cisel, dalsi o jedna min 
        if (procsWithMoreNums != 0)
            procsWithMoreNums++;

        for (int i = 0; i < numprocs; i++) {
            if (i == procsWithMoreNums - 1)
                numsPerProc--;

            MPI_Send(&numsPerProc, 1, MPI_INT, i, TAG, MPI_COMM_WORLD); //buffer, velikost, typ, rank prijemce, tag, komunikacni skupina
            MPI_Send(&numbers[sendFromIdx], numsPerProc, MPI_INT, i, TAG, MPI_COMM_WORLD); //buffer, velikost, typ, rank prijemce, tag, komunikacni skupina

            sendFromIdx += numsPerProc;
        }
        fin.close();                                
    }

    //PRIJETI HODNOTY CISLA
    //vsechny procesory(vcetne mastera) prijmou hodnotu a zahlasi ji
    MPI_Recv(&myCount, 1, MPI_INT, 0, TAG, MPI_COMM_WORLD, &stat); //buffer, velikost, typ, rank odesilatele, tag, skupina, stat
    int myNums[myCount];
    MPI_Recv(&myNums, myCount, MPI_INT, 0, TAG, MPI_COMM_WORLD, &stat); //buffer, velikost, typ, rank odesilatele, tag, skupina, stat
    // cout << "i am: " << myid << " my numbers are: ";
    // for (int x = 0; x < myCount; x++)
    //     cout << myNums[x] << ", ";
    // cout << endl;

    int halfcycles = ceil(numprocs / 2.0);
    int cycles = 0;                                   //pocet cyklu pro pocitani slozitosti


    //RAZENI
    sort(myNums, myNums + myCount);
    for (int j = 1; j <= halfcycles; j++) {
        cycles++;           //pocitame cykly, abysme mohli udelat krasnej graf:)

        // //sude proc 
        if (myid % 2 == 0 && myid < numprocs - 1) {
            MPI_Recv(&neighCount, 1, MPI_INT, myid + 1, TAG, MPI_COMM_WORLD, &stat); //jsem 0 a prijimam
            int neighNums[neighCount];
            MPI_Recv(&neighNums, neighCount, MPI_INT, myid + 1, TAG, MPI_COMM_WORLD, &stat); //jsem 0 a prijimam

            ordered = mergeLists(myNums, neighNums, myCount, neighCount);
            copy(ordered.begin(), ordered.begin() + myCount, myNums);
            copy(ordered.begin() + myCount, ordered.end(), neighNums);

            MPI_Send(&neighNums, neighCount, MPI_INT, myid + 1, TAG, MPI_COMM_WORLD);
        }
        else if (myid % 2 == 1) {
            MPI_Send(&myCount, 1, MPI_INT, myid - 1, TAG, MPI_COMM_WORLD);               //poslu sousedovi svoje cisla
            MPI_Send(&myNums, myCount, MPI_INT, myid - 1, TAG, MPI_COMM_WORLD);          //poslu sousedovi svoje cisla
            MPI_Recv(&myNums, myCount, MPI_INT, myid - 1, TAG, MPI_COMM_WORLD, &stat);   //a cekam na nizsi
        }

        //liche proc 
        if (myid % 2 == 1 && myid < numprocs - 1) {
            MPI_Recv(&neighCount, 1, MPI_INT, myid + 1, TAG, MPI_COMM_WORLD, &stat); //jsem sudy a prijimam
            int neighNums[neighCount];
            MPI_Recv(&neighNums, neighCount, MPI_INT, myid + 1, TAG, MPI_COMM_WORLD, &stat); //jsem sudy a prijimam

            ordered = mergeLists(myNums, neighNums, myCount, neighCount);
            copy(ordered.begin(), ordered.begin() + myCount, myNums);
            copy(ordered.begin() + myCount, ordered.end(), neighNums);

            MPI_Send(&neighNums, neighCount, MPI_INT, myid + 1, TAG, MPI_COMM_WORLD);
        }
        else if (myid % 2 == 0 && myid > 0) {
            MPI_Send(&myCount, 1, MPI_INT, myid - 1, TAG, MPI_COMM_WORLD);               //poslu sousedovi svoje cisla
            MPI_Send(&myNums, myCount, MPI_INT, myid - 1, TAG, MPI_COMM_WORLD);          //poslu sousedovi svoje cisla
            MPI_Recv(&myNums, myCount, MPI_INT, myid - 1, TAG, MPI_COMM_WORLD, &stat);   //a cekam na nizsi
        }
        // else
        //     cout << "NOT GOINT ANYWHERE 2 - " << myid << endl;        
    }
    // //RAZENI--------------------------------------------------------------------


    //FINALNI DISTRIBUCE VYSLEDKU K MASTEROVI-----------------------------------
    if (myid == 0) {
        ordered.clear();
        ordered.insert(ordered.end(), myNums, myNums + myCount);
    }

    for (int i = 1; i < numprocs; i++) {
        if (myid == i) {
            MPI_Send(&myCount, 1, MPI_INT, 0, TAG,  MPI_COMM_WORLD);
            MPI_Send(&myNums, myCount, MPI_INT, 0, TAG,  MPI_COMM_WORLD);
        }
        if (myid == 0) {
            MPI_Recv(&neighCount, 1, MPI_INT, i, TAG, MPI_COMM_WORLD, &stat); //jsem 0 a prijimam
            int neighNums[neighCount];
            MPI_Recv(&neighNums, neighCount, MPI_INT, i, TAG, MPI_COMM_WORLD, &stat); //jsem 0 a prijimam
            ordered.insert(ordered.end(), neighNums, neighNums + neighCount);
        }
    }

    // output ordered sequence
    if (myid == 0)
        printResult(ordered);

    MPI_Finalize(); 
    return 0;
}

// TODO comments, parse code into functions