#include<iostream>
#include<mutex>
#include<chrono>
#include<thread>
using namespace std;

int n = 10;										// ��������С
int in[3] = { 0,0,0 }, out[3] = { 0,0,0 };      // ����ָ��,����ָ��
int flag[3] = { 0,0,0 };						// ������
int buffer[3][10];								// ������
mutex mtx[3];									// ������������
mutex data_mtx;									// ���������

/**
*   �����ߺ���
*/
void producer(int id)
{
	do {
		for (int i = 0; i < 3; i++)
		{
			if (mtx[i].try_lock())
			{
				if (flag[i]!=n)
				{
					buffer[i][in[i]] = 1;

					data_mtx.lock();
					cout << "������" << id << "�ڵ�" << i + 1 << "��������������" << in[i] << endl;
					cout << "������" << i + 1 << ":";
					for (int j = 0; j < n; j++)
					{
						cout << buffer[i][j] << " ";
					}
					cout << endl << endl;
					data_mtx.unlock();

					in[i] = (in[i] + 1) % n;
					flag[i]++;
					mtx[i].unlock();

					this_thread::sleep_for(chrono::seconds(1));
					break;
				}
				else
				{
					mtx[i].unlock();
				}
			}
		}
	} while (true);
}
/**
*   �����ߺ���
*/
void consumer(int id)
{
	do {
		for (int i = 0; i < 3; i++)
		{
			if (mtx[i].try_lock())
			{
				if (flag[i] != 0)
				{
					buffer[i][out[i]] = 0;

					data_mtx.lock();
					cout << "������" << id << "�ڵ�" << i+1 << "�����������ѣ�" << out[i] << endl;
					cout << "������" << i + 1 << ":";
					for (int j = 0; j < n; j++)
					{
						cout << buffer[i][j] << " ";
					}
					cout << endl << endl;
					data_mtx.unlock();

					out[i] = (out[i] + 1) % n;
					flag[i]--;
					mtx[i].unlock();

					this_thread::sleep_for(chrono::seconds(5));
					break;
				}
				else
				{
					mtx[i].unlock();
				}
			}
		}
	} while (true);
}
int main() {

	thread t1(producer, 1);
	thread t2(consumer, 1);
	thread t3(producer, 2);
	thread t4(consumer, 2);
	thread t5(producer, 3);
	thread t6(consumer, 3);

	t1.join();
	t2.join();
	t3.join();
	t4.join();
	t5.join();
	t6.join();
	return 0;
}