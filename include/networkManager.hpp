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
#ifndef __NetQueue_HPP__
#define __NetQueue_HPP__

#include <pthreads/pthread.h>

#include <memory>
#include <vector>
#include <string>


/* helper class objects */
#include "mqueue.hpp"


/* Inspired by Libcocos */
#ifndef MYPROPERTY
#define MYPROPERTY(varType, varName, funName)                 \
                                                                 \
protected:                                                       \
    varType varName;                                             \
                                                                 \
public:                                                          \
    varType get##funName(void) const { return varName; } \
                                                                 \
public:                                                          \
    void set##funName(varType var) { varName = var; }
#endif /* MYPROPERTY */



class HttpResponse;

/* handles callbacks */
typedef void (*responseCallback)(HttpResponse* resp);



enum class HttpType {
    GET,
    POST
};

/* NOTE: Anything protected or non-public as an 'm_' prefix in it's name */

class HttpRequest {
    /* multidict */
    std::vector<std::string>m_headers;
    MYPROPERTY(std::string, m_postFields , PostFields);
    MYPROPERTY(std::string, m_proxy, Proxy);
    MYPROPERTY(HttpType, m_req, RequestType)
    MYPROPERTY(std::string, m_tag, Tag);
    /* Used for setting custom flags auto stuff such as request info */
    MYPROPERTY(int32_t, m_flag, Flag);
    MYPROPERTY(int32_t, m_timeout, Timeout);
    MYPROPERTY(std::string, m_url, URL);
    MYPROPERTY(responseCallback*, m_onResponse, Callback)

public:    
    HttpRequest(): m_postFields("") , m_proxy("") , m_tag(""), m_timeout(60){
        m_req = HttpType::GET;
        m_onResponse = nullptr;
    }
    
    std::vector<std::string> getHeaders(){return m_headers;}
    void addHeader(const std::string &header){return m_headers.push_back(header);}
    ~HttpRequest(){
        m_headers.clear();
    }
};


class HttpResponse {
    /* a retained reference to the http request we made so we can access tags and debug our requests 
     * soon after leaving the daemon, this unique_ptr never leaves or is moved and acts as a handle/ref */
    std::unique_ptr<HttpRequest, std::default_delete<HttpRequest>> m_request;
public:
    
    /* TODO Maybe a Good Idea to carry the CURLcode to be able to 
    better daignose any problems that a user may bump into */
    bool success;
    int status;
    std::string data;

    /* our libcurl write callback to write our response to `data` */
    static size_t write_callback(void *data, size_t size, size_t nmemb, void *clientp);
    HttpResponse() : data(""), success(false), status(0) {}
    ~HttpResponse(){
        m_request.reset();
    }
    
    int32_t getFlag(){return m_request->getFlag();}
    std::string getTag(){return m_request->getTag();}

    HttpRequest* getRequest(){ return m_request.get();}
    void setRequest(HttpRequest* req){m_request.reset(req);}

};




/* A Boolean with a mutex */
class BoolContainer {
    pthread_mutex_t m_mutex;
    MYPROPERTY(bool, m_value, Value);
public:
    BoolContainer(){
        m_value = false;
        pthread_mutex_init(&m_mutex, nullptr);
    }

    bool operator==(bool value){
        return m_value == value;
    }

    int lock(){
        return pthread_mutex_lock(&m_mutex);
    }

    int unlock(){
        return pthread_mutex_unlock(&m_mutex);
    }

    ~BoolContainer(){
        pthread_mutex_destroy(&m_mutex);
    }
};


class Condition {
    pthread_mutex_t m_mutex;
    pthread_cond_t m_cond;

public:
    Condition(){
        pthread_cond_init(&m_cond, nullptr);
        pthread_mutex_init(&m_mutex, nullptr);
    }

    ~Condition(){
        pthread_mutex_destroy(&m_mutex);
        pthread_cond_destroy(&m_cond);
    }
    
    int lock(){
        return pthread_mutex_lock(&m_mutex);
    }

    int unlock(){
        return pthread_mutex_unlock(&m_mutex);
    }

    int wait(){
        return pthread_cond_wait(&m_cond, &m_mutex);
    }

    int signal(){
        return pthread_cond_signal(&m_cond);
    }

    int broadcast(){
        return pthread_cond_broadcast(&m_cond);
    }
};


/* Used to carry queue data and is the middle-man and parent Object for all http related stuff */
class NetQueue {
    BoolContainer m_close;

    template<class T>
    void drainQueue(mqueue<T> queue){
        /* lock the queues before draining because I don't know who else plans 
         * to use this library */
        queue.lock();
        while (!queue.empty()){
            auto resp = responseQueue.get();
            delete resp;
        }
        queue.unlock();
    }

public:
    BoolContainer threadIsAlive;
    Condition mayclose;
    mqueue<HttpRequest*> requestQueue;
    mqueue<HttpResponse*> responseQueue;
    NetQueue(){};


    /* parent Thread of our thread's life-cycle */
    static void* RaiiThread(void* args);

    /* Releases a daemon to run our Life-cycle object */
    void init();

    /* sends out our http request off to the lauched http daemon. */
    void send(HttpRequest* req);

    bool hasResponse(){return !responseQueue.empty();}

    HttpResponse* getResponse(){return responseQueue.get();};

    bool ShouldCloseDaemon();

    /* Tells the NetQueue Thread to close */
    void CloseDaemon();

    /* shedules to have Daemon close on it's next cycle and drains the queues out */
    void shutdown(bool forceShutDown = false);
    
    /* mainly used to block other threads until NetQueue's MainThread has been closed or shutdown.
    `Wanring` Do not put this on the main Thread or render loop */
    void waitForClose(){mayclose.wait();};
    ~NetQueue();
};


/* used to manage networking Life-Cycles */
class networkManager {
    std::unique_ptr<NetQueue, std::default_delete<NetQueue>>m_nq;
public:
    networkManager(){
        m_nq.reset(new NetQueue);
        m_nq->init();
    };

    /* used to help create an http request to allow the user to conifigure the required http request */
    HttpRequest* newRequest() {return new HttpRequest;}
    
    /* submits the http request back to our memory pool to be sent off */
    void send(HttpRequest* request){
        m_nq->send(request);
    };

    bool hasResponse(){return m_nq->hasResponse();};

    /* returns a nullptr if there's no response avalibe to queue */
    HttpResponse* getResponse(){return m_nq->hasResponse() ? m_nq->getResponse() : nullptr;};

    /* Visits network manager to render on the main-thread */
    void visit();

    /* Gets a global network manager and allocates the object if it doesn't exist */
    static networkManager* sharedState();

    /* deallocates global networkManager */
    static void releaseState();
};



#endif // __NetQueue_HPP__