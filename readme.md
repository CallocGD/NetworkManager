# NetworkManager

Inspired by Robtop's works and cocos2dx. This is A Simple C++ 
Module/Library for handling Http Requests Async inside of an 
User Interface that was designed for interacting with the 
boomlings server or any other website on the internet if 
you really wanted to make it do that.

The reason I decided to go ahead and make this manager of mine that I'm using 
in my Imgui Styled Projects Open Source is that I haven't really seen any of 
these used outside of cocos2dx. Making something I could use outside of cocos2dx 
felt rather exciting to me. I had an older version of this code but it failed to work
propperly so I reworte this and came up with a plan for a loop and then designed a new 
queue object to transport the objects I wanted between the main thread and the http 
thread.

The Http Requests get handled inside of 1 thread/deamon However you could theoretically 
setup multiple networkmanagers by not using the global functions provided which are meant 
for only handling one single state.


# Features

- You can use this in QT and cocos2dx if you really wanted to beyond imgui and many other user interfaces that ultilize a main loop.

- It's portable and Doesn't have releativly too many moving parts.

- It has a queue object with built-in mutexes.

- This library sits on top libcurl and openssl and pthreads

- unlike cocos2dx (As Far as I am aware) You now have the ability to foward along Proxies if they are urls such as http , socks4 and socks5 


# TODOS
Feel free to send me Pull requests for any of these if you want to implement them into the library. 
- Was thinking about implementing a way to check curl-errors to diagnose potential issues that 
  a user using your application may come accross.
- make the `gd=1;` cookie optional via macros or make HttpRequest have a set/get Cookie property
- Add more request types like PUT & DELETE which are relatively obscure. 
- CMakeLists.txt (This just got started)
- In the future we could have a custom download handler 
- This tool is currently Windows only However if you want this on other operating systsems send me a pull request. there's quite a few things that you'll have to configure to get to work. luckly there's only 1 C++ file and 2 header files...


# Requirements

- libcurl
- Openssl (Optional but I recommend it when compiling libcurl and this project into your application
  Just know that from my own experience compiling openssl is a pain in the ass)
- pthreads (Luckily there's a windows version of this one)

# Installation On Windows 
- Unzip the `link.zip` file in the include/link folder be sure all the .lib fileds end up in include/link otherwise Cmake might complain at you (I added these because I know how difficult it is to compile these). From the main directory or making a build director you can use cmake to configure and compile everything to `networkmanager.lib` which is meant to be used as a static library 


## Examples 

```c++

#include <iostream>
#include <imgui/imgui.h>
#include <GLFW/glfw3.h>

/* our C++ library */
#include <networkManager.hpp>


void on_callback(HttpResponse* resp){
    std::cout << "HttpStatus: " << resp->status << std::endl;
    std::cout << "DATA: " << resp->data << std::endl;
}


void sendNetworkingTest(){
    /* Obtain our global-state */
    auto NM = networkManager::sharedState();
    HttpRequest* req = NM->newRequest();
    /* Optional Tag but highly recommended for tracking different requests there's also a setFlag option if you're wanting to use enums */
    req->setTag("http-bin-networking-test");
    req->addHeader("User-Agent:myApp ver:0.0.1")
    req->setURL("https://httpbin.org/user-agent");
    req->setFlag(0);
    /* IMPORTANT! Otherwise Callbacks get treated as nothing but an noop (no-operation) function */
    req->setCallback(on_callback);
    /* Tell the networkmanager to send this data to our http thread loop */
    NM->send(req);
}

void shutdownUI(){
    /* Deallocate our Manager */
    networkManager::releaseState();
}

/* Some code and then your Eventloop ... */
void mainloop(){
    while (!glfwWindowShouldClose(m_GLFWindow)){
        /* Handle keyboard calls and events */
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();

        if (ImGui::Begin("MY Window")){
            /* render any lingering http requests on this window */
            if (ImGui::Button("Test My Internet")){
                networkManager->sharedState->visit();
            }
            networkManager->sharedState()->visit();
        }
        ImGui::End();
        /* Render the rest of your ui... */
    shutdownUI();
}
```



