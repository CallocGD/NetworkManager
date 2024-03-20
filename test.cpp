#include <iostream>
#include "networkManager.hpp"
#include <chrono>
#include <thread>

#include <cassert>

#define LOG(MSG) std::cout << "[DEBUG] " << MSG << std::endl;

/* Macro Is used to check if System Crashed on a given Log... */
#define BeforeAndAfter(BEFORE, STMT, AFTER) LOG(BEFORE); STMT; LOG(AFTER);  


int main(int argc, char const *argv[])
{

    NetQueue nq;
    /* TODO: add assertions to other functions that forget to use NetQueue::init() */
    /* launch our main thread up */
    BeforeAndAfter("Launching NetQueue Daemon",nq.init(), "Daemon Is up")
    
    LOG("NetQueue::init() succeeded");

    BeforeAndAfter("Allocating To Request", HttpRequest* request = new HttpRequest, "Allocation Succeeded");
    request->addHeader("User-Agent: NetworkManager C++ Ver: 0.0.1 author: Calloc");    
    request->setURL("https://httpbin.org/user-agent");
    request->setTag("Testing");

    BeforeAndAfter("Sending Request", nq.send(request), "Request Sent To the Main Thread");

    while (!nq.hasResponse()){
        std::cout << "Waiting..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }

    auto resp = nq.getResponse();

    LOG("STATUS:" << resp->status);
    LOG("DATA:" << resp->data);
    LOG("TAG:" << resp->getRequest()->getTag());
    /* release the response */
    delete resp;

    LOG("Finished!")
    return 0;
}

