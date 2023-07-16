#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <future>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <functional>


using namespace std;


class Result {
};

class ThreadPoolExecutor {
	int maxThreadCount;
	vector<shared_ptr<thread>> threads;
	queue<packaged_task<Result()>> taskQueue;

	mutex mxQueue;
	condition_variable cvQueue;
	atomic_bool shouldExit = false;

	void executeTasks() {
		//check the queue and pick up a task to be executed
		while (!shouldExit) {
			packaged_task<Result()> task;
			{
				unique_lock<mutex> lock(mxQueue);
				cvQueue.wait(lock, [&]() { return !taskQueue.empty(); });
				cout << "task is executing " << this_thread::get_id() << endl;

				//pick the item from the queue
				task = move(taskQueue.front());
				taskQueue.pop();
			}
			//execute the task
			task();
			cout << "execution finished " << this_thread::get_id() << endl;
		}
	}


public:
	ThreadPoolExecutor(int threadCount) {
		maxThreadCount = threadCount;

		if (threadCount == 0)
			maxThreadCount = thread::hardware_concurrency();

		//create initial threas
		for (int i = 0; i < maxThreadCount; i++) {
			shared_ptr<thread> execThread = make_shared<thread>(&ThreadPoolExecutor::executeTasks, this);
			threads.push_back(execThread);
		}

		cout << maxThreadCount << " initial threads created " << endl;
	}


	void submitTask(function<Result()> fnc) {
		packaged_task<Result()> task(fnc);

		unique_lock<mutex> lock(mxQueue);
		taskQueue.push(move(task));
		cvQueue.notify_one(); //notify next waiting thread to be executed 
	}

	void deferShutDown() {
		for (auto t : threads)
			if (t->joinable()) t->join();
	}

	~ThreadPoolExecutor() {
		cout << "pool destructed " << endl;
	}

};

Result testFunction() {
	cout << "=====counting started======" << endl;
	for (int i = 0; i < 1000000000; i++) {
		int j = i * 1 / 1.9 + 2;
		if ((i % 50000000) == 0)  cout << ".";
	}

	cout << endl;
	return  Result();
}


int main() {
	unique_ptr<ThreadPoolExecutor> executor = make_unique<ThreadPoolExecutor>(0);

	for (int i = 0; i < 10; i++)
		executor->submitTask(testFunction);

	executor->deferShutDown();
}