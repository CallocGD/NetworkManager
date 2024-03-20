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


/* Modifed version of Libcocos's Http Extension 
 * library with a class object to monitor and destory 
 * the http thread loop */

#include <curl/curl.h>
#include <pthreads/pthread.h>
#include <cstdint>
#include <string>
#include <vector>
#include <memory>

#include "networkManager.hpp"

/* MQueue Library */
#include "mqueue.hpp"

/* TODO: Maybe Namespace this library up? */

typedef size_t (*write_callback)(void *data, size_t size, size_t nmemb, void *clientp);

static char error_buffer[256];

/* Inspired by CurlRaii by libcocos and GeometryDash and
 * is a modified version of it designed for use with imgui 
 * and other render engines
 *
 * Modifications Made vs CurlRaii:
 * - added proxy support 
 * - added external options to init() 
 * - added timeout configurations 
 * - designed to mainly communicate and talk to www.boomlings.com 
 *   however you could override my design choices to point it to other 
 *   sites by disabling the cookie gd=1; if nessesary.
 */

class Curl {
public:
    curl_slist* m_headers;
    CURL* m_curl;

    Curl() : m_curl(curl_easy_init()), m_headers(nullptr){}

    /* Same As libcocos's init function but with some newly added features as well as proxy support */
    /* TODO: Configure Threading support and mutexes */
    bool init(
        const std::string &URL,
        std::vector<std::string>headers, 
        write_callback callback, 
        void *stream, 
        int32_t timeout = 60,
        const std::string &proxy = ""
    )
    {
        if (!m_curl) return false;

        if (proxy != ""){
            if (!setOption(CURLOPT_PROXY, proxy.c_str()))
                return false;
        }

        for (size_t i = 0; i < headers.size(); i++ ){
            m_headers = curl_slist_append(m_headers, headers[i].c_str());
            if (m_headers == nullptr) return false;
        }
        if (!setOption(CURLOPT_HTTPHEADER, m_headers))
            return false;

        /* Robtop's server Admin portal says it can take up to 5 minutes to respond so we set the CURLOPT_Timeout to 300 seonds */
        auto code = curl_easy_setopt(m_curl, CURLOPT_TIMEOUT, 300);
        if (code != CURLE_OK) {
            return false;
        }
        code = curl_easy_setopt(m_curl, CURLOPT_CONNECTTIMEOUT, timeout);
        if (code != CURLE_OK) {
            return false;
        }
        curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYHOST, 0L);
        /* NOSIGNAL since we are on a thread afterall... */
        curl_easy_setopt(m_curl, CURLOPT_NOSIGNAL, 1L);
        

        return setOption(CURLOPT_URL, URL.c_str())
                && setOption(CURLOPT_WRITEFUNCTION, callback)
                && setOption(CURLOPT_WRITEDATA, stream);
    }


    template <class T>
    bool setOption(CURLoption option, T arg){
        return CURLE_OK == curl_easy_setopt(m_curl, option, arg);
    }

    // template <class T>
    // CURLcode getInfo(CURLINFO info, T* arg){
    //     return curl_easy_getinfo(m_curl, arg);
    // }

    /* performs an HTTP Request */
    bool perform(int *Status){
        if (curl_easy_perform(m_curl) != CURLE_OK){
            return false;
        }
       
        CURLcode code = curl_easy_getinfo(m_curl, CURLINFO_HTTP_CODE, Status);
        if (code != CURLE_OK || *Status != 200){
            return false;
        }
        return true;
    }

    ~Curl()
    {
        if (m_curl != nullptr)
            curl_easy_cleanup(m_curl);
        
        if (m_headers != nullptr)
            curl_slist_free_all(m_headers);
    }
};


/* TODO Make response have a member for CURLcode and set that to the response to daignose problems */

bool sendPostRequest(HttpRequest* request, HttpResponse* response){
    Curl curl;
    bool ok = curl.init(request->getURL(), request->getHeaders(), HttpResponse::write_callback, reinterpret_cast<void*>(response), request->getTimeout(), request->getProxy())
            && curl.setOption(CURLOPT_COOKIE, "gd=1;")
            && curl.setOption(CURLOPT_POST, 1)
            && curl.setOption(CURLOPT_POSTFIELDSIZE, request->getPostFields().size())
            && curl.setOption(CURLOPT_COPYPOSTFIELDS, request->getPostFields().c_str());
    
    return ok && curl.perform(&response->status);
}

bool sendGetRequest(HttpRequest* request, HttpResponse* response){
    Curl curl;
    bool ok = curl.init(request->getURL(), request->getHeaders(), HttpResponse::write_callback, reinterpret_cast<void*>(response), request->getTimeout(), request->getProxy())
            && curl.setOption(CURLOPT_COOKIE, "gd=1;")
            && curl.setOption(CURLOPT_HTTPGET, 1);
    return ok && curl.perform(&response->status);
}


size_t HttpResponse::write_callback(void *data, size_t size, size_t nmemb, void *clientp){
    size_t realsize = size * nmemb;
    HttpResponse* response = reinterpret_cast<HttpResponse*>(clientp);
    response->data.append(reinterpret_cast<const char*>(data), realsize);
    return realsize;
}



void* NetQueue::RaiiThread(void *args){

    NetQueue* netq = reinterpret_cast<NetQueue*>(args);
    
    while (true){
        /* TODO: add Condition variable to netq->resquestQueue ? */
        /* daemon check */
        if (netq->ShouldCloseDaemon())
            break;
        
        netq->requestQueue.lock();
        if (!netq->requestQueue.empty()){
            HttpRequest* request = netq->requestQueue.get();
            netq->requestQueue.unlock();

            HttpResponse* response = new HttpResponse();
            /* TODO Copy Request off if there's a problem with queues popping values */
            response->setRequest(request);
            
            switch (request->getRequestType()) {
                case HttpType::GET: {
                    response->success = sendGetRequest(response->getRequest(), response);
                    break;
                }

                default: /* HttpType::Post */
                    response->success = sendPostRequest(response->getRequest(), response);
                    break;
            }

            netq->requestQueue.lock();
            netq->requestQueue.pop();
            netq->requestQueue.unlock();

            netq->responseQueue.lock();
            netq->responseQueue.put(response);
            netq->responseQueue.unlock();

        } else {
            netq->requestQueue.unlock();
        }
    }

    /* wake-up the main-thread to drain out all of our queues */
    netq->mayclose.broadcast();
    return nullptr;
}


void NetQueue::init(){
    pthread_t tid;
    pthread_create(&tid, nullptr, NetQueue::RaiiThread, reinterpret_cast<void*>(this));
    /* treat tid as a daemon */
    pthread_detach(tid);
}

void NetQueue::send(HttpRequest* req){
    /* sendoff our http request */
    requestQueue.lock();
    requestQueue.put(req);
    requestQueue.unlock();
}

/* used to signal that we may need to close the Daemon */
bool NetQueue::ShouldCloseDaemon(){
    m_close.lock();
    if (m_close == true){
        m_close.unlock();
        return true;
    }
    m_close.unlock();
    return false;
}

/* signal to have the http thread shutdown */
void NetQueue::CloseDaemon(){
    m_close.lock();
    m_close.setValue(true);
    m_close.unlock();
};

/* shuts down the daemon's lifecycle NOTE: if you set forceShutdown to true you can 
reduce lag but it may cause problems if there's still http requests running 
inside the threadpool still... 
*/
void NetQueue::shutdown(bool forceShutDown){
    CloseDaemon();
    /* Force shutdown blocks the loop Do not attempt to do this unless 
    you know the requests queue is already empty */
    if (!forceShutDown){
        mayclose.lock();
        mayclose.wait();
        mayclose.unlock();
    }
    
    threadIsAlive.setValue(false);
    drainQueue(requestQueue);
    drainQueue(responseQueue);
    
}

NetQueue::~NetQueue(){
    /* see if we're idle before deciding to force the shutdown however 
    it is laggy but safe unless the user has already shutdown the threadpool 
    themselves... */
    if (threadIsAlive == true){
        shutdown(responseQueue.empty() && requestQueue.empty());
    }
    /* I'll leave up to the compiler on how to destory the other object */
}


/* Used to render data and callbacks you setup during your http requests */
void networkManager::visit(){
    /* this is a 1 response per frame styled visitation so that lag doesn't occur as frequently */
    if (hasResponse()){
        /* lock the queue so that other response objects aren't being removed */
        m_nq->responseQueue.lock();
        HttpResponse* resp = getResponse();
        /* do we have a callback to use? */
        if (resp->getRequest()->getCallback() != nullptr){
            responseCallback *cb = resp->getRequest()->getCallback();
            /* callback to our response */
            (*cb)(resp);
        }
        /* pop out this response */
        m_nq->responseQueue.pop();
        m_nq->responseQueue.unlock();
        /* remove the response now... */
        delete resp;
    }
}

static networkManager* GLOBAL_NETWORKMANAGER;

networkManager * networkManager::sharedState(){
    if (GLOBAL_NETWORKMANAGER == nullptr){
        GLOBAL_NETWORKMANAGER = new networkManager;
    }
    return GLOBAL_NETWORKMANAGER;
};


void networkManager::releaseState(){
    if (GLOBAL_NETWORKMANAGER != nullptr){
        delete GLOBAL_NETWORKMANAGER;
    }
}
