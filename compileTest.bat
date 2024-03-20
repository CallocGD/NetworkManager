

@set INCLUDES= /I include /I include/pthreads
@set FILES= src/networkManager.cpp test.cpp
@set LIBS= include/link/libssl.lib include/link/libcurl_a.lib include/link/libcrypto.lib include/link/libpthreadVC3.lib Ws2_32.lib User32.lib Advapi32.lib Crypt32.lib Wldap32.lib Normaliz.lib  
@set FILENAME=test
@set EXTRA=out
CL -DCURL_STATICLIB %FILES% %LIBS% %INCLUDES% /Fe%FILENAME%.exe /Fo%EXTRA%/
