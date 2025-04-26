// File: deadlock_demo.cpp
#include <iostream>
#include <thread>
#include <semaphore.h>
#include <chrono>

// Two binary semaphores, each initialized to 1
sem_t semA, semB;

void thread1() {
    // Step 1: acquire semA
    sem_wait(&semA);
    std::cout << "[Thread 1] acquired semA\n";

    // simulate some work
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    std::cout << "[Thread 1] waiting for semB...\n";
    // Step 2: try to acquire semB (will block if held)
    sem_wait(&semB);
    // (never reaches here)
    std::cout << "[Thread 1] acquired semB\n";

    // release if we ever get here
    sem_post(&semB);
    sem_post(&semA);
}

void thread2() {
    // Step 1: acquire semB
    sem_wait(&semB);
    std::cout << "[Thread 2] acquired semB\n";

    // simulate some work
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    std::cout << "[Thread 2] waiting for semA...\n";
    // Step 2: try to acquire semA (will block)
    sem_wait(&semA);
    // (never reaches here)
    std::cout << "[Thread 2] acquired semA\n";
    sem_post(&semA);
    sem_post(&semB);
}

int main() {
    // initialize both semaphores to 1 (unlocked)
    sem_init(&semA, 0, 1);
    sem_init(&semB, 0, 1);

    std::thread t1(thread1);
    std::thread t2(thread2);

    t1.join();
    t2.join();

    sem_destroy(&semA);
    sem_destroy(&semB);
    return 0;
}

