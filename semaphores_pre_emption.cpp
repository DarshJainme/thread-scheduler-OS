
#include <thread>
#include <chrono>
#include <iostream>
#include <windows.h>
#include <semaphore.h>

sem_t semA, semB;

// Wrap your logic in a function matching LPTHREAD_START_ROUTINE:
DWORD WINAPI Thread1Proc(LPVOID) {
    sem_wait(&semA);
    std::cout << "[T1] got A\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    std::cout << "[T1] waiting on B...\n";
    sem_wait(&semB);
    std::cout << "[T1] got B\n";
    sem_post(&semB);
    sem_post(&semA);
    std::cout << "[T1] done\n";
    return 0;
}

DWORD WINAPI Thread2Proc(LPVOID) {
    sem_wait(&semB);
    std::cout << "[T2] got B\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    std::cout << "[T2] waiting on A...\n";
    sem_wait(&semA);
    std::cout << "[T2] got A\n";
    sem_post(&semA);
    sem_post(&semB);
    std::cout << "[T2] done\n";
    return 0;
}
DWORD WINAPI PreemptorProc(LPVOID param) {
    HANDLE* threads = static_cast<HANDLE*>(param);
    HANDLE h1 = threads[0], h2 = threads[1];

    // 1) give both threads time to deadlock on semaphores
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // 2) suspend T1 so it won't be holding semA anymore
    std::cout << "[Preempt] Suspending T1\n";
    SuspendThread(h1);

    // 3) release semA so T2 can wake up and run to completion
    std::cout << "[Preempt] Posting semA to wake T2\n";
    sem_post(&semA);

    // 4) wait for T2 to finish its critical section
    WaitForSingleObject(h2, INFINITE);

    // 5) now resume T1 — it will wake on semB (which T2 just posted)
    std::cout << "[Preempt] Resuming T1\n";
    ResumeThread(h1);

    return 0;
}

int main() {
    sem_init(&semA, 0, 1);
    sem_init(&semB, 0, 1);

    // Spawn Win32 threads
    HANDLE t1 = CreateThread(nullptr, 0, Thread1Proc, nullptr, 0, nullptr);
    HANDLE t2 = CreateThread(nullptr, 0, Thread2Proc, nullptr, 0, nullptr);

    // Bundle handles for the preemptor
    HANDLE arr[2] = { t1, t2 };
    HANDLE pr = CreateThread(nullptr, 0, PreemptorProc, arr, 0, nullptr);

    // Wait
    WaitForSingleObject(t1, INFINITE);
    WaitForSingleObject(t2, INFINITE);
    WaitForSingleObject(pr, INFINITE);

    // Cleanup
    CloseHandle(t1);
    CloseHandle(t2);
    CloseHandle(pr);
    sem_destroy(&semA);
    sem_destroy(&semB);
    return 0;
}
