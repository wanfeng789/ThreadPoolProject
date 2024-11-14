#ifndef THREAD_POOL_H
#define THREAD_POOL_H
#include <iostream>
#include <vector>
#include <queue>
#include <functional>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <unordered_map>

// Any类型可以接受任意的类型
class Any
{
public:
	Any() = default;
	~Any() = default;
	Any(const Any&) = delete;
	Any( Any&&) = default;
	Any& operator=(const Any&) = delete;
	Any& operator=( Any&&) = default;
	
	// 这个是让Any类型接受任意类型的数据
	template<typename T>
	Any(T data) : base_(std::make_unique<Derive<T>>(data))
	{}
	// 这个方法能把Any里面存储的数据类型提取出来
	template<typename T>
	T cast_()
	{
		// 找到从base_中指向的Derived对象
		Derive<T>* pd = dynamic_cast<Derive<T>*>(base_.get());
		if (pd == nullptr)
		{
			throw "类型不匹配";
		}
		return pd->data_;
	}
private:
	class Base
	{
	public:
		virtual ~Base() = default;
	private:
	};
	// 派生类类型
	template <typename T>
	class Derive : public Base
	{
	public:
		Derive(T data)
			: data_(data)
		{}
		T data_;	// 保存了任意的类型
	};
private:
	// 定义一个基类的指针指向派生类对象
	std::unique_ptr<Base> base_;

};

// 实现一个信号类
class Semaphore
{
public:
	Semaphore(int limit = 0): resLimit_(limit)
	{}
	~Semaphore() = default;

	// 增加一个信号量
	void post()
	{
		std::unique_lock<std::mutex> lock(mx_);
		++resLimit_;
		// 通知其他线程
		cond_.notify_all();
	}
	// 减少一个信号量
	void wait()
	{
		std::unique_lock<std::mutex> lock(mx_);

		//如果没有信号量资源就阻塞
		cond_.wait(lock, [&]()->bool {return resLimit_ > 0; });

		// 信号量减1
		--resLimit_;

	}
private:
	int resLimit_;	// 资源数
	std::mutex mx_;	// 互斥锁
	std::condition_variable cond_;	// 条件变量
};

// Task类型的前置声明
class Task;

// 实现接受提交到线程池的task任务执行后的返回值类型Result

class Result
{
public:
	Result(std::shared_ptr<Task> task, bool isValid = true);
	~Result() = default;
	// 问题一: setVal方法, 获取任务执行完的返回值
	void setValue(Any any);
	// 问题二: get方法, 用户调用这个方法获取Task返回值
	Any get();

private:
	Any any_;		// 存储任务的返回值类型
	Semaphore sem_;	// 线程通信信号量
	std::shared_ptr<Task> task_;	// 指向对应获取返回值的任务对象
	std::atomic_bool isValid_;		// 返回值是否有效

};

// 任务抽象基类
class Task
{
public:
	Task();
	~Task() = default;
	void exec();
	void setResult(Result* res);
	// 用户可自定义的任意任务类型,从Task继承, 重写run方法
	virtual Any run() = 0;
private:
	Result* result_;	// Result 对象的生命周期 > Task
};


// 线程池支持的模式
enum class PoolMode
{
	MODE_FIXED,		// 固定数量的线程
	MODE_CACHED,	// 动态形式增长的线程
};


// 线程类型
class Thread
{
public:
	// 线程函数类型
	using ThreadFunc = std::function<void(int)>;
	// 线程的构造
	Thread(ThreadFunc func);
	// 线程的析构
	~Thread();
	// 启动线程
	void start();
	//获取线程ID
	int getId()const;
private:
	ThreadFunc func_;
	static int generateId_;	// 生产线程ID
	int threadId_;	// 线程id
};


// 线程池类型
class ThreadPool
{
public:
	// 线程池构造
	ThreadPool();
	// 线程池析构
	~ThreadPool();
	// 开启线程池
	void start(int initThreadSize = 4);
	// 设置线程池的工作模式
	void setMode(PoolMode mode);
	// 设置task任务队列上线阈值
	void setTaskQueMaxThreshHold(int threshhold);
	// 设置task任务队列上线阈值
	void setthreadMaxThreshHold(int threshhold);
	// 给线程池提交任务
	Result submitTask(std::shared_ptr<Task> sp);

	ThreadPool(const ThreadPool& ) = delete;
	ThreadPool& operator=(const ThreadPool&) = delete;
private:
	// 定义线程处理函数
	void threadFunc(int threadid);
	// 检查线程池的运行状态
	bool checkRunningState()const;
private:
	//std::vector<std::unique_ptr<Thread>> threads_;	// 线程列表
	std::unordered_map<int, std::unique_ptr<Thread>> threads_;// 线程列表
	size_t initThreadSize_;// 初始的线程数量
	std::atomic_int curThreadSize_;	// 记录当前线程池里线程的数量
	std::atomic_int idleThreadSize_;	// 空闲线程的数量
	int threadSizeThreshhold_;	// 线程数量的上限阈值
	std::atomic_bool isPoolRunning_; // 表示当前线程池的启动状态
	std::mutex taskQueMtx_;		// 互斥锁保证线程安全
	PoolMode poolMode_;	// 线程池的工作模式


	std::queue<std::shared_ptr<Task>> task_Que_;// 任务队列
	std::atomic_int taskSize_;	// 任务的数量
	int taskQueMaxThreshHold_;	// 任务数量的上限阈值
	std::condition_variable notFull_;	// 任务队列不满
	std::condition_variable notEmpty_;	// 任务队列不空
};

#endif
