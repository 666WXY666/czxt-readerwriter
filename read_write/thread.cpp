#include  "windows.h"
#include  <conio.h>
#include  <stdlib.h>
#include  <iostream>
#include  <fstream>
#include  <io.h>
#include  <string.h>
#include  <stdio.h>
using namespace std;

#define  READER  'R'  //读者
#define  WRITER  'W' //写者
#define  INTE_PER_SEC  1000 //每秒时钟中断数目
#define  MAX_THREAD_NUM  64//最大线程数目
#define  MAX_FILE_NUM  32//最大数据文件数目
#define  MAX_STR_LEN  32//字符串长度

int  readcount = 0;//读者数目
int  writecount = 0;//写者数目
CRITICAL_SECTION  RP_Write;//临界区
CRITICAL_SECTION  cs_Write;
CRITICAL_SECTION  cs_Read;
struct  ThreadInfo//线程结构
{
	int  serial;//线程序号
	char  entity;//线程类别（判断是读者线程还是写者线程）
	double  delay;//线程延迟
	double  persist;//线程读写操作持续时间
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//读者优先-读者线程
//p:读者线程信息

void RP_ReaderThread(void* p)
{

	//互斥变量
	HANDLE h_Mutex;
	h_Mutex = OpenMutex(MUTEX_ALL_ACCESS, FALSE, "mutex_for_readcount");

	DWORD wait_for_mutex;//等待互斥变量所有权
	DWORD m_delay;//延迟时间
	DWORD m_persist;//读文件持续时间
	int m_serial;//线程序号
	//从参数中获得信息
	m_serial = ((ThreadInfo*)(p))->serial;
	m_delay = (DWORD)(((ThreadInfo*)(p))->delay * INTE_PER_SEC);
	m_persist = (DWORD)(((ThreadInfo*)(p))->persist * INTE_PER_SEC);

	Sleep(m_delay);//延迟等待

	printf("读者进程 %d 发出读请求\n", m_serial);

	//等待互斥信号，保证对readcount的访问、修改和互斥
	wait_for_mutex = WaitForSingleObject(h_Mutex, -1);//P操作
	//读者数目增加
	readcount++;
	if (readcount == 1)
	{
		//读第一个读者，等待资源
		EnterCriticalSection(&RP_Write);
	}
	ReleaseMutex(h_Mutex);//V操作
	printf("读者进程 %d 开始读文件\n", m_serial);
	
	Sleep(m_persist);

	//退出线程
	printf("读者进程 %d 完成读文件\n", m_serial);
	//等待互斥信号，保证对readcount的访问、修改互斥
	wait_for_mutex = WaitForSingleObject(h_Mutex, -1);//P操作
	//读者数目减少
	readcount--;
	if (readcount == 0)
	{
		//如果读者全部读完，唤醒写者
		LeaveCriticalSection(&RP_Write);
	}
	ReleaseMutex(h_Mutex);//V操作
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//读者优先-写者线程
//写者线程信息

void RP_WriterThread(void* p)
{
	DWORD m_delay;//延迟时间
	DWORD m_persist;//写文件持续时间
	int m_serial;//线程序号
	//从参数中获得信息
	m_serial = ((ThreadInfo*)(p))->serial;
	m_delay = (DWORD)(((ThreadInfo*)(p))->delay * INTE_PER_SEC);
	m_persist = (DWORD)(((ThreadInfo*)(p))->persist * INTE_PER_SEC);
	Sleep(m_delay);//延迟等待

	printf("写者进程 %d 发出写请求\n", m_serial);
	//等待资源
	EnterCriticalSection(&RP_Write);

	//写文件
	printf("写者进程 %d 开始写文件\n", m_serial);
	Sleep(m_persist);

	//退出线程
	printf("写者进程 %d 完成写文件\n", m_serial);
	//释放资源
	LeaveCriticalSection(&RP_Write);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//读者优先处理函数
//file:文件名

void ReaderPriority(char* file)
{
	DWORD n_thread = 0;//线程数目
	DWORD thread_ID;//线程ID
	DWORD wait_for_all;//等待所有线程结束

	//互斥对象
	HANDLE h_Mutex;
	h_Mutex = CreateMutex(NULL, FALSE, "mutex_for_readcount");

	//线程对象的数组
	HANDLE h_Thread[MAX_THREAD_NUM];
	ThreadInfo thread_info[MAX_THREAD_NUM];

	readcount = 0;//初始化readcount
	InitializeCriticalSection(&RP_Write);//初始化临界区
	ifstream inFile;
	inFile.open(file);//打开文件
	printf("读者优先:\n\n");
	while (inFile)
	{
		//读入每一个读者、写者的信息
		inFile >> thread_info[n_thread].serial;
		inFile >> thread_info[n_thread].entity;
		inFile >> thread_info[n_thread].delay;
		inFile >> thread_info[n_thread++].persist;
		inFile.get();
	}
	for (int i = 0; i < (int)(n_thread); i++)
	{
		if (thread_info[i].entity == READER || thread_info[i].entity == 'r')
		{
			//创建读者进程
			h_Thread[i] = CreateThread(NULL, 0,
				(LPTHREAD_START_ROUTINE)(RP_ReaderThread),
				&thread_info[i],
				0, &thread_ID);
		}
		else 
		{
			//创建写者进程
			h_Thread[i] = CreateThread(NULL, 0,
				(LPTHREAD_START_ROUTINE)(RP_WriterThread),
				&thread_info[i],
				0, &thread_ID);
		}
	}
	//等待所有线程结束
	wait_for_all = WaitForMultipleObjects(n_thread, h_Thread, TRUE, -1);
	printf("所有的读者写者均已完成操作\n");
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//写者优先--读者进程
//p：读者线程信息

void WP_ReaderThread(void* p)
{

	//互斥变量
	HANDLE h_mutex1;
	h_mutex1 = OpenMutex(MUTEX_ALL_ACCESS, FALSE, "mutex1");
	HANDLE h_mutex2;
	h_mutex2 = OpenMutex(MUTEX_ALL_ACCESS, FALSE, "mutex2");

	DWORD wait_for_mutex1;//等待互斥变量所有权
	DWORD wait_for_mutex2;
	DWORD m_delay;//延迟时间
	DWORD m_persist;//读文件持续时间
	int m_serial;//线程序号
	//从参数中获得信息
	m_serial = ((ThreadInfo*)(p))->serial;
	m_delay = (DWORD)(((ThreadInfo*)(p))->delay * INTE_PER_SEC);
	m_persist = (DWORD)(((ThreadInfo*)(p))->persist * INTE_PER_SEC);
	Sleep(m_delay);//延迟等待

	printf("读者进程 %d 发出读请求\n", m_serial);
	wait_for_mutex1 = WaitForSingleObject(h_mutex1, -1);//P操作

	//进入读者临界区
	EnterCriticalSection(&cs_Read);//P操作

	//阻塞互斥对象mutex2,保证对readcount的访问、修改互斥
	wait_for_mutex2 = WaitForSingleObject(h_mutex2, -1);//P操作
	//修改读者数目
	readcount++;
	if (readcount == 1)
	{
		//如果是第一个读者，等待写者写完
		EnterCriticalSection(&cs_Write);
	}
	ReleaseMutex(h_mutex2);//释放互斥信号mutex2
	//让其他读者进入临界区
	LeaveCriticalSection(&cs_Read);
	ReleaseMutex(h_mutex1);
	//读文件
	printf("读者进程 %d 开始读文件\n", m_serial);
	Sleep(m_persist);

	//退出线程
	printf("读者进程 %d 完成读文件\n", m_serial);
	//阻塞互斥对象mutex2,保证对readcount的访问、修改互斥
	wait_for_mutex2 = WaitForSingleObject(h_mutex2, -1);//P操作
	readcount--;
	if (readcount == 0)
	{
		//如果所有读者读完，唤醒写者
		LeaveCriticalSection(&cs_Write);
	}
	ReleaseMutex(h_mutex2);//V操作
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//写者优先--写者线程
//p：写者线程信息

void WP_WriterThread(void* p)
{
	DWORD wait_for_mutex3;
	DWORD m_delay;//延迟时间
	DWORD m_persist;//写文件持续时间
	int m_serial;//线程序号

	//互斥对象
	HANDLE h_mutex3;
	h_mutex3 = OpenMutex(MUTEX_ALL_ACCESS, FALSE, "mutex3");

	//从参数中获得信息
	m_serial = ((ThreadInfo*)(p))->serial;
	m_delay = (DWORD)(((ThreadInfo*)(p))->delay * INTE_PER_SEC);
	m_persist = (DWORD)(((ThreadInfo*)(p))->persist * INTE_PER_SEC);
	Sleep(m_delay);//延迟等待
	printf("写者进程 %d 发出写请求\n", m_serial);

	//阻塞互斥对象mutex3,保证对writecount的访问、修改互斥
	wait_for_mutex3 = WaitForSingleObject(h_mutex3, -1);//P操作
	//修改写者数目
	writecount++;
	if (writecount == 1)
	{
		//第一个写者，等待读者读完
		EnterCriticalSection(&cs_Read);
	}
	ReleaseMutex(h_mutex3);//释放互斥信号mutex3

	//进入写者临界区
	EnterCriticalSection(&cs_Write);

	//写文件
	printf("写者进程 %d 开始写文件\n", m_serial);
	Sleep(m_persist);

	//退出线程
	printf("写者进程 %d 完成写文件\n", m_serial);
	//离开临界区 
	LeaveCriticalSection(&cs_Write);

	//阻塞互斥对象mutex3,保证对writecount的访问、修改互斥
	wait_for_mutex3 = WaitForSingleObject(h_mutex3, -1);//P操作
	writecount--;
	if (writecount == 0)
	{
		//写者写完，读者可以读
		LeaveCriticalSection(&cs_Read);
	}
	ReleaseMutex(h_mutex3);//V操作
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//写者优先处理函数
//file:文件名

void WriterPriority(char* file)
{
	DWORD n_thread = 0;//线程数目
	DWORD thread_ID;//线程ID
	DWORD wait_for_all;//等待所有线程结束

	//互斥对象
	HANDLE h_Mutex1;
	h_Mutex1 = CreateMutex(NULL, FALSE, "mutex1");
	HANDLE h_Mutex2;
	h_Mutex2 = CreateMutex(NULL, FALSE, "mutex2");
	HANDLE h_Mutex3;
	h_Mutex3 = CreateMutex(NULL, FALSE, "mutex3");

	//线程对象
	HANDLE h_Thread[MAX_THREAD_NUM];
	ThreadInfo thread_info[MAX_THREAD_NUM];

	readcount = 0;//初始化readcount
	writecount = 0;//初始化writecount
	InitializeCriticalSection(&cs_Write);//初始化临界区
	InitializeCriticalSection(&cs_Read);
	ifstream inFile;
	inFile.open(file);//打开文件
	printf("写者优先:\n\n");
	while (inFile)
	{
		//读入每一个读者、写者的信息
		inFile >> thread_info[n_thread].serial;
		inFile >> thread_info[n_thread].entity;
		inFile >> thread_info[n_thread].delay;
		inFile >> thread_info[n_thread++].persist;
		inFile.get();
	}
	for (int i = 0; i < (int)(n_thread); i++)
	{
		if (thread_info[i].entity == READER || thread_info[i].entity == 'r')
		{
			//创建读者进程
			h_Thread[i] = CreateThread(NULL, 0,
				(LPTHREAD_START_ROUTINE)(WP_ReaderThread),
				&thread_info[i],
				0, &thread_ID);
		}
		else {
			//创建写者进程
			h_Thread[i] = CreateThread(NULL, 0,
				(LPTHREAD_START_ROUTINE)(WP_WriterThread),
				&thread_info[i],
				0, &thread_ID);
		}
	}
	//等待所有线程结束
	wait_for_all = WaitForMultipleObjects(n_thread, h_Thread, TRUE, -1);
	printf("所有的读者写者均已完成操作\n");
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//主函数
int main(int argc, char* argv[])
{
	char ch;
	char str[50] = ".\\thread.dat";//最好输入数据的绝对路径;
	//C:\\Users\\Administrator\\Desktop\\os\\thread.dat
	while (true)
	{
		//打印提示信息
		printf("****************************************************\n");
		printf("        1:读者优先\n");
		printf("        2:写者优先\n");
		printf("        3:退出 \n");
		printf("****************************************************\n");
		printf("请输入(1,2 或 3):");
		//如果信息不正确，继续输入
		do 
		{
			ch = (char)_getch();
		} while (ch != '1' && ch != '2' && ch != '3');

		system("cls");
		//选择3，返回
		if (ch == '3')
			return 0;
		//选择1，读者优先
		else if (ch == '1')
			ReaderPriority(str);
		//选择2，写者优先
		else
			WriterPriority(str);
		//结束
		printf("\n按任意键结束:");
		_getch();
		system("cls");
	}
	return 0;
}
