#include "threadpool.h"

const int TASK_MAX_THRESHHOLD = 1024;
const int THREAD_MAX_THRESHHOLD = 10;
const int THREAD_MAX_IDLE_TIME = 60;	// 单位s
ThreadPool::ThreadPool()
	:initThreadSize_(4),
	taskSize_(0),
	isPoolRunning_(false),
	idleThreadSize_(0),
	taskQueMaxThreshHold_(TASK_MAX_THRESHHOLD),
	poolMode_(PoolMode::MODE_FIXED)
	,threadSizeThreshhold_(THREAD_MAX_THRESHHOLD)
	, curThreadSize_(0)
{}
// 线程池析构
ThreadPool::~ThreadPool()
{}
// 开启线程池
void ThreadPool::start(int initThreadSize)
{
	// 设置线程的启动状态
	isPoolRunning_ = true;
	// 线程池的数量
	initThreadSize_ = initThreadSize;
	curThreadSize_ = initThreadSize;
	// 创建线程
	for (int i = 0; i < initThreadSize_; ++i)
	{
		// 创建线程类
		auto ptr = std::make_unique<Thread>(std::bind(&ThreadPool::threadFunc, this, std::placeholders::_1));
		int id = ptr->getId();
		// unique_ptr 屏蔽左值操作 使用移动语义
		threads_.emplace(id, std::move(ptr));
		

	}
	// 启动所有线程
	for (int i = 0; i < initThreadSize_; ++i)
	{
		// 启动每一个创建的线程
		threads_[i]->start();
		++idleThreadSize_;// 空闲线程的数量++
	}
}
// 设置线程池的工作模式
void ThreadPool::setMode(PoolMode mode)
{
	if (checkRunningState())
	{
		return;
	}
	poolMode_ = mode;
}
// 设置task任务队列上线阈值
void ThreadPool::setTaskQueMaxThreshHold(int threshhold)
{
	if (checkRunningState())
	{
		return;
	}
	taskQueMaxThreshHold_ = threshhold;
}
void ThreadPool::setthreadMaxThreshHold(int threshhold)
{
	if (checkRunningState())
	{
		return;
	}
	if (poolMode_ == PoolMode::MODE_CACHED)
	{
		threadSizeThreshhold_ = threshhold;
	}
	
}
// 给线程池提交任务
// 用户调入该接口, 传入生成任务
Result ThreadPool::submitTask(std::shared_ptr<Task> sp)
{
	// 获取锁
	std::unique_lock <std::mutex> lock(taskQueMtx_);
	// 线程通信 等在任务队列有空余
	// 用户提交的任务最长不得超过1s, 否则提交任务失败
	if (!notFull_.wait_for(lock, std::chrono::seconds(1), [&]()->bool {return task_Que_.size() < taskQueMaxThreshHold_; }))
	{
		std::cerr << "task queue is full, submit task is fail" << std::endl;
		// 线程处理完任务之后task就被析构了, 因此先让它别析构
		return Result(sp, false);
	}
	// 如果有空余, 把任务放入任务队列中 
	task_Que_.emplace(sp);
	++taskSize_;
	// 因为有了新任务, 任务队列肯定不空, 在notEmpty_上通知, 分配线程执行任务
	notEmpty_.notify_all();

	// cached模式 任务处理比较急和快和小
	// 需要根据任务的数量和当前线程的数量,判断是否要创建新的线程出来
	if (poolMode_ == PoolMode::MODE_CACHED
		&&taskSize_ > idleThreadSize_
		&& curThreadSize_ < threadSizeThreshhold_)
	{
		// 创建线程类
		auto ptr = std::make_unique<Thread>(std::bind(&ThreadPool::threadFunc, this, std::placeholders::_1));
		int id = ptr->getId();
		// unique_ptr 屏蔽左值操作 使用移动语义
		threads_.emplace(id, std::move(ptr));
		++curThreadSize_;
	}
	 return Result(sp);
}
// 定义线程处理函数, 去消费任务队列里的任务
void ThreadPool::threadFunc(int threadid)
{
	/*std::cout << "beginID:" << std::this_thread::get_id() << "endID:" 
		<< std::this_thread::get_id() << std::endl;*/
	auto lastTime = std::chrono::high_resolution_clock().now();
	while (true)
	{
		std::shared_ptr<Task> task;
		{
			// 先获取锁
			std::unique_lock<std::mutex> lock(taskQueMtx_);

			std::cout << "tid:" << std::this_thread::get_id() << "尝试获取任务" << std::endl;
			
			// cache模式下创建了很多线程, 空闲时间超过了60s, 需要进行回收
			// 回收超过(initThreadSize_)的线程
			if (poolMode_ == PoolMode::MODE_CACHED)
			{
				// 每一秒返回一次 怎么区分超时返回还是执行任务返回
				while (taskSize_ > 0)
				{
					if (std::cv_status::timeout ==
						notEmpty_.wait_for(lock, std::chrono::seconds(1)))
					{
						auto now = std::chrono::high_resolution_clock().now();
						auto dur = std::chrono::duration_cast<std::chrono::seconds>(now - lastTime);
						if (dur.count() >= THREAD_MAX_IDLE_TIME)
						{
							// 开始回收当前线程

							// 记录线程数量的的相关变量删除


							// 把线程对象从容器内删除
							// 线程id 》 删除
						}
					}
				}
			}
			else
			{
				// 等待任务队列中是否有任务可以消费
				notEmpty_.wait(lock, [&]()->bool {return task_Que_.size() > 0; });
			}

			--idleThreadSize_;

			std::cout << "tid:" << std::this_thread::get_id() << "获取任务成功" << std::endl;
			// 从任务队列里去一个任务
			task = task_Que_.front();
			task_Que_.pop();
			--taskSize_;

			// 在取完一个任务后还有剩余的任务,通知其他线程可以执行这些

			if (task_Que_.size() > 0)
			{
				notEmpty_.notify_all();
			}

			// 取出任务进行通知, 可以进行生产任务
			notFull_.notify_all();
		}// 取出任务之后应该释放锁

		if (task != nullptr)
		{
			// 当前线程负责执行这个任务
			//task->run();
			task->exec();
		}
		// 更新线程执行完的时间
		 lastTime = std::chrono::high_resolution_clock().now();
		++idleThreadSize_;
	}

}
// 检查线程池的运行状态
bool ThreadPool::checkRunningState()const
{
	return isPoolRunning_;
}
//////////////////////////////////////
// 线程方法实现
// 线程的构造
Thread::Thread(ThreadFunc func)
	:func_(func)
	,threadId_(generateId_++)
{}
// 线程的析构
Thread::~Thread()
{}
int Thread::generateId_ = 0;
// 启动线程
void Thread::start()
{
	// 创建一个线程, 执行线程操作
	std::thread t(func_, threadId_);

	// 设置线程分析
	t.detach();
}

////////////////////// Task 实现
Task::Task():result_(nullptr)
{}
void Task::exec()
{
	if (result_ != nullptr)
	{
		result_->setValue(run());	// 这里发生多态的调用
	}

}
void Task::setResult(Result* res)
{
	result_ = res;
}
int Thread::getId()const
{
	return threadId_;
}
///////////////////////// Result方法的实现
Result::Result(std::shared_ptr<Task> task, bool isValid)
	:task_(task)
	, isValid_(isValid)
{
	task_->setResult(this);
}
Any Result::get()
{
	if (!isValid_)
	{
		return "";
	}
	sem_.wait();	// task任务如果没有执行完,这里会阻塞用户的线程
	return std::move(any_);
}

void Result::setValue(Any any)
{
	// 存储task的返回值
	this->any_ = std::move(any);
	sem_.post();		// 已经获取的任务的返回值,增加信号量资源

}