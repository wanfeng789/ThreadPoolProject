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
// 任务抽象基类
class Task
{
public:
	// 用户可自定义的任意任务类型,从Task继承, 重写run方法
	virtual void run() = 0;
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
	using ThreadFunc = std::function<void()>;
	// 线程的构造
	Thread(ThreadFunc func);
	// 线程的析构
	~Thread();
	// 启动线程
	void start();
private:
	ThreadFunc func_;
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

	// 给线程池提交任务
	void submitTask(std::shared_ptr<Task> sp);

	ThreadPool(const ThreadPool& ) = delete;
	ThreadPool& operator=(const ThreadPool&) = delete;
private:
	// 定义线程处理函数
	void threadFunc();
private:
	std::vector<std::unique_ptr<Thread>> threads_;	// 线程列表
	size_t initThreadSize_;// 初始的线程数量
	
	std::queue<std::shared_ptr<Task>> task_Que_;// 任务队列
	std::atomic_int taskSize_;	// 任务的数量
	int taskQueMaxThreshHold_;	// 任务数量的阈值

	std::mutex taskQueMtx_;		// 互斥锁保证线程安全
	std::condition_variable notFull_;	// 任务队列不满
	std::condition_variable notEmpty_;	// 任务队列不空
	PoolMode poolMode_;	// 线程池的工作模式
};

#endif
