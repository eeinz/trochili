/*************************************************************************************************
 *                                     Trochili RTOS Kernel                                      *
 *                                  Copyright(C) 2016 LIUXUMING                                  *
 *                                       www.trochili.com                                        *
 *************************************************************************************************/
#include "string.h"

#include "tcl.types.h"
#include "tcl.config.h"
#include "tcl.cpu.h"
#include "tcl.thread.h"
#include "tcl.timer.h"
#include "tcl.debug.h"
#include "tcl.kernel.h"

/* �ں˹ؼ��������� */
TKernelVariable uKernelVariable;

static void CreateRootThread(void);

/*************************************************************************************************
 *  ���ܣ��弶�ַ�����ӡ����                                                                     *
 *  ������(1) pStr ����ӡ���ַ���                                                                *
 *  ���أ���                                                                                     *
 *  ˵����                                                                                       *
 *************************************************************************************************/
void uKernelTrace(const char* pStr)
{
    if (uKernelVariable.TraceEntry != (TTraceEntry)0)
    {
        uKernelVariable.TraceEntry(pStr);
    }
}


/*************************************************************************************************
 *  ���ܣ��弶�ַ�����ӡ����                                                                     *
 *  ������(1) pStr ����ӡ���ַ���                                                                *
 *  ���أ���                                                                                     *
 *  ˵����                                                                                       *
 *************************************************************************************************/
void xKernelTrace(const char* pNote)
{
    TReg32 imask;

    CpuEnterCritical(&imask);
    if (uKernelVariable.TraceEntry != (TTraceEntry)0)
    {
        uKernelVariable.TraceEntry(pNote);
    }
    CpuLeaveCritical(imask);
}


/*************************************************************************************************
 *  ���ܣ��ں˽����жϴ������                                                                   *
 *  ��������                                                                                     *
 *  ���أ���                                                                                     *
 *  ˵����                                                                                       *
 *************************************************************************************************/
void xKernelEnterIntrState(void)
{
    TReg32 imask;
    CpuEnterCritical(&imask);

    uKernelVariable.IntrNestTimes++;
    uKernelVariable.State = eIntrState;

    CpuLeaveCritical(imask);
}


/*************************************************************************************************
 *  ���ܣ��ں��˳��жϴ������                                                                   *
 *  ��������                                                                                     *
 *  ���أ���                                                                                     *
 *  ˵����                                                                                       *
 *************************************************************************************************/
void xKernelLeaveIntrState(void)
{
    TReg32 imask;

    CpuEnterCritical(&imask);

    KNL_ASSERT((uKernelVariable.IntrNestTimes > 0U), "");
    uKernelVariable.IntrNestTimes--;
    if (uKernelVariable.IntrNestTimes == 0U)
    {
        /* 
         * ������������жϱ���ڹ���򼤻˵����ǰ�ж���������ȼ��жϣ���Ȼû�з���Ƕ�ף�
         * ���Ƿ��غ󽫽���ͼ�����жϣ����������������Ҫ���������л�����Ӧ�������һ��������
         * ��ͼ�����Ǹ��ж����˳��ж�ʱ����ɡ�
         * �˴����̵߳������ֵ���"��ռ" 
         */
        if (uKernelVariable.Schedulable == eTrue)
        {
            uThreadSchedule();
        }
        uKernelVariable.State = eThreadState;
    }

    CpuLeaveCritical(imask);
}


/*************************************************************************************************
 *  ���ܣ�ʱ�ӽ����жϴ�����                                                                   *
 *  ��������                                                                                     *
 *  ���أ���                                                                                     *
 *  ˵������ʱ���������������У��������̵߳��ȴ���                                           *
 *************************************************************************************************/
void xKernelTickISR(void)
{
    TReg32 imask;

    CpuEnterCritical(&imask);

    uKernelVariable.Jiffies++;

#if (TCLC_TIMER_ENABLE)
    uTimerTickISR();
#endif

    uThreadTickISR();

    CpuLeaveCritical(imask);
}


/*************************************************************************************************
 *  ���ܣ��ر�������ȹ���                                                                       *
 *  ��������                                                                                     *
 *  ���أ���                                                                                     *
 *  ˵����������ֻ�ܱ��̵߳���                                                                   *
 *************************************************************************************************/
TState xKernelLockSched(void)
{
    TState state = eFailure;
    TReg32 imask;

    CpuEnterCritical(&imask);
    if (uKernelVariable.State == eThreadState)
    {
        uKernelVariable.Schedulable = eFalse;
        state = eSuccess;
    }
    CpuLeaveCritical(imask);
    return state;
}


/*************************************************************************************************
 *  ���ܣ�����������ȹ���                                                                       *
 *  ��������                                                                                     *
 *  ���أ���                                                                                     *
 *  ˵����������ֻ�ܱ��̵߳���                                                                   *
 *************************************************************************************************/
TState xKernelUnlockSched(void)
{
    TState state = eFailure;
    TReg32 imask;

    CpuEnterCritical(&imask);
    if (uKernelVariable.State == eThreadState)
    {
        uKernelVariable.Schedulable = eTrue;
        state = eSuccess;
    }
    CpuLeaveCritical(imask);
    return state;
}


/*************************************************************************************************
 *  ���ܣ�����ϵͳIdle������IDLE�̵߳���                                                         *
 *  ������(1) pEntry ϵͳIdle����                                                                *
 *  ���أ���                                                                                     *
 *  ˵����                                                                                       *
 *************************************************************************************************/
void xKernelSetIdleEntry(TSysIdleEntry pEntry)
{
    TReg32 imask;

    CpuEnterCritical(&imask);
    uKernelVariable.SysIdleEntry = pEntry;
    CpuLeaveCritical(imask);
}


/*************************************************************************************************
 *  ���ܣ����ϵͳ��ǰ�߳�ָ��                                                                   *
 *  ������(1) pThread2 ���ص�ǰ�߳�ָ��                                                          *
 *  ���أ���                                                                                     *
 *  ˵����                                                                                       *
 *************************************************************************************************/
void xKernelGetCurrentThread(TThread** pThread2)
{
    TReg32 imask;

    CpuEnterCritical(&imask);
    *pThread2 = uKernelVariable.CurrentThread;
    CpuLeaveCritical(imask);
}


/*************************************************************************************************
 *  ���ܣ����ϵͳ������ʱ�ӽ�����                                                               *
 *  ������(1) pJiffies ����ϵͳ������ʱ�ӽ�����                                                  *
 *  ���أ���                                                                                     *
 *  ˵����                                                                                       *
 *************************************************************************************************/
void xKernelGetJiffies(TTimeTick* pJiffies)
{
    TReg32 imask;

    CpuEnterCritical(&imask);
    *pJiffies = uKernelVariable.Jiffies;
    CpuLeaveCritical(imask);
}


/*************************************************************************************************
 *  ���ܣ��ں���������                                                                           *
 *  ������(1) pUserEntry  Ӧ�ó�ʼ������                                                         *
 *        (2) pCpuEntry   ��������ʼ������                                                       *
 *        (3) pBoardEntry �弶�豸��ʼ������                                                     *
 *        (4) pTraceEntry �����������                                                           *
 *  ���أ���                                                                                     *
 *  ˵����                                                                                       *
 *************************************************************************************************/
void xKernelStart(TUserEntry pUserEntry,
                  TCpuSetupEntry pCpuEntry,
                  TBoardSetupEntry pBoardEntry,
                  TTraceEntry pTraceEntry)
{
    /* �رմ������ж� */
    CpuDisableInt();

    /* ��ʼ�������ں˲��� */
    memset(&uKernelVariable, 0, sizeof(uKernelVariable));
    uKernelVariable.UserEntry       = pUserEntry;
    uKernelVariable.CpuSetupEntry   = pCpuEntry;
    uKernelVariable.BoardSetupEntry = pBoardEntry;
    uKernelVariable.TraceEntry      = pTraceEntry;
    uKernelVariable.Schedulable     = eTrue;

    /* ��ʼ�������ں�ģ�� */
    uThreadModuleInit();                    /* ��ʼ���̹߳���ģ��           */
#if (TCLC_TIMER_ENABLE)
    uTimerModuleInit();                     /* ��ʼ����ʱ��ģ��             */
#endif
#if (TCLC_IRQ_ENABLE)
    uIrqModuleInit();                       /* ��ʼ���жϹ���ģ��           */
#endif

    CreateRootThread();                     /* ��ʼ���ں�ROOT�̲߳��Ҽ���   */
#if ((TCLC_TIMER_ENABLE) && (TCLC_TIMER_DAEMON_ENABLE))
    uTimerCreateDaemon();                   /* ��ʼ���ں˶�ʱ���̲߳��Ҽ��� */
#endif
#if ((TCLC_IRQ_ENABLE) && (TCLC_IRQ_DAEMON_ENABLE))
    uIrqCreateDaemon();                     /* ��ʼ���ں�IRQ�̲߳��Ҽ���    */
#endif

    uKernelVariable.CpuSetupEntry();        /* ���ô������Ͱ弶��ʼ������   */
    uKernelVariable.BoardSetupEntry();      /* ���ô������Ͱ弶��ʼ������   */

    CpuLoadRootThread();                    /* �����ں�ROOT�߳�             */

    /* �򿪴������ж� */
    CpuEnableInt();

    /* 
     * ���δ���Ӧ����Զ���ᱻִ�У������е��ˣ�˵����ֲʱ�������⡣
     * �����ѭ�������𵽶������ã����⴦�������������״̬
     */
    while (eTrue)
    {
        uDebugPanic("", __FILE__, __FUNCTION__, __LINE__);
    }
}


/* �ں�ROOT�̶߳����ջ���� */
static TThread RootThread;
static TBase32 RootThreadStack[TCLC_ROOT_THREAD_STACK_BYTES >> 2];

/* �ں�ROOT�̲߳������κ��̹߳���API���� */
#define THREAD_ACAPI_ROOT (THREAD_ACAPI_NONE)

/*************************************************************************************************
 *  ���ܣ��ں�ROOT�̺߳���                                                                       *
 *  ������(1) argument �̵߳Ĳ���                                                                *
 *  ���أ���                                                                                     *
 *  ˵�����ú������ȿ�����������ƣ�Ȼ����������߳�����                                         *
 *        ע���߳�ջ������С�����⣬����̺߳�����Ҫ��̫�๤��                                   *
 *************************************************************************************************/
static void xRootThreadEntry(TBase32 argument)
{
    /* �رմ������ж� */
    CpuDisableInt();

    /* ����ں˽�����߳�ģʽ */
    uKernelVariable.State = eThreadState;

    /* ��ʱ�ر��̵߳��ȹ��� */
    uKernelVariable.Schedulable = eFalse;

    /* 
     * �����û���ں�������ʼ���û�����
     * �ú���������eThreadState,���ǽ�ֹSchedulable��״̬��
     */
    if(uKernelVariable.UserEntry == (TUserEntry)0)
    {
        uDebugPanic("", __FILE__, __FUNCTION__, __LINE__);
    }
    uKernelVariable.UserEntry();

    /* �����̵߳��ȹ��� */
    uKernelVariable.Schedulable = eTrue;

    /* ��ϵͳʱ�ӽ��� */
    CpuStartTickClock();

    /* �򿪴������ж� */
    CpuEnableInt();

    /* ����IDLE Hook��������ʱ���̻߳����Ѿ��� */
    while (eTrue)
    {
        if (uKernelVariable.SysIdleEntry != (TSysIdleEntry)0)
        {
            uKernelVariable.SysIdleEntry();
        }
    }
}


/*************************************************************************************************
 *  ���ܣ����������ں�ROOT�߳�                                                                   *
 *  ��������                                                                                     *
 *  ���أ���                                                                                     *
 *  ˵����                                                                                       *
 *************************************************************************************************/
static void CreateRootThread(void)
{
    /* ����ں��Ƿ��ڳ�ʼ״̬ */
    if(uKernelVariable.State != eOriginState)
    {
        uDebugPanic("", __FILE__, __FUNCTION__, __LINE__);
    }

    /* ��ʼ���ں�ROOT�߳� */
    uThreadCreate(&RootThread,
                  eThreadReady,
                  THREAD_PROP_PRIORITY_FIXED|\
                  THREAD_PROP_CLEAN_STACK|\
                  THREAD_PROP_ROOT,
                  THREAD_ACAPI_ROOT,
                  xRootThreadEntry,
                  (TArgument)0,
                  (void*)RootThreadStack,
                  (TBase32)TCLC_ROOT_THREAD_STACK_BYTES,
                  (TPriority)TCLC_ROOT_THREAD_PRIORITY,
                  (TTimeTick)TCLC_ROOT_THREAD_SLICE);

    /* ��ʼ����ص��ں˱��� */
    uKernelVariable.RootThread    = &RootThread;
    uKernelVariable.NomineeThread = &RootThread;
    uKernelVariable.CurrentThread = &RootThread;
}
