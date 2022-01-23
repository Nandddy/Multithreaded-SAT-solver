#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cmath>
#include <omp.h>
#include <mutex>
#include <chrono>
#include <thread>
#include <atomic>

using namespace std;
using namespace std::chrono;
int numVariables = 0;
int numClauses = 0;
int numThreads = 4;
int divWork = 0; //4 thread doesnt work?
bool extraWork = false;
long int long remainingAssumptions = 0;

high_resolution_clock::time_point t1;
high_resolution_clock::time_point t2;
high_resolution_clock::time_point t3;
high_resolution_clock::time_point t4;
atomic<bool> goToSleep(false);
vector<int> inputVector;
vector<int> numStartLocations = { 0 };
mutex door;

void MultiThreaded(bool &keepRunning, std::vector<int> &answerVector, int &remainingAssumptions, bool &noSolution, int &clausePassCount);
void multi(int threadId, std::vector<int> &answerVector, long int long &remainingAssumptions, int &clausePassCount, bool &noSolution, bool &keepRunning);

void SingleThread(std::vector<int> &answerVector, long int long &remainingAssumptions, bool &noSolution);

void PrintSolution(bool noSolution, std::vector<int> &answerVector);

void InputReader() {

	int i = 0;
	bool condition = false;
	ifstream file("Text2.txt");
	string str;
	while (file >> str)
	{
		if (i == 2) {
			//variables
			numVariables = stoi(str);
		}
		if (i == 3) {
			//clauses
			numClauses = stoi(str);
		}
		if (i > 3) {
			if (stoi(str) == 0) {
				numStartLocations.emplace_back(i + 1 - 4);
			}
			inputVector.emplace_back(stoi(str));
		}
		i++;
	}
	numStartLocations.pop_back();
}

vector<int> changeAssumption(vector<int> answerVector, int step) {
	if (answerVector[answerVector.size() - step] == 0) {
		answerVector[answerVector.size() - step] = 1;
	}
	else {
		answerVector[answerVector.size() - step] = 0;
		step++;
		answerVector = changeAssumption(answerVector, step);
	}
	return answerVector;
}


int main()
{
	InputReader();
	if (numThreads > numClauses) {
		numThreads = numClauses;
		///cout << "Hello!" << endl;
	}
	divWork = (int)floor((float)numClauses / numThreads); //..//..//
	extraWork = !(divWork*numThreads == numClauses);
//	if (extraWork) {
//		cout << "There is extra work to do" << endl;
//	}
	vector<int> answerVector(numVariables, 0);
	vector<thread> threads;
	long int long remainingAssumptions = (pow(2, numVariables)) - 1;
	bool keepRunning = false;
	int i = 0;
	int clausePassCount = 0;
	bool noSolution = false;

	t1 = high_resolution_clock::now();
	t3 = high_resolution_clock::now();
	///#pragma omp task //STUDIO2017 DOESNT SUPPORT THIS
//	{
//		while (true) {
//			cout << "Backtracks: " << pow(2, numVariables) - remainingAssumptions << endl;
//			this_thread::sleep_for(chrono::seconds(2));
//		}
//	}

//	cout << "Hello! !@" << endl;
#pragma omp parallel num_threads(numThreads) ///
{
		// call 4 threads, use 4th one for counting back track

		int id = omp_get_thread_num();
		multi(id, answerVector, remainingAssumptions, clausePassCount, noSolution, keepRunning);
		
	

}
	//cout << "finished" << endl;
	t2 = high_resolution_clock::now();
	auto duration = duration_cast<microseconds>(t2 - t1).count();
//	cout << duration<<endl;
	PrintSolution(noSolution, answerVector);

	cout << endl;

	//InputReader();
	//vector<int> answerVector2(numVariables, 0);
	//remainingAssumptions = pow(2, numVariables) - 1;
	//keepRunning = false;
	//i = 0;
	//clausePassCount = 0;
	//noSolution = false;

	//t1 = high_resolution_clock::now();
	//SingleThread(answerVector2, remainingAssumptions,noSolution);
	//t2 = high_resolution_clock::now();
	//duration = duration_cast<microseconds>(t2 - t1).count();
	//cout << duration<<endl;
	//PrintSolution(noSolution, answerVector2);
	return 0;

}

void PrintSolution(bool noSolution, std::vector<int> &answerVector)
{
	cout << "Backtracks: " << pow(2,numVariables) - remainingAssumptions << endl;
	cout << "Solution: \n";
	for (int i = 0; i < numVariables; i++) {
		if (noSolution) {
			cout << "There are no solutions to this problem.";
			break;
		}
		else if (answerVector[i] == 0) {
			cout << "x" << i + 1 << " ";
		}
		else {
			cout << "x" << i + 1 << "_bar ";
		}
	}
}

void SingleThread(std::vector<int> &answerVector, long int long &remainingAssumptions, bool &noSolution)
{
	for (int i = 0; i < inputVector.size(); i++) {
		bool clause = false;
		//checking current clause
		for (i; inputVector[i] != 0; i++) {

			if (inputVector[i] > 0) {
				if (answerVector[inputVector[i] - 1] == 0) { //"x"
					clause = true;
				}
			}
			else if (inputVector[i] < 0) {
				if (answerVector[(-1 * inputVector[i]) - 1] == 1) { //"xbar"
					clause = true;
				}
			}
		}

		if (!clause) { //current clause is false; change assumption and restart
			if (noSolution) { // no more answers to try
			 //using 2 here to indicate that there is no solution
				break;
			}
			remainingAssumptions = remainingAssumptions - 1;
			answerVector = changeAssumption(answerVector, 1);
			i = -1;
		}

	}
}


atomic<bool> restart(false);
atomic<bool> repeat(true);
atomic<bool> globalFalseClause(false);//
atomic<bool> firstClause(true);
atomic<bool> reset(false);
atomic<bool> finished(false);
int totalClauseTried = 0;
int totalClausePassed = 0;
int totalClauseFailed = 0;

void multi(int threadId, std::vector<int> &answerVector, long int long &remainingAssumptions, int &clausePassCount, bool &noSolution, bool &keepRunning) {
	//what happens when single clause?
	//int clauseCount = 0;

	//if lower than x, use x number of threads
	//if num clause is even, using even number of threads
	//if num clause is odd, use odd number of threads

//	if (threadId == numThreads) {

//	}
for (int x = threadId * divWork; (x < (threadId + 1)*divWork); x++) {
			//#pragma omp barrier
#pragma omp master 
			{	
			t4 = high_resolution_clock::now();
			auto duration = abs(duration_cast<seconds>(t4 - t3).count());
				if (abs(duration) > 2) {
					cout << "Backtracks: " << pow(2, numVariables) - remainingAssumptions << endl;
					if (duration > 0) {
						t3 = high_resolution_clock::now();
					}
					else {
						t4 = high_resolution_clock::now();
					}
				}
			}
#pragma omp single
			{
				if (extraWork)
				{
					//cout << "Doing extra work" << endl;
					for (int x = divWork * numThreads; x < numClauses; x++) {
						bool clause = false;
						//checking current clause
						int i = numStartLocations[x];
						for (i; inputVector[i] != 0; i++) {

							if (inputVector[i] > 0) {
								if (answerVector[inputVector[i] - 1] == 0) { //"x"
									clause = true;
								}
							}
							else if (inputVector[i] < 0) {
								if (answerVector[(-1 * inputVector[i]) - 1] == 1) { //"xbar"
									clause = true;
								}
							}
						}

						if (!clause) { //current clause is false; change assumption and restart
							remainingAssumptions = remainingAssumptions - 1;
							if (remainingAssumptions == 0) { // no more answers to try
								noSolution = true;
							}
							else {
							answerVector = changeAssumption(answerVector, 1);
								i = -1;
							}
						}

					}
					//cout << "Answer Vector: ";
					//for (int j = 0; j < answerVector.size() - 1; j++) {
					//	cout << answerVector[j] << " ";
					//}
					//cout << endl;
				}
			}
#pragma omp barrier


			reset = false;
			bool clause = false;
			if (x == numClauses) {
				finished = true;
			}
			int i = numStartLocations[x];

			for (i; inputVector[i] != 0 && i < inputVector.size() - 1; i++) {
				if (inputVector[i] > 0) {
					if (answerVector[inputVector[i] - 1] == 0) { //"x"		
						clause = true;

					}
				}
				else if (inputVector[i] < 0) {
					if (answerVector[(-1 * inputVector[i]) - 1] == 1) { //"xbar"
						clause = true;

					}
				}

			}

#pragma omp barrier //important

			if (!clause && !finished) {//false; ignores case when done
#pragma omp critical
				{
					remainingAssumptions = remainingAssumptions - 1;
					if (remainingAssumptions == 0) {
						noSolution = true;
						finished = true;
					}
					else {
						totalClauseFailed++;
						answerVector = changeAssumption(answerVector, 1);
						//cout << "RESET" << endl;
					}
				}
				reset = true;

			}
#pragma omp barrier
			if (clause) { //true
#pragma omp critical
				{
					totalClausePassed++;
					//cout << "Passed Clauses: " << totalClausePassed << endl;
					if (totalClausePassed == numClauses) { //different for 4 clauses?
						finished = true;
					}
				}
			}
#pragma omp barrier
			//			if (reset == true) {
			//#pragma omp critical
			//				{
			//					totalClauseTried = 0;
			//					totalClauseFailed = 0;
			//					totalClausePassed = 0;
			//					finished = false;
			//				}
			//					x = threadId * divWork - 1;
								//cout << "RESETING x:" <<x<<"thread: "<<threadId<< endl;

			//			}
						//if (finished == true) {
						//	return;
						//}

			if (reset == true) {
#pragma omp critical
				{
					totalClauseTried = 0;
					totalClauseFailed = 0;
					totalClausePassed = 0;
					finished = false;
				}
				x = threadId * divWork - 1;
				//cout << "RESETING x:" <<x<<"thread: "<<threadId<< endl;

			}
			if (finished) {
				return;
			}
#pragma omp barrier
		}
	}

