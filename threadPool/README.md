# **半同步半异步I/O的设计模式(half sync/half async)**

##1.动机：
众所周知，同步模式编程简单，但是I/O的利用利率低；而异步模式编程复杂，但是I/O利用率高。
综合同步异步的有优点，就有了半同步半异步的设计模式。

这个模式中，高层使用同步I/O模型，简化编程。低层使用异步I/O模型，高效执行。

half sync/half async可以很好的使得"变成复杂度"和"执行效率"之间达到一种平衡.

##2.应用场景：
半同步半异步模式在下面的场景中使用:
###2.1 一个系统中的进程有下面的特征:
系统必须响应和处理外部异步发生的事件，
如果为每一个外部资源的事件分派一个独立的线程同步处理I/O，效率很低。
如果上层的任务以同步方式处理I/O，实现起来简单。
###2.2 一个或多个任务必须在单独的控制线程中执行，其它任务可以在多线程中执行:
上层的任务（如：数据库查询，文件传输）使用同步I/O模型，简化了编写并行程序的难度。
而底层的任务（如网络控制器的中断处理）使用异步I/O模型，提供了执行效率。

一般情况下，上层的任务要比下层的任务多，使用一个简单的层次实现异步处理的复杂性，可以对外隐藏异步处理的细节。另外，同步层次和异步层次任务间的通信使用一个队列来协调。

##3.实现方案：
可以分为三层：同步任务层，队列层，异步任务层。
###3.1 同步任务层（用户级的进程）:
本层的任务完成上层的I/O操作，使用同步I/O模型，通过队列层的队列中传输数据。和异步层不同，同步层的任务使用活动对象执行，这些活动对象有自己运行栈和寄存器状态。当执行同步I/O的时候，他们会被阻塞/睡眠。
###3.2 队列层:
这个层在同步任务层和异步任务层之间，提供了同步控制和缓存的功能。异步任务的I/O 事件被缓存到消息队列中，同步任务层在队列中提取这些事件（相反方向亦然）
###3.3 异步任务层:

处理低层的事件，这些事件由多个外部的事件源产生（例如网卡，终端）。和异步任务不同，此层的实体是被动对象，没有自己的运行栈，要求不能被阻塞。


##4.优缺点：
###4.1 半同步半异步模式有下面的优点:
上层的任务被简化
不同层可以使用不同的同步策略
层间的通信被限制在单独的一点，因为所有的交互使用队列层协调。
在多处理器环境中提高了性能。
###4.2 半同步半异步模式有下面的缺点:
跨边界导致的性能消耗，这是因为同步控制，数据拷贝和上下文切换会过度地消耗资源。

上层任务缺少异步I/O的实现。

simple 实例仅供参考：

```c++
#include <pthread.h>  // for pthread.  
#include <semaphore.h>    // for sem.  
#include <queue>      // for queue.  
#include <vector>     // for vector.  
#include <string.h>       // for memset.  
  
// for file operation.  
#include <sys/types.h>  
#include <sys/stat.h>  
#include <fcntl.h>  
#include <unistd.h>  
  
// for print log.  
#include <iostream>  
  
using namespace std;  
  
bool pread(int m_fd, void *buffer, int size, int start_offset){
int bytes_read=read(m_fd,buffer,size);
/* 一个致命的错误发生了 */
if ((bytes_read == -1) && (errno != EINTR)) {
	cout<<"Error occur!"<<endl;
	return false;
}
if(size==bytes_read){
	return true;
}
return false;
}

// 队列层  
class TaskQueue  
{  
	public:  
		/* 
		 * 定义item。 
		 */  
		typedef struct _taskItem  
		{  
			int start_offset;  
			int size;  
			void *buffer;  
			sem_t *sem;  
		} TaskItem;  
	public:  
		TaskQueue()  
		{  
			pthread_mutex_init(&m_mutex, NULL);  
		}  
		~TaskQueue()  
		{  
			pthread_mutex_destroy(&m_mutex);  
		}  
  
		/* 
		 * 添加任务。 
		 */  
		void append(TaskItem &item)  
		{  
			pthread_mutex_lock(&m_mutex);  
			m_items.push(item);  
			pthread_mutex_unlock(&m_mutex);  
		}  
  
		/* 
		 * 抛出一个任务； 
		 */  
		bool pop(TaskItem &item)  
		{  
			pthread_mutex_lock(&m_mutex);  
			if(m_items.empty())  
			{  
				pthread_mutex_unlock(&m_mutex);  
				return false;  
			}  
			item.start_offset = m_items.front().start_offset;  
			item.size = m_items.front().size;  
			item.buffer = m_items.front().buffer;  
			item.sem = m_items.front().sem;  
			m_items.pop();  
			pthread_mutex_unlock(&m_mutex);  
  
			return true;  
		}  
  
		/* 
		 * 获取item的size； 
		 */  
		int getSize()  
		{  
			int size = 0;  
			pthread_mutex_lock(&m_mutex);  
			size  = m_items.size();  
			pthread_mutex_unlock(&m_mutex);  
			return m_items.size();  
		}  
  
	private:  
		queue<TaskItem> m_items;  // 任务队列。  
		pthread_mutex_t m_mutex;    // 任务队列的互斥量。  
};  
  
// 异步任务层  
class AioProcessor  
{  
	public:  
		AioProcessor(int fd, TaskQueue *queue)  
		{  
			m_fd = fd;  
			m_queue = queue;  
			m_isStartup = false;  
		}  
		~AioProcessor()  
		{  
			shutdwon();  
		}  
  
		void startup(int thread_count)  
		{  
			if(!m_isStartup)  
			{  
				/* 
				 * 启动指定数目的线程。 
				 */  
				m_isStartup = true;  
				for(int i=0; i<thread_count; ++i)  
				{  
					pthread_t tid;  
					pthread_create(&tid, NULL, process, this);  
					m_tids.push_back(tid);  
				}  
			}  
		}  
		void shutdwon()  
		{  
			if(m_isStartup)  
			{  
				/* 
				 * 结束启动的线程。 
				 */  
				m_isStartup = false;  
				vector<pthread_t>::iterator iter = m_tids.begin();  
				for(; iter<m_tids.end(); ++iter)  
				{  
					pthread_join(*iter, NULL);  
				}  
			}  
		}  
  
	private:  
		static void *process(void *param)  
		{  
			AioProcessor *processor = (AioProcessor *)param;  
  
			/* 
			 * 处理读文件请求，读取完毕发送完毕信号。 
			 */  
			TaskQueue::TaskItem item_to_be_process;  
			while(processor->m_isStartup)  
			{  
				if(processor->m_queue->pop(item_to_be_process))  
				{  
					pread(processor->m_fd, item_to_be_process.buffer, item_to_be_process.size, item_to_be_process.start_offset);  
					sem_post(item_to_be_process.sem);  
				}  
				else  
				{  
					usleep(1000);  
				}  
			}  
  
			/* 
			 * 线程结束。 
			 */  
			return NULL;  
		}  
	private:  
		int m_fd;                   // 文件句柄  
		TaskQueue *m_queue;         // 任务队列  
		vector<pthread_t> m_tids; // 线程Id  
		bool m_isStartup;           // true:已启动；false:未启动。  
};  
  
// 应用层  
class Reader  
{  
	public:  
		Reader(int fd)  
			: m_fd(fd), m_queue(), m_processor(new AioProcessor(m_fd, &m_queue))  
		{  
			/* 
			 * 启动processor。 
			 */  
			m_processor->startup(2);  
		}  
  
		~Reader()  
		{  
			/* 
			 * 停止processor，并释放相关资源。 
			 */  
			m_processor->shutdwon();  
			delete m_processor;  
		}  
		void read(const int start_offset, int size, void *buffer)  
		{  
			/* 
			 * 构造一个item。 
			 */  
			sem_t sem;  
			sem_init(&sem, 0, 0);  
			TaskQueue::TaskItem item;  
			item.start_offset = start_offset;  
			item.size = size;  
			item.buffer = buffer;  
			item.sem = &sem;  
  
			/* 
			 * 将上面的item加入到任务队列中。 
			 */  
			m_queue.append(item);  
  
			/* 
			 * 等待读文件操作完成。 
			 */  
			sem_wait(&sem);  
			sem_destroy(&sem);  
  
			/* 
			 * 读文件操作完成。 
			 */  
			return;  
  
		}  
	private:  
		int             m_fd;           // 文件句柄.  
		TaskQueue       m_queue;        // 任务队列.  
		AioProcessor*   m_processor;    // 任务处理器.  
};  
  
// 测试样例。  
int main()  
{  
	int fd = open("./a.file", O_RDONLY);  
	Reader reader(fd);  
  
	int size = 10;  
	char *buf = new char[size + 1];  
	memset(buf, '\0', size+1);  
	for(int i=0; i<10; i++)  
	{  
		reader.read(i*size, size, buf);  
		cout<<buf<<endl;  
	}  
  
	close(fd);  
	return 0;  
}  
```



