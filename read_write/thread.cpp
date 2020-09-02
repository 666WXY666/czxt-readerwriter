#include  "windows.h"
#include  <conio.h>
#include  <stdlib.h>
#include  <iostream>
#include  <fstream>
#include  <io.h>
#include  <string.h>
#include  <stdio.h>
using namespace std;

#define  READER  'R'  //����
#define  WRITER  'W' //д��
#define  INTE_PER_SEC  1000 //ÿ��ʱ���ж���Ŀ
#define  MAX_THREAD_NUM  64//����߳���Ŀ
#define  MAX_FILE_NUM  32//��������ļ���Ŀ
#define  MAX_STR_LEN  32//�ַ�������

int  readcount = 0;//������Ŀ
int  writecount = 0;//д����Ŀ
CRITICAL_SECTION  RP_Write;//�ٽ���
CRITICAL_SECTION  cs_Write;
CRITICAL_SECTION  cs_Read;
struct  ThreadInfo//�߳̽ṹ
{
	int  serial;//�߳����
	char  entity;//�߳�����ж��Ƕ����̻߳���д���̣߳�
	double  delay;//�߳��ӳ�
	double  persist;//�̶߳�д��������ʱ��
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//��������-�����߳�
//p:�����߳���Ϣ

void RP_ReaderThread(void* p)
{

	//�������
	HANDLE h_Mutex;
	h_Mutex = OpenMutex(MUTEX_ALL_ACCESS, FALSE, "mutex_for_readcount");

	DWORD wait_for_mutex;//�ȴ������������Ȩ
	DWORD m_delay;//�ӳ�ʱ��
	DWORD m_persist;//���ļ�����ʱ��
	int m_serial;//�߳����
	//�Ӳ����л����Ϣ
	m_serial = ((ThreadInfo*)(p))->serial;
	m_delay = (DWORD)(((ThreadInfo*)(p))->delay * INTE_PER_SEC);
	m_persist = (DWORD)(((ThreadInfo*)(p))->persist * INTE_PER_SEC);

	Sleep(m_delay);//�ӳٵȴ�

	printf("���߽��� %d ����������\n", m_serial);

	//�ȴ������źţ���֤��readcount�ķ��ʡ��޸ĺͻ���
	wait_for_mutex = WaitForSingleObject(h_Mutex, -1);//P����
	//������Ŀ����
	readcount++;
	if (readcount == 1)
	{
		//����һ�����ߣ��ȴ���Դ
		EnterCriticalSection(&RP_Write);
	}
	ReleaseMutex(h_Mutex);//V����
	printf("���߽��� %d ��ʼ���ļ�\n", m_serial);
	
	Sleep(m_persist);

	//�˳��߳�
	printf("���߽��� %d ��ɶ��ļ�\n", m_serial);
	//�ȴ������źţ���֤��readcount�ķ��ʡ��޸Ļ���
	wait_for_mutex = WaitForSingleObject(h_Mutex, -1);//P����
	//������Ŀ����
	readcount--;
	if (readcount == 0)
	{
		//�������ȫ�����꣬����д��
		LeaveCriticalSection(&RP_Write);
	}
	ReleaseMutex(h_Mutex);//V����
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//��������-д���߳�
//д���߳���Ϣ

void RP_WriterThread(void* p)
{
	DWORD m_delay;//�ӳ�ʱ��
	DWORD m_persist;//д�ļ�����ʱ��
	int m_serial;//�߳����
	//�Ӳ����л����Ϣ
	m_serial = ((ThreadInfo*)(p))->serial;
	m_delay = (DWORD)(((ThreadInfo*)(p))->delay * INTE_PER_SEC);
	m_persist = (DWORD)(((ThreadInfo*)(p))->persist * INTE_PER_SEC);
	Sleep(m_delay);//�ӳٵȴ�

	printf("д�߽��� %d ����д����\n", m_serial);
	//�ȴ���Դ
	EnterCriticalSection(&RP_Write);

	//д�ļ�
	printf("д�߽��� %d ��ʼд�ļ�\n", m_serial);
	Sleep(m_persist);

	//�˳��߳�
	printf("д�߽��� %d ���д�ļ�\n", m_serial);
	//�ͷ���Դ
	LeaveCriticalSection(&RP_Write);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//�������ȴ�����
//file:�ļ���

void ReaderPriority(char* file)
{
	DWORD n_thread = 0;//�߳���Ŀ
	DWORD thread_ID;//�߳�ID
	DWORD wait_for_all;//�ȴ������߳̽���

	//�������
	HANDLE h_Mutex;
	h_Mutex = CreateMutex(NULL, FALSE, "mutex_for_readcount");

	//�̶߳��������
	HANDLE h_Thread[MAX_THREAD_NUM];
	ThreadInfo thread_info[MAX_THREAD_NUM];

	readcount = 0;//��ʼ��readcount
	InitializeCriticalSection(&RP_Write);//��ʼ���ٽ���
	ifstream inFile;
	inFile.open(file);//���ļ�
	printf("��������:\n\n");
	while (inFile)
	{
		//����ÿһ�����ߡ�д�ߵ���Ϣ
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
			//�������߽���
			h_Thread[i] = CreateThread(NULL, 0,
				(LPTHREAD_START_ROUTINE)(RP_ReaderThread),
				&thread_info[i],
				0, &thread_ID);
		}
		else 
		{
			//����д�߽���
			h_Thread[i] = CreateThread(NULL, 0,
				(LPTHREAD_START_ROUTINE)(RP_WriterThread),
				&thread_info[i],
				0, &thread_ID);
		}
	}
	//�ȴ������߳̽���
	wait_for_all = WaitForMultipleObjects(n_thread, h_Thread, TRUE, -1);
	printf("���еĶ���д�߾�����ɲ���\n");
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//д������--���߽���
//p�������߳���Ϣ

void WP_ReaderThread(void* p)
{

	//�������
	HANDLE h_mutex1;
	h_mutex1 = OpenMutex(MUTEX_ALL_ACCESS, FALSE, "mutex1");
	HANDLE h_mutex2;
	h_mutex2 = OpenMutex(MUTEX_ALL_ACCESS, FALSE, "mutex2");

	DWORD wait_for_mutex1;//�ȴ������������Ȩ
	DWORD wait_for_mutex2;
	DWORD m_delay;//�ӳ�ʱ��
	DWORD m_persist;//���ļ�����ʱ��
	int m_serial;//�߳����
	//�Ӳ����л����Ϣ
	m_serial = ((ThreadInfo*)(p))->serial;
	m_delay = (DWORD)(((ThreadInfo*)(p))->delay * INTE_PER_SEC);
	m_persist = (DWORD)(((ThreadInfo*)(p))->persist * INTE_PER_SEC);
	Sleep(m_delay);//�ӳٵȴ�

	printf("���߽��� %d ����������\n", m_serial);
	wait_for_mutex1 = WaitForSingleObject(h_mutex1, -1);//P����

	//��������ٽ���
	EnterCriticalSection(&cs_Read);//P����

	//�����������mutex2,��֤��readcount�ķ��ʡ��޸Ļ���
	wait_for_mutex2 = WaitForSingleObject(h_mutex2, -1);//P����
	//�޸Ķ�����Ŀ
	readcount++;
	if (readcount == 1)
	{
		//����ǵ�һ�����ߣ��ȴ�д��д��
		EnterCriticalSection(&cs_Write);
	}
	ReleaseMutex(h_mutex2);//�ͷŻ����ź�mutex2
	//���������߽����ٽ���
	LeaveCriticalSection(&cs_Read);
	ReleaseMutex(h_mutex1);
	//���ļ�
	printf("���߽��� %d ��ʼ���ļ�\n", m_serial);
	Sleep(m_persist);

	//�˳��߳�
	printf("���߽��� %d ��ɶ��ļ�\n", m_serial);
	//�����������mutex2,��֤��readcount�ķ��ʡ��޸Ļ���
	wait_for_mutex2 = WaitForSingleObject(h_mutex2, -1);//P����
	readcount--;
	if (readcount == 0)
	{
		//������ж��߶��꣬����д��
		LeaveCriticalSection(&cs_Write);
	}
	ReleaseMutex(h_mutex2);//V����
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//д������--д���߳�
//p��д���߳���Ϣ

void WP_WriterThread(void* p)
{
	DWORD wait_for_mutex3;
	DWORD m_delay;//�ӳ�ʱ��
	DWORD m_persist;//д�ļ�����ʱ��
	int m_serial;//�߳����

	//�������
	HANDLE h_mutex3;
	h_mutex3 = OpenMutex(MUTEX_ALL_ACCESS, FALSE, "mutex3");

	//�Ӳ����л����Ϣ
	m_serial = ((ThreadInfo*)(p))->serial;
	m_delay = (DWORD)(((ThreadInfo*)(p))->delay * INTE_PER_SEC);
	m_persist = (DWORD)(((ThreadInfo*)(p))->persist * INTE_PER_SEC);
	Sleep(m_delay);//�ӳٵȴ�
	printf("д�߽��� %d ����д����\n", m_serial);

	//�����������mutex3,��֤��writecount�ķ��ʡ��޸Ļ���
	wait_for_mutex3 = WaitForSingleObject(h_mutex3, -1);//P����
	//�޸�д����Ŀ
	writecount++;
	if (writecount == 1)
	{
		//��һ��д�ߣ��ȴ����߶���
		EnterCriticalSection(&cs_Read);
	}
	ReleaseMutex(h_mutex3);//�ͷŻ����ź�mutex3

	//����д���ٽ���
	EnterCriticalSection(&cs_Write);

	//д�ļ�
	printf("д�߽��� %d ��ʼд�ļ�\n", m_serial);
	Sleep(m_persist);

	//�˳��߳�
	printf("д�߽��� %d ���д�ļ�\n", m_serial);
	//�뿪�ٽ��� 
	LeaveCriticalSection(&cs_Write);

	//�����������mutex3,��֤��writecount�ķ��ʡ��޸Ļ���
	wait_for_mutex3 = WaitForSingleObject(h_mutex3, -1);//P����
	writecount--;
	if (writecount == 0)
	{
		//д��д�꣬���߿��Զ�
		LeaveCriticalSection(&cs_Read);
	}
	ReleaseMutex(h_mutex3);//V����
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//д�����ȴ�����
//file:�ļ���

void WriterPriority(char* file)
{
	DWORD n_thread = 0;//�߳���Ŀ
	DWORD thread_ID;//�߳�ID
	DWORD wait_for_all;//�ȴ������߳̽���

	//�������
	HANDLE h_Mutex1;
	h_Mutex1 = CreateMutex(NULL, FALSE, "mutex1");
	HANDLE h_Mutex2;
	h_Mutex2 = CreateMutex(NULL, FALSE, "mutex2");
	HANDLE h_Mutex3;
	h_Mutex3 = CreateMutex(NULL, FALSE, "mutex3");

	//�̶߳���
	HANDLE h_Thread[MAX_THREAD_NUM];
	ThreadInfo thread_info[MAX_THREAD_NUM];

	readcount = 0;//��ʼ��readcount
	writecount = 0;//��ʼ��writecount
	InitializeCriticalSection(&cs_Write);//��ʼ���ٽ���
	InitializeCriticalSection(&cs_Read);
	ifstream inFile;
	inFile.open(file);//���ļ�
	printf("д������:\n\n");
	while (inFile)
	{
		//����ÿһ�����ߡ�д�ߵ���Ϣ
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
			//�������߽���
			h_Thread[i] = CreateThread(NULL, 0,
				(LPTHREAD_START_ROUTINE)(WP_ReaderThread),
				&thread_info[i],
				0, &thread_ID);
		}
		else {
			//����д�߽���
			h_Thread[i] = CreateThread(NULL, 0,
				(LPTHREAD_START_ROUTINE)(WP_WriterThread),
				&thread_info[i],
				0, &thread_ID);
		}
	}
	//�ȴ������߳̽���
	wait_for_all = WaitForMultipleObjects(n_thread, h_Thread, TRUE, -1);
	printf("���еĶ���д�߾�����ɲ���\n");
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//������
int main(int argc, char* argv[])
{
	char ch;
	char str[50] = ".\\thread.dat";//����������ݵľ���·��;
	//C:\\Users\\Administrator\\Desktop\\os\\thread.dat
	while (true)
	{
		//��ӡ��ʾ��Ϣ
		printf("****************************************************\n");
		printf("        1:��������\n");
		printf("        2:д������\n");
		printf("        3:�˳� \n");
		printf("****************************************************\n");
		printf("������(1,2 �� 3):");
		//�����Ϣ����ȷ����������
		do 
		{
			ch = (char)_getch();
		} while (ch != '1' && ch != '2' && ch != '3');

		system("cls");
		//ѡ��3������
		if (ch == '3')
			return 0;
		//ѡ��1����������
		else if (ch == '1')
			ReaderPriority(str);
		//ѡ��2��д������
		else
			WriterPriority(str);
		//����
		printf("\n�����������:");
		_getch();
		system("cls");
	}
	return 0;
}
