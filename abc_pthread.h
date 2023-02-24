#pragma once
#include "abc_000_macro.h"
#include <inttypes.h>

extern int get_thread_stacksize(void);
extern int GetNumCores(void);

 
#if defined(WIN64_OS) || defined(WIN32_OS)

//https://stackoverflow.com/questions/13828913/mingw-gcc-not-recognizing-memstatusex
//https://docs.microsoft.com/en-us/windows/win32/winprog/using-the-windows-headers?redirectedfrom=MSDN

//#undef   _WIN32_WINNT
//#define _WIN32_WINNT_WIN7 0x0601


// PROCESSER_NUMBER is defined in winnt.h when __WIN32_WINNT > 0x0601 (windows 7). 
// Here we just assume the host machines are at least Win 7
// #define _WIN32_WINNT 0x502  is found in x86_64-w64-mingw32/include/_mingw.h
// It turns out that there is no way to replace the old value of __WIN32_WINNT bcz
// _mingw.h is inlcuded in winnt.
 
//Update????: in rtool4, the issue with earlier versions of rtoools is resolved. Now we
// can redefine WIN32_WINNT so that the THREAD functions can be defined.

 //#define _WIN32_WINNT     0x0601

 #define WIN32_LEAN_AND_MEAN
 #include <windows.h> 
 //#include <synchapi.h> //SleepConditionVariableCS
 #include <stdlib.h>    //malloc and free

#if defined(TARGET_32)
    #define __in 
    #define _Inout_
    #define _Out_
    #include <synchapi.h> //SleepConditionVariableCS

    extern WINBASEAPI BOOL WINAPI SleepConditionVariableCS(_Inout_ PCONDITION_VARIABLE ConditionVariable, _Inout_ PCRITICAL_SECTION CriticalSection, _In_ DWORD dwMilliseconds);
    extern WINBASEAPI VOID WINAPI WakeConditionVariable(_Inout_ PCONDITION_VARIABLE ConditionVariable);
    extern WINBASEAPI VOID WINAPI InitializeConditionVariable(_Out_ PCONDITION_VARIABLE ConditionVariable);
#endif

#if (_WIN32_WINNT < 0x0601)
    #ifndef ___PROCESSOR_NUMBER_DEFINED
        #define ___PROCESSOR_NUMBER_DEFINED
        typedef struct _PROCESSOR_NUMBER {
            WORD Group;
            BYTE Number;
            BYTE Reserved;
        } PROCESSOR_NUMBER, * PPROCESSOR_NUMBER;
    #endif

    #define ProcThreadAttributeIdealProcessor       5

    #define PROC_THREAD_ATTRIBUTE_NUMBER    0x0000FFFF
    #define PROC_THREAD_ATTRIBUTE_THREAD    0x00010000  // Attribute may be used with thread creation
    #define PROC_THREAD_ATTRIBUTE_INPUT     0x00020000  // Attribute is input only
    #define PROC_THREAD_ATTRIBUTE_ADDITIVE  0x00040000  // Attribute may be "accumulated," e.g. bitmasks, counters, etc.


    #define ProcThreadAttributeValue(Number, Thread, Input, Additive) \
        (((Number) & PROC_THREAD_ATTRIBUTE_NUMBER) | \
         ((Thread != FALSE) ? PROC_THREAD_ATTRIBUTE_THREAD : 0) | \
         ((Input != FALSE) ? PROC_THREAD_ATTRIBUTE_INPUT : 0) | \
         ((Additive != FALSE) ? PROC_THREAD_ATTRIBUTE_ADDITIVE : 0))

    #define PROC_THREAD_ATTRIBUTE_IDEAL_PROCESSOR \
        ProcThreadAttributeValue (ProcThreadAttributeIdealProcessor, TRUE, TRUE, FALSE)

#endif

#ifdef WIN64_OS
    #include "Processthreadsapi.h" //InitializeProcThreadAttributeList DeleteProcThreadAttributeList
#endif

typedef HANDLE        pthread_t;
typedef struct {
                  LPPROC_THREAD_ATTRIBUTE_LIST lpAttributeList;
	              SIZE_T                       dwStackSize;    
}   pthread_attr_t;

typedef CRITICAL_SECTION   pthread_mutex_t;
typedef int                pthread_mutexattr_t;
typedef CONDITION_VARIABLE pthread_cond_t;
typedef int                pthread_condattr_t;

typedef struct {
    #ifdef WIN64_OS
	PROCESSOR_NUMBER ProcNumber;
    #else
    void * ProcNumber;
    #endif
} cpu_set_t;


static INLINE int pthread_attr_init(pthread_attr_t * attr)
{
#ifdef WIN64_OS
    attr->dwStackSize     = 0;
    attr->lpAttributeList = NULL;

    // stackoverflow.com/questions/25472441/pthread-affinity-before-create-threads
    DWORD  attributeCounts = 1L;
    SIZE_T size;
    if (InitializeProcThreadAttributeList(NULL, attributeCounts, 0, &size)
        || GetLastError() == ERROR_INSUFFICIENT_BUFFER)  {
        attr->lpAttributeList =  malloc(size);
        InitializeProcThreadAttributeList(attr->lpAttributeList, attributeCounts, 0, &size);
    }
#endif
    return 0;
}

static INLINE int	pthread_attr_setstacksize(pthread_attr_t* tattr, size_t  size) {
       tattr->dwStackSize = size;
       return 0;
}

// On windows,A sloppy way to get the default stack size: create a thread and run GetCurrentTrheadLimits
extern int pthread_attr_getstacksize_win32(pthread_attr_t* attr, size_t* stacksize);

static int pthread_attr_getstacksize(pthread_attr_t* attr, size_t* stacksize) {
    return pthread_attr_getstacksize_win32(attr, stacksize);
}

static INLINE  int pthread_attr_destroy(pthread_attr_t* attr)
{
#ifdef WIN64_OS
    if (attr->lpAttributeList != NULL) {
        DeleteProcThreadAttributeList(attr->lpAttributeList);
        free(attr->lpAttributeList);
    }    
#endif
    return 0;
}

//https://stackoverflow.com/questions/25472441/pthread-affinity-before-create-threads
static INLINE  int pthread_attr_setaffinity_np ( pthread_attr_t* attr, size_t cpusetsize, const cpu_set_t* cpuset)
{
    #ifdef WIN64_OS
    BOOL fok = UpdateProcThreadAttribute(
                    attr->lpAttributeList,
                    0L,
                    PROC_THREAD_ATTRIBUTE_IDEAL_PROCESSOR,
                    &( ((cpu_set_t *)cpuset)->ProcNumber),
                    sizeof(PROCESSOR_NUMBER), NULL, NULL);
    return fok;
    #else
    //Do notting
    return 0;
    #endif
}
static int pthread_mutex_init(pthread_mutex_t* mutex, const pthread_mutexattr_t* attr) {
	InitializeCriticalSection(mutex); 
    return 0;
}

static INLINE int pthread_mutex_destroy(pthread_mutex_t* mutex) {
	DeleteCriticalSection(mutex);
    return 0;
}
static INLINE int pthread_mutex_lock(pthread_mutex_t* mutex) {
	EnterCriticalSection(mutex);
    return 0;
}
static INLINE int pthread_mutex_unlock(pthread_mutex_t* mutex){
	LeaveCriticalSection(mutex);
    return 0;
}
static INLINE int pthread_cond_init(pthread_cond_t * cond, const pthread_condattr_t * attr) {
	InitializeConditionVariable(cond);
    return 0;
}

static INLINE int pthread_cond_wait(pthread_cond_t * cond, pthread_mutex_t * mutex) {
	SleepConditionVariableCS(cond, mutex, INFINITE);
    return 0;
}
static INLINE int pthread_cond_signal(pthread_cond_t * cond) {
	WakeConditionVariable(cond);
    return 0;
}
static INLINE int pthread_cond_destroy(pthread_cond_t * cond) {
	//https:// stackoverflow.com/questions/28975958/why-does-windows-have-no-deleteconditionvariable-function-to-go-together-with
    return 0;
}
static INLINE void pthread_exit(void *value_ptr)
{
	//https:// stackoverflow.com/questions/11226072/windows-c-closing-thread-with-closehandle
	//Closehandle doesn't destory the thread; it only destory the handle itself and the thread may still run and 
	// we lose the handle to kil or wait on it
    //https:// stackoverflow.com/questions/3959473/must-i-closehandle-on-a-thread-handle
}

#define PTHREAD_CREATE_JOINABLE  1
static INLINE int  pthread_attr_setdetachstate(pthread_attr_t * attr, int detachstate)
{
    return 0;
}
static INLINE int pthread_join(pthread_t thread, void **retvalue_ptr)
{  //Even after the thread exited - its handle is valid. You can for instance query its return value
	WaitForSingleObject(thread, INFINITE);
    if (retvalue_ptr) {
        //https://stackoverflow.com/questions/7100441/how-can-you-get-the-return-value-of-a-windows-thread
        GetExitCodeThread(thread, retvalue_ptr);
    }
	CloseHandle(thread);
    return 0;
}
static INLINE  pthread_t  pthread_self(void) {
    return (pthread_t) GetCurrentThreadId();
}

extern int  GetCPUInfo(void);
extern void PrintCPUInfo(void);

extern void CPU_ZERO(cpu_set_t* cpus);
extern void CPU_SET(int i, cpu_set_t* cpus);

extern int  pthread_create0(pthread_t* tid, const pthread_attr_t* attr, void* (*start) (void*), void* arg);

static int  pthread_create(pthread_t* tid, const pthread_attr_t* attr, void* (*start) (void*), void* arg)
{
    pthread_create0(tid, attr, start, arg);
    return 0;
}

#elif   defined(LINUX_OS)
    
        //you have to define_GNU_SOURCE before anything else
        //https://stackoverflow.com/questions/1407786/how-to-set-cpu-affinity-of-a-particular-pthread
        //https://stackoverflow.com/questions/7296963/gnu-source-and-use-gnu/7297011#7297011
        //https://stackoverflow.com/questions/24034631/error-message-undefined-reference-for-cpu-zero/24034698
	    #ifndef _GNU_SOURCE
		    #define _GNU_SOURCE
	    #endif
        #include <sched.h>  ////cpu_set_t , CPU_SET
	    #include <pthread.h>


#elif   defined(MAC_OS) 

    #include <mach/thread_policy.h> // for thread_port_t etc. //https://github.com/xoreaxeaxeax/sandsifter/issues/3

   //https://lists.apple.com/archives/darwin-kernel/2014/Nov/msg00003.html
   //protoptye for thread_policy_st moved here
    #include <mach/thread_act.h> 
    #include <sys/sysctl.h> // header for systclbyname()
    #include <pthread.h>
   //https://stackoverflow.com/questions/32282270/c99-error-unknown-type-name-pid-t
    #include <sys/types.h> // pid_t
    #include <unistd.h>   // pid_t
    #define SYSCTL_CORE_COUNT   "machdep.cpu.core_count"

    typedef struct cpu_set {    uint32_t    count; } cpu_set_t;

    static inline void   CPU_ZERO(cpu_set_t* cs) {
        cs->count = 0; 
    }

    static inline void    CPU_SET(int num, cpu_set_t* cs) { 
        num = num % GetNumCores(); cs->count |= (1 << num); 
    }

    static inline int     CPU_ISSET(int num, cpu_set_t* cs) {
        num = num % GetNumCores(); return (cs->count & (1 << num)); 
    }

    static int sched_getaffinity(pid_t pid, size_t cpu_size, cpu_set_t* cpu_set)    {
        int32_t core_count = 0;
        size_t  len = sizeof(core_count);
        int ret = sysctlbyname(SYSCTL_CORE_COUNT, &core_count, &len, 0, 0);
        if (ret) {
            //printf("error while get core count %d\n", ret);
            return -1;
        }
        cpu_set->count = 0;
        for (int i = 0; i < core_count; i++) {
            cpu_set->count |= (1 << i);
        }

        return 0;
    }

    static int pthread_setaffinity_np(pthread_t thread, size_t cpu_size,   cpu_set_t *cpu_set) {

            thread_port_t mach_thread;
            int core = 0;

            for (core = 0; core < 8 * cpu_size; core++) {
                  if (CPU_ISSET(core, cpu_set)) break;
            }
            //printf("binding to core %d\n", core);
            thread_affinity_policy_data_t policy = { core };
            mach_thread = pthread_mach_thread_np(thread);
            thread_policy_set(mach_thread, THREAD_AFFINITY_POLICY,
                            (thread_policy_t)&policy, 1);
            return 0;
        }

#elif  defined(SOLARIS_OS)
    //you have to define_GNU_SOURCE before anything else
    //https://stackoverflow.com/questions/1407786/how-to-set-cpu-affinity-of-a-particular-pthread
    //https://stackoverflow.com/questions/7296963/gnu-source-and-use-gnu/7297011#7297011
    //https://stackoverflow.com/questions/24034631/error-message-undefined-reference-for-cpu-zero/24034698
  
    #include <sched.h>  ////cpu_set_t , CPU_SET
    #include <pthread.h>

    #include <sys/types.h> //https://github.com/atom-zju/NUMA-aware-hpc-kernels/blob/e4dee686acf4a3dae49a363edf22ac3966a68d10/mosbench/micro/bench.c
    #include <sys/processor.h>
    #include <sys/procset.h>

    typedef  int cpu_set_t;
    static inline void   CPU_ZERO(cpu_set_t* cs) { *cs = 0; }
    static inline void   CPU_SET(int num, cpu_set_t* cs) { num = num % GetNumCores(); *cs=num; }

    static int sched_getaffinity(pthread_t pid, size_t cpu_size, cpu_set_t* cpu_set) {
       //https://github.com/tpapagian/mosbench/blob/c250c395fab356ab83413db43bf9844cb4f63d4f/micro/bench.c
        processorid_t obind;
        /*
        if (processor_bind(P_LWPID, P_MYID, c, &obind) < 0)
            edie("setaffinity, processor_bind failed");
            */

        // processor_bind(P_LWPID, P_MYID, cpu_id, NULL); //https://github.com/mslovy/barrelfish/blob/780bc02cfbc3594b0ae46ca3a921d183557839be/lib/phoenix/processor.c
        // processor_bind (P_LWPID, P_MYID, PBIND_NONE, NULL); //unbind a thread
        int cpu = *cpu_set;
        processor_bind(P_LWPID, pid, cpu, &obind);
          
        return 0;
        /*  Test whether processor CPU_ID is available.
        //https://github.com/mslovy/barrelfish/blob/780bc02cfbc3594b0ae46ca3a921d183557839be/lib/phoenix/processor.c
            return CPU_ISSET (cpu_id, &cpu_set) ? true : false; //LINUX 
             return (p_online (cpu_id, P_STATUS) == P_ONLINE); //(_SOLARIS_)
        */
    }
#endif

