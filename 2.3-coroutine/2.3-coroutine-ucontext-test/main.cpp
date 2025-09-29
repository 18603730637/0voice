#include"coroutine.h"

Schedule schedule;

void yield() {
    schedule.yield();
}

void func1() {
    for(int i=0; i<3; i++) {
        std::cout << "func1 times " << i << std::endl;
        yield();
    }
}

void func2() {
    for(int i=0; i<3; i++) {
        std::cout << "func2 times " << i << std::endl;
        yield();
    }
}

void func3() {
    for(int i=0; i<3; i++) {
        std::cout << "func3 times " << i << std::endl;
        yield();
    }
}

int main()
{
    schedule.CreateCoroutine(std::function(func1));
    schedule.CreateCoroutine(std::function(func2));
    schedule.CreateCoroutine(std::function(func3));

    schedule.run();
}