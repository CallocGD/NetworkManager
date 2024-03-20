/* 
Copyright 2024 Calloc

Redistribution and use in source and binary forms, with or without 
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, 
    this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, 
    this list of conditions and the following disclaimer in the documentation 
    and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors 
    may be used to endorse or promote products derived from this software without 
    specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS “AS IS” AND ANY 
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES 
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT 
SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, 
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, 
STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT 
OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/



#ifndef __MQUEUE_H__
#define __MQUEUE_H__

#include <queue>
#include <pthreads/pthread.h>


/* Mutex-Queue by Calloc */

/* A Queue with a mutex in it aka. a mutex-queue */
template<typename T>
class mqueue {
    std::queue<T> m_queue;
    pthread_mutex_t m_mutex;
public:

    mqueue(){
        pthread_mutex_init(&m_mutex, nullptr);
    }

    ~mqueue(){
        pthread_mutex_destroy(&m_mutex);
    }
    
    int lock(){
        return pthread_mutex_lock(&m_mutex);
    }

    int unlock(){
        return pthread_mutex_unlock(&m_mutex);
    }

    /* queue is empty */
    bool empty(){
        return m_queue.empty();
    }

    T get(){
        return m_queue.front();
    }

    void pop(){
        return m_queue.pop();
    }

    void put(T _item){
        m_queue.push(_item);
    }
};


/* A Queue that carries a condition variable as well, This is unused but you can optionally use it if you want... */
template <typename T>
class mcqueue : public mqueue<T> {
    pthread_cond_t m_cond;
public:
    mcqueue(){
        pthread_cond_init(&m_cond, nullptr);
    }

    ~mcqueue(){
        pthread_cond_destroy(&m_cond);
    }

    /* waits for a signal from our condition variable */
    int wait(){
        return pthread_cond_wait(&m_cond, &m_mutex);
    }

    int broadcast(){
        return pthread_cond_broadcast(&m_cond);
    }

    /* signals the system */
    int signal(){
        return pthread_cond_signal(&m_cond);
    }
};

#endif // __MQUEUE_H__