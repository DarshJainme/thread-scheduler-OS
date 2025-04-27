#include <bits/stdc++.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

using namespace std;

// --- Semaphores in place of mutexes & condition vars ---
sem_t sem1, sem2;                    // in lieu of mutex1, mutex2
sem_t sem_resume1;                   // in lieu of cv1 + controlMutex
atomic<bool> paused_thread1{false};  // flag to tell Thread1 to pause
pthread_t detector_id;

// For deadlock‐detector bookkeeping (unchanged, still uses a real mutex):
pthread_mutex_t graphMutex = PTHREAD_MUTEX_INITIALIZER;
map<pthread_t, pthread_t> wait_for_graph;
map<sem_t*, pthread_t> sem_owner;
map<pthread_t, sem_t*> waiting_for;

// --- Helper wrappers that update the wait‐for graph ---
void lock_sem(sem_t* s) {
    pthread_mutex_lock(&graphMutex);
      waiting_for[pthread_self()] = s;
    pthread_mutex_unlock(&graphMutex);

    sem_wait(s);

    pthread_mutex_lock(&graphMutex);
      sem_owner[s] = pthread_self();
      waiting_for.erase(pthread_self());
    pthread_mutex_unlock(&graphMutex);
}

bool trylock_sem(sem_t* s) {
    if (sem_trywait(s) == 0) {
        pthread_mutex_lock(&graphMutex);
          sem_owner[s] = pthread_self();
          waiting_for.erase(pthread_self());
        pthread_mutex_unlock(&graphMutex);
        return true;
    }
    return false;
}

void unlock_sem(sem_t* s) {
    pthread_mutex_lock(&graphMutex);
      sem_owner.erase(s);
    pthread_mutex_unlock(&graphMutex);

    sem_post(s);
}

// --- Deadlock detector (as before) ---
void* deadlock_detector(void*) {
    while (true) {
        sleep(2);
        pthread_mutex_lock(&graphMutex);
          // build wait‐for graph
          wait_for_graph.clear();
          for (auto &e : waiting_for) {
            if (sem_owner.count(e.second))
              wait_for_graph[e.first] = sem_owner[e.second];
          }
        pthread_mutex_unlock(&graphMutex);

        // detect cycle in wait_for_graph
        set<pthread_t> visited, stack;
        function<bool(pthread_t)> dfs = [&](pthread_t u) {
            visited.insert(u);
            stack.insert(u);
            auto it = wait_for_graph.find(u);
            if (it != wait_for_graph.end()) {
                pthread_t v = it->second;
                if (stack.count(v) || (!visited.count(v) && dfs(v)))
                    return true;
            }
            stack.erase(u);
            return false;
        };

        bool deadlock = false;
        for (auto &p : wait_for_graph) {
            if (!visited.count(p.first) && dfs(p.first)) {
                deadlock = true;
                break;
            }
        }
        if (deadlock)
            cout << "\n>>> DEADLOCK DETECTED by detector!"<<endl;
    }
    return nullptr;
}

// --- Thread 1: grab sem1, then try to grab sem2; allow preemption on sem1 ---
// --- Thread 1: grab sem1, then try to grab sem2; allow preemption on sem1 ---
void* thread1(void*) {
    cout << "Thread1: taking sem1..."<<endl;
    lock_sem(&sem1);
    cout << "Thread1: got sem1"<<endl;

    usleep(100*1000);

    while (true) {
        if (paused_thread1.load()) {
            cout << "Thread1: PAUSED — waiting for resume (sem1 might be released externally!)"<<endl;
            // Just wait till resume, assume sem1 is already released
            sem_wait(&sem_resume1);

            cout << "Thread1: RESUMED — re-taking sem1"<<endl;
            lock_sem(&sem1);
            continue;
        }

        if (trylock_sem(&sem2)) {
            cout << "Thread1: got sem2"<<endl;
            break;
        }

        // back off a bit
        usleep(50*1000);
    }

    cout << "Thread1: working..."<<endl;
    usleep(300*1000);

    unlock_sem(&sem2);
    unlock_sem(&sem1);
    cout << "Thread1: done, released both semaphores"<<endl;
    return nullptr;
}

// --- Preemptor: after a pause, trigger Thread1 to release sem1 externally ---
void* preemptor(void*) {
    sleep(5);
    cout << "\n>>> DEADLOCK DETECTED by preemptor! Preempting Thread1"<<endl;
    paused_thread1.store(true);

    // Give Thread1 a moment to pause
    usleep(300*1000);

    // Now preemptor directly releases sem1 (even though Thread1 was the one that acquired it!)
    pthread_mutex_lock(&graphMutex);
      sem_owner.erase(&sem1); // Important: update ownership bookkeeping
    pthread_mutex_unlock(&graphMutex);

    cout << "[Preemptor] Force releasing sem1!"<<endl;
    sem_post(&sem1);  // <<< THIS shows the "semaphore can be released by other thread" concept

    // Now allow Thread1 to resume
    cout << "[Preemptor] Resuming Thread1"<<endl;
    paused_thread1.store(false);
    sem_post(&sem_resume1);

    return nullptr;
}


// --- Thread 2: grab sem2 then sem1 (classic deadlock risk) ---
void* thread2(void*) {
    cout << "Thread2: taking sem2..."<<endl;
    lock_sem(&sem2);
    cout << "Thread2: got sem2"<<endl;

    usleep(100*1000);

    cout << "Thread2: taking sem1..."<<endl;
    lock_sem(&sem1);
    cout << "Thread2: got sem1"<<endl;

    unlock_sem(&sem1);
    unlock_sem(&sem2);
    cout << "Thread2: done, released both semaphores"<<endl;
    return nullptr;
}


int main() {
    // init semaphores
    sem_init(&sem1, 0, 1);
    sem_init(&sem2, 0, 1);
    sem_init(&sem_resume1, 0, 0);

    pthread_t Thread1, Thread2, p, d;
    pthread_create(&Thread1, nullptr, thread1, nullptr);
    pthread_create(&Thread2, nullptr, thread2, nullptr);
    pthread_create(&p,  nullptr, preemptor, nullptr);
    pthread_create(&d,  nullptr, deadlock_detector, nullptr);

    pthread_join(Thread1, nullptr);
    pthread_join(Thread2, nullptr);

    // clean up
    pthread_cancel(d);
    pthread_join(p, nullptr);
    sem_destroy(&sem1);
    sem_destroy(&sem2);
    sem_destroy(&sem_resume1);

    return 0;
}
