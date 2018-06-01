#include <iostream>
#include <thread>
#include <cstdlib>
#include <algorithm>
#include <atomic>
#include "hazard.h"
#include "lock_free.h"
#define NUMBER 100000
#define THREAD 4

lock_free_stack<int> stack;
unsigned int seed = time(NULL);
std::vector<std::thread> PUSH(THREAD);
std::vector<std::thread> POP(THREAD);

void pusher() {
	for (int i = 0; i < NUMBER; ++i) {
		auto span = rand_r(&seed) % 200 + 1;
		stack.push(span);
		//std::cout << i << "# PUSH = " << span << "\n";
	}
}

void poper() {
	for (int i = 0; i < NUMBER; ++i) {
		std::shared_ptr<int> temp = stack.pop();
		//std::cout << i << "# POP = " << *temp << "\n";
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
