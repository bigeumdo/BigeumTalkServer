#include "pch.h"
#include "Service.h"

using namespace std;

int main()
{
	auto service = make_shared<Service>(make_shared<Iocp>(), L"0.0.0.0", 3000);
	ASSERT_CRASH(service->Start());

	vector<thread> threads;
	for (unsigned int i = 0; i < thread::hardware_concurrency(); i++)
	{
		threads.push_back(thread([=]()
		{
			while (true)
			{
				service->GetIocp()->Dispatch();
			}
		}));
	}


	for (auto& t : threads)
	{
		t.join();
	}
}
