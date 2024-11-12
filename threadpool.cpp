#include "threadpool.h"

const int TASK_MAX_THRESHHOLD = 1024;

ThreadPool::ThreadPool()
	:initThreadSize_(4),
	taskSize_(0),
	taskQueMaxThreshHold_(TASK_MAX_THRESHHOLD),
	poolMode_(PoolMode::MODE_FIXED)
{}
// 线程池析构
ThreadPool::~ThreadPool()
{}
// 开启线程池
void ThreadPool::start(int initThreadSize)
{
	// 线程池的数量
	initThreadSize_ = initThreadSize;

	// 创建线程
	for (int i = 0; i < initThreadSize_; ++i)
	{
		// 创建线程类
		auto ptr = std::make_unique<Thread>(std::bind(&ThreadPool::threadFunc, this));
		// unique_ptr 屏蔽左值操作 使用移动语义
		threads_.emplace_back(std::move(ptr));
		

	}
	// 启动所有线程
	for (int i = 0; i < initThreadSize_; ++i)
	{
		// 启动每一个创建的线程
		threads_[i]->start();
	}
}
// 设置线程池的工作模式
void ThreadPool::setMode(PoolMode mode)
{
	poolMode_ = mode;
}
// 设置task任务队列上线阈值
void ThreadPool::setTaskQueMaxThreshHold(int threshhold)
{
	taskQueMaxThreshHold_ = threshhold;
}

// 给线程池提交任务
// 用户调入该接口, 传入生成任务
void ThreadPool::submitTask(std::shared_ptr<Task> sp)
{
	// 获取锁
	std::unique_lock <std::mutex> lock(taskQueMtx_);
	// 线程通信 等在任务队列有空余
	// 用户提交的任务最长不得超过1s, 否则提交任务失败
	notFull_.wait(lock, [&]()->bool {return taskSize_ < taskQueMaxThreshHold_; });
	
	// 如果有空余, 把任务放入任务队列中 
	task_Que_.emplace(sp);
	++taskSize_;
	// 因为有了新任务, 任务队列肯定不空, 在notEmpty_上通知, 分配线程执行任务

	notEmpty_.notify_all();

}
// 定义线程处理函数, 去消费任务队列里的任务
void ThreadPool::threadFunc()
{
	std::cout << "beginID:" << std::this_thread::get_id() << "endID:" 
		<< std::this_thread::get_id() << std::endl;
}

// ----------------------------------
// 线程方法实现
// 线程的构造
Thread::Thread(ThreadFunc func)
	:func_(func)
{}
// 线程的析构
Thread::~Thread()
{}
// 启动线程
void Thread::start()
{
	// 创建一个线程, 执行线程操作
	std::thread t(func_);

	// 设置线程分析
	t.detach();
}