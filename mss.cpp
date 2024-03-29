// Project: Merge-splitting sort
// Author:  Nikola Valesova, xvales02
// Date:    8. 4. 2018


#include <mpi.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <math.h>
#include <algorithm>


using namespace std;


#define TAG 0
MPI_Status stat;            // struct for storing MPI transaction result


// output ordered sequence
void printResult(vector<int> ordered) {
    for (int i = 0; i < ordered.size(); i++)
        cout << ordered[i] << endl;
}

// apply merge sort on myNums and neighNums
void mergeLists(int *myNums, int *neighNums, int myCount, int neighCount) {
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

    copy(ordered.begin(), ordered.begin() + myCount, myNums);
    copy(ordered.begin() + myCount, ordered.end(), neighNums);
}

// print input unordered sequence 
void printUnorderedSeq(vector<int> numbers) {
    for (vector<int>::size_type i = 0; i != numbers.size() - 2; i++)
        cout << numbers[i] << " ";
    cout << numbers[numbers.size() - 2];
    cout << endl;
}

// send current proccesor's numbers to processor procID
void sendNumbers(int procID, int myCount, int *myNums) {
    MPI_Send(&myCount, 1, MPI_INT, procID, TAG, MPI_COMM_WORLD);
    MPI_Send(myNums, myCount, MPI_INT, procID, TAG, MPI_COMM_WORLD);
}

// get numbers from input file
void getInputSequence(vector<int> &numbers) {
    fstream fin;                                // input filestream
    fin.open("numbers", ios::in);                   
    // read file
    while (fin.good())
        numbers.push_back(fin.get());
    fin.close();
}

// send current proccesor's numbers to processor neighID and wait for a half ofordered sequence
void waitForOrdered(int neighID, int count, int *myNums) {
    sendNumbers(neighID, count, myNums);
    MPI_Recv(myNums, count, MPI_INT, neighID, TAG, MPI_COMM_WORLD, &stat);
}

// receive neighbour's numbers, order neighbour's numbers with yours 
void receiveAndOrder(int neighID, int myCount, int *myNums) {
    vector<int> ordered;

    // receive numbers from neighbour
    int neighCount;             // number of values being returned from neighbour    
    MPI_Recv(&neighCount, 1, MPI_INT, neighID, TAG, MPI_COMM_WORLD, &stat);
    int *neighNums = new int [neighCount];
    MPI_Recv(neighNums, neighCount, MPI_INT, neighID, TAG, MPI_COMM_WORLD, &stat);

    // merge my numbers with neighbour's
    mergeLists(myNums, neighNums, myCount, neighCount);

    // send neighbour second half of ordered numbers
    MPI_Send(neighNums, neighCount, MPI_INT, neighID, TAG, MPI_COMM_WORLD);
    delete[] neighNums;
}

// execute odd step of ordering algorithm
void executeOddStep(int myID, int numprocs, int myCount, int *myNums) {
    // even numbered processors
    if (myID % 2 == 0 && myID < numprocs - 1)
        receiveAndOrder(myID + 1, myCount, myNums);
    // odd numbered processors 
    else if (myID % 2 == 1)
        waitForOrdered(myID - 1, myCount, myNums);
}

// execute even step of ordering algorithm
void executeEvenStep(int myID, int numprocs, int myCount, int *myNums) {
    // odd numbered processors
    if (myID % 2 == 1 && myID < numprocs - 1)
        receiveAndOrder(myID + 1, myCount, myNums);
    // even numbered processors
    else if (myID % 2 == 0 && myID > 0)
        waitForOrdered(myID - 1, myCount, myNums);
}

// order input numbers
void orderSequence(int myID, int numprocs, int myCount, int *myNums) {
    int halfcycles = ceil(numprocs / 2.0) + 1;

    // sort given numbers
    sort(myNums, myNums + myCount);

    // execute sorting algorithm
    for (int j = 0; j < halfcycles; j++) {
        executeOddStep(myID, numprocs, myCount, myNums);
        executeEvenStep(myID, numprocs, myCount, myNums);
    }
}

// processors 1 to n send their result sequences, processor 0 receives 
vector<int> finalStage(int myID, int numprocs, int myCount, int *myNums) {
    int neighCount;             // number of values being returned from neighbour
    vector<int> ordered;

    for (int i = 1; i < numprocs; i++) {
        // send numbers
        if (myID == i)
            sendNumbers(0, myCount, myNums);

        // receive numbers
        if (myID == 0) {
            MPI_Recv(&neighCount, 1, MPI_INT, i, TAG, MPI_COMM_WORLD, &stat);
            int *neighNums = new int [neighCount];
            MPI_Recv(neighNums, neighCount, MPI_INT, i, TAG, MPI_COMM_WORLD, &stat);
            ordered.insert(ordered.end(), neighNums, neighNums + neighCount);
            delete[] neighNums;
        }
    }
    return ordered;
}



int main(int argc, char *argv[])
{
    int numprocs;               // number of processors
    int myID;                   // my processor id
    int myCount;                // my count of given numbers to order
    vector<int> numbers;        // input numbers to order
    int numsPerProc;            // numbers per processor
    int procsWithMoreNums;      // number of processors that will have more numbers

    // MPI initialization
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &myID);
 
    // read numbers from input file and send them to appropriate processors 
    if (myID == 0) {
        int sendFromIdx = 0;    // index of first number being sent
        // read file
        getInputSequence(numbers);
        // output unordered sequence
        if (numbers.size() > 1)
            printUnorderedSeq(numbers);

        numsPerProc = ceil((float(numbers.size() - 1)) / numprocs);
        procsWithMoreNums = (numbers.size() - 1) % numprocs;
        if (procsWithMoreNums != 0)
            procsWithMoreNums++;

        // send amount of numbers assigned to processor
        for (int i = 0; i < numprocs; i++) {
            if (i == procsWithMoreNums - 1)
                numsPerProc--;
            MPI_Send(&numsPerProc, 1, MPI_INT, i, TAG, MPI_COMM_WORLD);
            sendFromIdx += numsPerProc;
        }
    }

    // receive assigned number of values
    MPI_Recv(&myCount, 1, MPI_INT, 0, TAG, MPI_COMM_WORLD, &stat);
    int *myNums = new int [myCount];

    if (myID == 0) {
        int sendFromIdx = 0;     // index of first number being sent
        numsPerProc = ceil((float(numbers.size() - 1)) / numprocs);

        // send assigned numbers to every processor
        for (int i = 0; i < numprocs; i++) {
            if (i == procsWithMoreNums - 1)
                numsPerProc--;
            if (i == 0)
                memcpy(myNums, &numbers[sendFromIdx], sizeof(int) * numsPerProc);                
            else
                MPI_Send(&numbers[sendFromIdx], numsPerProc, MPI_INT, i, TAG, MPI_COMM_WORLD);
            sendFromIdx += numsPerProc;
        }
    }
    // receive assigned values
    else
        MPI_Recv(myNums, myCount, MPI_INT, 0, TAG, MPI_COMM_WORLD, &stat);

    // order assigned numbers
    orderSequence(myID, numprocs, myCount, myNums);

    // final result distribution to master
    vector<int> ordered;
    // processor 0 inserts his sequence first
    if (myID == 0)
        ordered.insert(ordered.end(), myNums, myNums + myCount);

    // append a vector of results from other processors
    vector<int> orderedSequence = finalStage(myID, numprocs, myCount, myNums);
    ordered.insert(ordered.end(), orderedSequence.begin(), orderedSequence.end());

    // output ordered sequence
    if (myID == 0)
        printResult(ordered);

    MPI_Finalize();
    delete[] myNums;
    return 0;
}
