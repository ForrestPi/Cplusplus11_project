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
/* һ�������Ĵ������� */
if ((bytes_read == -1) && (errno != EINTR)) {
	cout<<"Error occur!"<<endl;
	return false;
}
if(size==bytes_read){
	return true;
}
return false;
}

// ���в�  
class TaskQueue  
{  
	public:  
		/* 
		 * ����item�� 
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
		 * ������� 
		 */  
		void append(TaskItem &item)  
		{  
			pthread_mutex_lock(&m_mutex);  
			m_items.push(item);  
			pthread_mutex_unlock(&m_mutex);  
		}  
  
		/* 
		 * �׳�һ������ 
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
		 * ��ȡitem��size�� 
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
		queue<TaskItem> m_items;  // ������С�  
		pthread_mutex_t m_mutex;    // ������еĻ�������  
};  
  
// �첽�����  
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
				 * ����ָ����Ŀ���̡߳� 
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
				 * �����������̡߳� 
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
			 * ������ļ����󣬶�ȡ��Ϸ�������źš� 
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
			 * �߳̽����� 
			 */  
			return NULL;  
		}  
	private:  
		int m_fd;                   // �ļ����  
		TaskQueue *m_queue;         // �������  
		vector<pthread_t> m_tids; // �߳�Id  
		bool m_isStartup;           // true:��������false:δ������  
};  
  
// Ӧ�ò�  
class Reader  
{  
	public:  
		Reader(int fd)  
			: m_fd(fd), m_queue(), m_processor(new AioProcessor(m_fd, &m_queue))  
		{  
			/* 
			 * ����processor�� 
			 */  
			m_processor->startup(2);  
		}  
  
		~Reader()  
		{  
			/* 
			 * ֹͣprocessor�����ͷ������Դ�� 
			 */  
			m_processor->shutdwon();  
			delete m_processor;  
		}  
		void read(const int start_offset, int size, void *buffer)  
		{  
			/* 
			 * ����һ��item�� 
			 */  
			sem_t sem;  
			sem_init(&sem, 0, 0);  
			TaskQueue::TaskItem item;  
			item.start_offset = start_offset;  
			item.size = size;  
			item.buffer = buffer;  
			item.sem = &sem;  
  
			/* 
			 * �������item���뵽��������С� 
			 */  
			m_queue.append(item);  
  
			/* 
			 * �ȴ����ļ�������ɡ� 
			 */  
			sem_wait(&sem);  
			sem_destroy(&sem);  
  
			/* 
			 * ���ļ�������ɡ� 
			 */  
			return;  
  
		}  
	private:  
		int             m_fd;           // �ļ����.  
		TaskQueue       m_queue;        // �������.  
		AioProcessor*   m_processor;    // ��������.  
};  
  
// ����������  
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