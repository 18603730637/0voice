#include<iostream>
#include<thread>
#include <chrono>
using namespace std;

void callback(void) {
    std::this_thread::sleep_for(std::chrono::seconds(3));
    cout<<"call back\n"<<endl;
}

int main()
{
    thread t1(callback);
    cout<<"main thread\n"<<endl;
    t1.join();
}