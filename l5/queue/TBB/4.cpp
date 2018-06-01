#include <iostream>
#include <thread>
#include <cstdlib>
#include <algorithm>
#include <atomic>
#define NUMBER 10000000
#define THREAD 4
#include "tbb/concurrent_queue.h"
tbb::concurrent_queue< int > queue;
unsigned int seed = time(NULL);
std::vector<std::thread> PUSH(THREAD);
std::vector<std::thread> POP(THREAD);

void pusher() {
	for (int i = 0; i < NUMBER; ++i) {
		auto span = rand_r(&seed) % 200 + 1;
		queue.push(span);
	}
}

void poper() {
	for (int i = 0; i < NUMBER; ++i) {
		int temp;
		queue.try_pop(temp);
	}
}

int main() {
	std::chrono::time_point<std::chrono::high_resolution_clock> start, end;
    	start = std::chrono::high_resolution_clock::now();
	for (auto &thr : PUSH) thr = std::thread(pusher);
	for (auto &thr : PUSH) thr.join();
	end = std::chrono::high_resolution_clock::now();
	std::cout << "PUSH const time: " 
        << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
        << "(ms)."
        << std::endl;

	start = std::chrono::high_resolution_clock::now();	
	for (auto &thr : POP) thr = std::thread(poper);
	for (auto &thr : POP) thr.join();
	end = std::chrono::high_resolution_clock::now();
	std::cout << "POP const time: " 
        << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
        << "(ms)."
        << std::endl;
	std::cout << "FIN" << "\n";
	return 0;
}

