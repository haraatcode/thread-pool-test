#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <future>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>



using namespace std;


class Result {
	int result;

public :
	Result(int result) {
		this->result = result;
	}
	void setIntResult(int result) {
		this->result = result;
	}

	int getIntResult() {
		return this->result;
	}
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
				cout << "waiting " << this_thread::get_id() << endl;
				cvQueue.wait(lock, [&]() { return !taskQueue.empty(); });
				cout << "task is executing " << this_thread::get_id() << endl;

				//pick the item from the queue
				task = move(taskQueue.front());
				taskQueue.pop();
			}

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


	future<Result> submitTask(function<Result()> fnc) {

		future<Result> taskFuture;

		packaged_task<Result()> task(fnc);
		taskFuture = task.get_future();

		unique_lock<mutex> lock(mxQueue);
		taskQueue.push(move(task));
		cout << "task is submitted" << endl << endl;
		cvQueue.notify_one(); //notify next waiting thread to be executed 
		return taskFuture;
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
	//generate a  random number
	default_random_engine en;

	uniform_int_distribution<int> dist(1000000000 / 2, 1000000000);

	int upperLimit = dist(en);
	int j = 0;
	for (int i = 0; i < upperLimit; i++) {
		j = i * 1 / 1.9 + 2;
		if ((i % upperLimit / 10) == 0)  cout << ".";
	}

	//throw 42;

	cout << endl;
	Result result = { j };

	return  result;
}


int main() {
	unique_ptr<ThreadPoolExecutor> executor = make_unique<ThreadPoolExecutor>(1);
	vector<future<Result>> futures;

	for (int i = 0; i < 10; i++) {
		future<Result> future = executor->submitTask(testFunction);
		futures.push_back(move(future));
	}

	try {
		int result = futures[0].get().getIntResult();
		cout << "the result is " << result << endl;
	}
	catch (...) {
		cout << "error" << endl;
	}



	executor->deferShutDown();
} 