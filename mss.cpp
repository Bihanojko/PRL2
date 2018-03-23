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
MPI_Status stat;            // struct for storing MPI transaction result


// output ordered sequence
void printResult(vector<int> ordered) {
    // cout << cycles << endl;
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

// read numbers from input file and send them to appropriate processors
void distributeInputValues(int myID, int numprocs) {
    int numsPerProc;                            // numbers per processor
    int procsWithMoreNums;                      // number of processors that will have more numbers
    int sendFromIdx = 0;                        // index of first number being sent
    vector<int> numbers;                        // input numbers to order
    fstream fin;                                // input filestream
    fin.open("numbers", ios::in);                   

    // read file
    while (fin.good())
        numbers.push_back(fin.get());
    // output unordered sequence
    printUnorderedSeq(numbers);

    numsPerProc = ceil((float(numbers.size() - 1)) / numprocs);
    procsWithMoreNums = (numbers.size() - 1) % numprocs;  // tolik procs dostane plnej pocet cisel, dalsi o jedna min 
    if (procsWithMoreNums != 0)
        procsWithMoreNums++;

    // send assigned amount of numbers to every processor
    for (int i = 0; i < numprocs; i++) {
        if (i == procsWithMoreNums - 1)
            numsPerProc--;
        sendMyNumbers(i, numsPerProc, &numbers[sendFromIdx]);
        sendFromIdx += numsPerProc;
    }
    fin.close();                                
}

// print input unordered sequence 
void printUnorderedSeq(vector<int> numbers) {
    for (vector<int>::size_type i = 0; i != numbers.size() - 1; i++)
        cout << numbers[i] << " ";
    cout << endl;
}

// send current proccesor's numbers to processor procID
void sendMyNumbers(int procID, int myCount, int *myNums) {
    MPI_Send(&myCount, 1, MPI_INT, procID, TAG, MPI_COMM_WORLD);
    MPI_Send(myNums, myCount, MPI_INT, procID, TAG, MPI_COMM_WORLD);
}

// send current proccesor's numbers to processor neighID and wait for a half ofordered sequence
void waitForOrdered(int neighID, int count, int *myNums) {
    sendMyNumbers(neighID, count, myNums);
    MPI_Recv(myNums, count, MPI_INT, neighID, TAG, MPI_COMM_WORLD, &stat);
}

// receive neighbour's numbers, order neighbour's numbers with yours 
void receiveAndOrder(int neighID, int myCount, int *myNums) {
    vector<int> ordered;

    // receive numbers from neighbour
    int neighCount;             // number of values being returned from neighbour    
    MPI_Recv(&neighCount, 1, MPI_INT, neighID, TAG, MPI_COMM_WORLD, &stat);
    int neighNums[neighCount];
    MPI_Recv(&neighNums, neighCount, MPI_INT, neighID, TAG, MPI_COMM_WORLD, &stat);

    // merge my numbers with neighbour's
    mergeLists(myNums, neighNums, myCount, neighCount);

    // send neighbour second half of ordered numbers
    MPI_Send(&neighNums, neighCount, MPI_INT, neighID, TAG, MPI_COMM_WORLD);
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
    int halfcycles = ceil(numprocs / 2.0);
    int cycles = 0;

    // sort given numbers
    sort(myNums, myNums + myCount);

    // execute sorting algorithm
    for (int j = 1; j <= halfcycles; j++) {
        cycles++;
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
            sendMyNumbers(0, myCount, myNums);

        // receive numbers
        if (myID == 0) {
            MPI_Recv(&neighCount, 1, MPI_INT, i, TAG, MPI_COMM_WORLD, &stat);
            int neighNums[neighCount];
            MPI_Recv(&neighNums, neighCount, MPI_INT, i, TAG, MPI_COMM_WORLD, &stat);
            ordered.insert(ordered.end(), neighNums, neighNums + neighCount);
        }
    }
    return ordered;
}



int main(int argc, char *argv[])
{
    int numprocs;               // number of processors
    int myID;                   // my processor id
    int myCount;                // my count of given numbers to order

    // MPI initialization
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &myID);
 
    if (myID == 0)
        distributeInputValues(myID, numprocs);

    // receive assigned values
    MPI_Recv(&myCount, 1, MPI_INT, 0, TAG, MPI_COMM_WORLD, &stat);
    int myNums[myCount];
    MPI_Recv(&myNums, myCount, MPI_INT, 0, TAG, MPI_COMM_WORLD, &stat);
    // TODO remove after debugging
    // cout << "i am: " << myID << " my numbers are: ";
    // for (int x = 0; x < myCount; x++)
    //     cout << myNums[x] << ", ";
    // cout << endl;

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
    return 0;
}

// TODO remove cycles