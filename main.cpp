#include "threadpool.h"
#include <chrono>


// 如果要得到线程执行的结果
class MyTask:public Task
{
public:
	MyTask(int begin, int end):begin_(begin), end_(end)
	{}
	// 怎么设计,可以返回任意类型
	Any run()
	{
		std::cout << "beginID:" << std::this_thread::get_id() << std::endl;
		//std::this_thread::sleep_for(std::chrono::seconds(2));
		int sum = 0;
		for (int i = begin_; i <= end_; ++i)
		{
			sum += i;
		}
		std::cout << "endID:" << std::this_thread::get_id() << std::endl;
		return sum;
	}
private:
	int begin_;
	int end_;
};

int main()
{
	ThreadPool pool;
	// 用户自己设置线程池的工作模式
	pool.setMode(PoolMode::MODE_CACHED);

	// 开启线程池
	pool.start(4);
	
	// 如何在这里设计Result 类型呢
	Result res1 = pool.submitTask(std::make_shared<MyTask>(1, 100));
	Result res2 = pool.submitTask(std::make_shared<MyTask>(0, 0));
	Result res3 = pool.submitTask(std::make_shared<MyTask>(0, 0));
	// 返回了一个Any类型如何转换成具体的类型呢
	int sum1 = res1.get().cast_<int>();
	int sum2 = res2.get().cast_<int>();
	int sum3 = res3.get().cast_<int>();

	std::cout << "总计数: " << (sum1 + sum2 + sum3) << std::endl;
	//pool.submitTask(std::make_shared<MyTask>());
	//pool.submitTask(std::make_shared<MyTask>());
	//pool.submitTask(std::make_shared<MyTask>());
	//pool.submitTask(std::make_shared<MyTask>());


	//std::this_thread::sleep_for(std::chrono::seconds(6));
	getchar();
	return 0;
}