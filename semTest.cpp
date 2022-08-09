//
//  main.cpp
//  SemTester
//
//  Created by Ercan Güngör on 9.08.2022.
//

#include <iostream>
#include <semaphore.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

using namespace std;

static int32_t ChildProcessCount = 5;
sem_t *sharedSem = NULL;
pthread_mutex_t logMutex;

struct SignalInfo {
    int32_t SigNo;
    const char* SigName;
};

SignalInfo SignalInfoList[] = {
    {SIGABRT, "SIGABRT"},
    {SIGINT, "SIGINT"},
    {SIGQUIT, "SIGQUIT"},
    {SIGSEGV, "SIGSEGV"},
    {SIGTERM, "SIGTERM"}
};

void LogError(const char* funcName, const char* param, errno_t err) {
    if(param != NULL) {
        std::cout << getpid() << " - " << funcName << "(" <<  param << ") failed. errno: " << err << " (" << strerror(err) << ")" << std::endl;
    } else {
        std::cout <<  getpid() << " - " << funcName << " failed. errno: " << err << " (" << strerror(err) << ")" << std::endl;
    }
}

void SignalHandler(int sigNo, siginfo_t* sigInfo, void* context) {
    std::cout <<  getpid() << " - Signal " << sigNo << " received" << std::endl;
}

int RegisterSignalHandler(int sigNo, const char* sigName, void (*handler)(int, siginfo_t*, void*)) {
    struct sigaction sa;
    
    sigemptyset(&sa.sa_mask);
    sa.sa_sigaction = handler;
    sa.sa_flags = SA_SIGINFO;
    
    if(sigaction(sigNo, &sa, NULL) < 0) {
        LogError("sigaction", sigName, errno);
        return -1;
    }
    
    return 0;
}

int InitMutex() {
    if(pthread_mutex_init(&logMutex, NULL)) {
        LogError("pthread_mutex_init", NULL, errno);
        return -1;
    }
    
    return 0;
}

int InitSem() {
    int retVal = 0;
    
    sharedSem = sem_open("/mySemaphore", O_CREAT, 0660, 1);
    
    if(sharedSem == SEM_FAILED) {
        LogError("sem_open", "O_CREAT", errno);
        retVal = -1;
    }
    
    if(sem_unlink("/mySemaphore") != 0) {
        LogError("sem_unlink", NULL, errno);
    }
    /*sharedSem = sem_open("/mySem", O_CREAT | O_EXCL, 0777, 1);
    
    if(sharedSem == SEM_FAILED) {
        if(errno == EEXIST) {
            sharedSem = sem_open("/mySem", O_RDWR);
            
            if(sharedSem == SEM_FAILED) {
                LogError("sem_open", "O_RDWR", errno);
                retVal = -1;
            }
        } else {
            LogError("sem_open", "O_CREAT", errno);
            retVal = -1;
        }
    }*/
    
    return retVal;
}

int CreateChildProcess() {
    pid_t pid = fork();
    
    if (pid != 0) { // This is parent process
        std::cout << "Child process created. PID: " << pid << std::endl;
    }
    
    return pid;
}

int main(int argc, const char * argv[]) {
    std::cout << "MAIN PID: " << getpid() << std::endl;
    
    if(InitMutex() != 0) {
        return -1;
    }
    
    for (uint32_t i = 0; i < sizeof(SignalInfoList) / sizeof(SignalInfoList[0]); ++i) {
        std::cout <<  "Registering for signal " << SignalInfoList[i].SigName << std::endl;
        RegisterSignalHandler(SignalInfoList[i].SigNo, SignalInfoList[i].SigName, SignalHandler);
    }
    
    for(int32_t i = 0; i < ChildProcessCount; ++i) {
    
        if(CreateChildProcess() == 0) break;
        sleep(1);
    }
    
    if(InitSem() != 0) {
        return -1;
    }
    
    sleep(60);
    
    sem_close(sharedSem);
    
    return 0;
}
