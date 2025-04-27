#include <bits/stdc++.h>
#include <pthread.h>
#include <unistd.h>

using namespace std;

pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t controlMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cv1 = PTHREAD_COND_INITIALIZER;
pthread_cond_t cv2 = PTHREAD_COND_INITIALIZER;

bool paused_thread1 = false;
bool thread2_can_run = false;
bool thread1_locked_mutex1 = false;
bool thread1_locked_mutex2 = false;
pthread_t thread2_id;

map<pthread_mutex_t*, pthread_t> mutex_owner;   
map<pthread_t, pthread_mutex_t*> waiting_for;    

pthread_mutex_t graphMutex = PTHREAD_MUTEX_INITIALIZER; 


void lock_mutex(pthread_mutex_t* mtx)
{
    pthread_mutex_lock(&graphMutex);
    waiting_for[pthread_self()] = mtx;
    pthread_mutex_unlock(&graphMutex);

    pthread_mutex_lock(mtx);

    pthread_mutex_lock(&graphMutex);
    mutex_owner[mtx] = pthread_self();
    waiting_for.erase(pthread_self());
    pthread_mutex_unlock(&graphMutex);
}

void unlock_mutex(pthread_mutex_t* mtx)
{
    pthread_mutex_lock(&graphMutex);
    mutex_owner.erase(mtx);
    pthread_mutex_unlock(&graphMutex);

    pthread_mutex_unlock(mtx);
}
void* thread1(void*)
{
    cout << "Thread 1: locking mutex1..." << endl;
    lock_mutex(&mutex1);
    thread1_locked_mutex1 = true;
    cout << "Thread 1: got mutex1" << endl;

    usleep(100 * 1000); 
    cout<<"Thread1 trying to aquire mutex2"<<endl;
    while (true) 
    {
        // 1) Pause handler
        pthread_mutex_lock(&controlMutex);
          if (paused_thread1) {
            cout << "Thread 1: PAUSED — releasing mutex1 and waiting" << endl;
            if (thread1_locked_mutex1) {
                unlock_mutex(&mutex1);
                thread1_locked_mutex1 = false;
            }
            // wait kar rhe hai
            pthread_cond_wait(&cv1, &controlMutex);
            pthread_mutex_unlock(&controlMutex);

            cout << "Thread 1: RESUMED — re-locking mutex1" << endl;
            lock_mutex(&mutex1);
            thread1_locked_mutex1 = true;
            cout << "Thread 1: re-locked mutex1" << endl;
            continue;
          }
        pthread_mutex_unlock(&controlMutex);

        
        if (pthread_mutex_trylock(&mutex2) == 0) {
            pthread_mutex_lock(&graphMutex);
              mutex_owner[&mutex2] = pthread_self();
              waiting_for.erase(pthread_self());
            pthread_mutex_unlock(&graphMutex);

            thread1_locked_mutex2 = true;
            cout << "Thread 1: got mutex2" << endl;
            break;  
            
        }
        
        usleep(50 * 1000);
    }

    cout << "Thread 1: working..." << endl;
    usleep(300 * 1000);
    pthread_mutex_lock(&graphMutex);
      mutex_owner.erase(&mutex2);
    pthread_mutex_unlock(&graphMutex);

    unlock_mutex(&mutex2);
    thread1_locked_mutex2 = false;

    unlock_mutex(&mutex1);
    thread1_locked_mutex1 = false;

    cout << "Thread 1: finished work and released locks." << endl;
    return nullptr;
}


void* thread2(void*)
{
    cout << "Thread 2 trying to lock mutex2..." << endl;
    lock_mutex(&mutex2);
    cout << "Thread 2 locked mutex2" << endl;

    usleep(100 * 1000);

    cout << "Thread 2 trying to lock mutex1..." << endl;
    lock_mutex(&mutex1);
    cout << "Thread 2 locked mutex1" << endl;

    unlock_mutex(&mutex1);
    unlock_mutex(&mutex2);

    cout << "Thread 2 finished work and released locks." << endl;
    return nullptr;
}

void* preemptor(void*)
{
    sleep(5);
    cout << "\n>>> DEADLOCK DETECTED (by preemptor)! PREEMPTING THREAD 1 <<<" << endl;
    pthread_mutex_lock(&controlMutex);
    paused_thread1 = true;  // Flag tells Thread 1 to unlock
    pthread_mutex_unlock(&controlMutex);
    pthread_cond_signal(&cv1); 
    cout << "[Preempt] Signaled thread1 to pause." << endl;
    usleep(300 * 1000);
    if (thread1_locked_mutex1) 
    {
        cout << "[Preempt] Forcibly unlocking mutex1 held by thread1." << endl;
        unlock_mutex(&mutex1);
        thread1_locked_mutex1 = false;
    }
    if (thread1_locked_mutex2) {
        cout << "[Preempt] Forcibly unlocking mutex2 held by thread1." << endl;
        unlock_mutex(&mutex2);
        thread1_locked_mutex2 = false;
    }

    pthread_mutex_lock(&controlMutex);
    thread2_can_run = true;
    pthread_cond_signal(&cv2);
    pthread_mutex_unlock(&controlMutex);
    cout << "[Preempt] Allowed thread2 to proceed." << endl;

    sleep(2);

    pthread_mutex_lock(&controlMutex);
    paused_thread1 = false;
    pthread_cond_signal(&cv1);
    pthread_mutex_unlock(&controlMutex);
    cout << "[Preempt] Resumed thread1." << endl;

    return nullptr;
}

int main()
{
    pthread_t t1, t2, p, d;

    pthread_create(&t1, nullptr, thread1, nullptr);
    
    pthread_create(&t2, nullptr, thread2, nullptr);
    pthread_create(&p, nullptr, preemptor, nullptr);
    pthread_join(t1, nullptr);
    pthread_join(t2, nullptr);
    pthread_cancel(d);
    pthread_join(p, nullptr);

    return 0;
}