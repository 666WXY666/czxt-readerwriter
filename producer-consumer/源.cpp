#include<iostream>
#include<mutex>
#include<chrono>
#include<thread>
using namespace std;

int n = 10;										// 缓存区大小
int in[3] = { 0,0,0 }, out[3] = { 0,0,0 };      // 生产指针,消费指针
int flag[3] = { 0,0,0 };						// 空与满
int buffer[3][10];								// 缓存区
mutex mtx[3];									// 缓冲区互斥量
mutex data_mtx;									// 输出数据锁

/**
*   生产者函数
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
					cout << "生产者" << id << "在第" << i + 1 << "个缓冲区生产：" << in[i] << endl;
					cout << "缓冲区" << i + 1 << ":";
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
*   消费者函数
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
					cout << "消费者" << id << "在第" << i+1 << "个缓冲区消费：" << out[i] << endl;
					cout << "缓冲区" << i + 1 << ":";
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