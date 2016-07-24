/*************************************************************************************************
 *                                     Trochili RTOS Kernel                                      *
 *                                  Copyright(C) 2016 LIUXUMING                                  *
 *                                       www.trochili.com                                        *
 *************************************************************************************************/
#include <string.h>

#include "tcl.types.h"
#include "tcl.config.h"
#include "tcl.cpu.h"
#include "tcl.debug.h"
#include "tcl.kernel.h"
#include "tcl.ipc.h"
#include "tcl.mutex.h"

#if ((TCLC_IPC_ENABLE)&&(TCLC_IPC_MUTEX_ENABLE))

/*************************************************************************************************
 *  ����: ����ʹ���̻߳�û��⻥����                                                             *
 *  ����: (1) pThread  �߳̽ṹ��ַ                                                              *
 *        (2) pMutex   �������ṹ��ַ                                                            *
 *        (3) pHiRP    �Ƿ��и������ȼ�����                                                      *
 *  ����: (1) ��                                                                                 *
 *  ˵����������һ���ǵ�ǰ�̵߳��ã����ߵ�ǰ�̻߳�û����������߰ѻ�������������߳�             *
 *        ������ĳ�������ȼ�������ߣ����Ը���ǰ�߳�ֱ�ӱȽ����ȼ�                               *
 *************************************************************************************************/
/* 1 ���̻߳����£��������ض�����ǰ�̵߳���
     1.1 ��ǰ�߳̿��ܻ���ñ�����(lock)��ռ�õĻ�������
     1.2 ��ǰ�߳̿��ܻ���ñ�����(free)�����������������߳�(���������ľ���״̬)
   2 ��isr�����²����ܵ��ñ����� */
static TState AddLock(TThread* pThread, TMutex* pMutex, TBool* pHiRP, TError* pError)
{
    TState state;
    TError error;

    /* �������������߳������У������ȼ����� */
    uObjListAddPriorityNode(&(pThread->LockList), &(pMutex->LockNode));
    pMutex->Nest = 1U;
    pMutex->Owner = pThread;

    /* ����߳����ȼ�û�б��̶� */
    if (!(pThread->Property & THREAD_PROP_PRIORITY_FIXED))
    {
        /* �߳����ȼ���mutex��������API�����޸� */
        pThread->Property &= ~(THREAD_PROP_PRIORITY_SAFE);

        /* PCP �õ�������֮�󣬵�ǰ�߳�ʵʩ�컨���㷨,��Ϊ���߳̿��ܻ�ö����������
        ���̵߳ĵ�ǰ���ȼ����ܱ��»�õĻ��������컨�廹�ߡ� �����������Ƚ�һ�����ȼ���
        ����ֱ�����ó��»��������컨�����ȼ� */
        if (pThread->Priority > pMutex->Priority)
        {
            state = uThreadSetPriority(pThread, pMutex->Priority, eFalse, pHiRP, &error);
        }
    }

    *pError = error;
    return state;
}


/*************************************************************************************************
 *  ����: ���߳���������ɾ��������                                                               *
 *  ����: (1) pThread �߳̽ṹ��ַ                                                               *
 *        (2) pMutex  �������ṹ��ַ                                                             *
 *        (3) pHiRP   �Ƿ��и������ȼ�����                                                       *
 *  ����: ��                                                                                     *
 *  ˵������ǰ�߳����ȼ����ͣ�ֻ�ܸ������̱߳Ƚ����ȼ�                                           *
 *************************************************************************************************/
static TState RemoveLock(TThread* pThread, TMutex* pMutex, TBool* pHiRP, TError* pError)
{
    TState    state;
    TPriority priority = TCLC_LOWEST_PRIORITY;
    TObjNode* pHead = (TObjNode*)0;
    TBool     nflag = eFalse;
    TError    error;
//    TBool     HiRP;

    /* �����������߳����������Ƴ� */
    pHead = pThread->LockList;
    uObjListRemoveNode(&(pThread->LockList), &(pMutex->LockNode));
    pMutex->Owner = (TThread*)0;
    pMutex->Nest = 0U;

    /* ����߳����ȼ�û�б��̶� */
    if (!(pThread->Property & THREAD_PROP_PRIORITY_FIXED))
    {
        /* ����߳�������Ϊ�գ����߳����ȼ��ָ����������ȼ�,
           ��mutex��߳����ȼ�һ���������̻߳������ȼ� */
        if (pThread->LockList == (TObjNode*)0)
        {
            /* ����߳�û��ռ�б�Ļ�������,�������߳����ȼ����Ա�API�޸� */
            pThread->Property |= (THREAD_PROP_PRIORITY_SAFE);

            /* ׼���ָ��߳����ȼ� */
            priority = pThread->BasePriority;
            nflag = eTrue;
        }
        else
        {
            /* ��Ϊ�������ǰ������ȼ��½����������̵߳���һ�����ȼ�һ������Ȼ��ߵ͵�,
               ע��ɾ�����������ڶ�������κ�λ�ã���������ڶ���ͷ������Ҫ�����߳����ȼ� */
            if (pHead == &(pMutex->LockNode))
            {
                /* ׼���ָ��߳����ȼ� */
                priority = *((TPriority*)(pThread->LockList->Data));
                nflag = eTrue;
            }
        }

        /* ����߳����ȼ��б仯(nflag = eTrue)������Ҫ����(priority > pThread->Priority) */
        if (nflag && (priority > pThread->Priority))
        {
            /* �޸��߳����ȼ� */
            state = uThreadSetPriority(pThread, priority, eFalse, pHiRP, &error);

        }
    }

    *pError = error;
    return state;
}


/*************************************************************************************************
 *  ����: �̻߳�û��⻥����                                                                     *
 *  ����: (1) pMutex   �������ṹ��ַ                                                            *
 *        (2) pHiRP    �Ƿ��и������ȼ�����                                                      *
 *        (3) pError   ��ϸ���ý��                                                              *
 *  ����: (1) eSuccess �����ɹ�                                                                  *
 *        (2) eFailure ����ʧ��                                                                  *
 *        (3) eError   ��������                                                                  *
 *  ˵����                                                                                       *
 *************************************************************************************************/
static TState LockMutex(TMutex* pMutex, TBool* pHiRP, TError* pError)
{
    TState state = eSuccess;
    TError error = IPC_ERR_NONE;

    /* �̻߳�û���������
     * Priority Ceilings Protocol
     * ������ɹ�, PCP�����µ�ǰ�߳����ȼ����ή��,ֱ�ӷ���
     * �����ʧ�ܲ����Ƿ�������ʽ���ʻ�������ֱ�ӷ���
     * �����ʧ�ܲ�����������ʽ���ʻ����������߳������ڻ����������������У�Ȼ����ȡ�
    */
    if (pMutex->Owner == (TThread*)0)
    {
        /*
         * ��ǰ�̻߳�û����������ȼ���ʹ�б䶯Ҳ���ɱ������, ����Ҫ�߳����ȼ���ռ��
         * HiRP��ֵ��ʱ���ô�
         */
        state = AddLock(uKernelVariable.CurrentThread, pMutex, pHiRP, &error);
    }
    else if (pMutex->Owner == uKernelVariable.CurrentThread)
    {
        pMutex->Nest++;
    }
    else
    {
        error = IPC_ERR_FORBIDDEN;
        state = eFailure;
    }

    *pError  = error;
    return state;
}

/*************************************************************************************************
 *  ����: �ͷŻ��⻥����                                                                         *
 *  ����: (1) pMutex   �������ṹ��ַ                                                            *
 *        (2) pHiRP    �Ƿ��и������ȼ�����                                                      *
 *        (3) pError   ��ϸ���ý��                                                              *
 *  ����: (1) eSuccess �����ɹ�                                                                  *
 *        (2) eFailure ����ʧ��                                                                  *
 *  ˵����ֻ�е�ǰ�̲߳��ܹ��ͷ�ĳ��������������ǰ�߳�һ����������״̬��                         *
 *        Ҳ�Ͳ�������ʽ���ȼ�����������                                                         *
 *************************************************************************************************/
static TState FreeMutex(TMutex* pMutex, TBool* pHiRP, TError* pError)
{
    TState state = eFailure;
    TError error = IPC_ERR_FORBIDDEN;
    TIpcContext* pContext;
    TThread* pThread;

    /* ֻ��ռ�л��������̲߳����ͷŸû����� */
    if (pMutex->Owner == uKernelVariable.CurrentThread)
    {
        /* ���߳�Ƕ��ռ�л�����������£���Ҫ����������Ƕ�״��� */
        pMutex->Nest--;

        /*
         * ���������Ƕ����ֵΪ0��˵��Ӧ�ó����ͷŻ�����,
         * �����ǰ�߳������������ȼ��컨��Э�飬���ǵ����߳����ȼ�
         */
        if (pMutex->Nest == 0U)
        {
            /* �����������߳����������Ƴ�,���û�����������Ϊ��. */
            state = RemoveLock(uKernelVariable.CurrentThread, pMutex, pHiRP, &error);

            /* ���Դӻ���������������ѡ����ʵ��̣߳�ʹ�ø��̵߳õ������� */
            if (pMutex->Property & IPC_PROP_PRIMQ_AVAIL)
            {
                pContext = (TIpcContext*)(pMutex->Queue.PrimaryHandle->Owner);
                uIpcUnblockThread(pContext, eSuccess, IPC_ERR_NONE, pHiRP);

                pThread = (TThread*)(pContext->Owner);
                state = AddLock(pThread, pMutex, pHiRP, &error);
            }
        }

        error = IPC_ERR_NONE;
        state = eSuccess;
    }

    *pError = error;
    return state;
}

/*
 * ���������ݲ�������ISR�б�����
 */

/*************************************************************************************************
 *  ����: �ͷŻ��⻥����                                                                         *
 *  ����: (1) pMutex   �������ṹ��ַ                                                            *
 *        (2) pError   ��ϸ���ý��                                                              *
 *  ����: (1) eSuccess �����ɹ�                                                                  *
 *        (2) eFailure ����ʧ��                                                                  *
 *  ˵����mutex֧������Ȩ�ĸ�������߳��ͷ�mutex�Ĳ����������̷��ص�,���ͷ�mutex�������ᵼ��   *
 *        �߳�������mutex���߳�����������                                                        *
 *************************************************************************************************/
TState xMutexFree(TMutex* pMutex, TError* pError)
{
    TState state = eFailure;
    TError error = IPC_ERR_UNREADY;
    TReg32 imask;
    TBool HiRP = eFalse;

    CpuEnterCritical(&imask);

    if (pMutex->Property & IPC_PROP_READY)
    {
        if (uKernelVariable.State == eThreadState)
        {
            state = FreeMutex(pMutex, &HiRP, &error);
            if (uKernelVariable.Schedulable == eTrue)
            {
                if (state == eSuccess)
                {
                    if (HiRP == eTrue)
                    {
                        uThreadSchedule();
                    }
                }
            }
        }

    }

    CpuLeaveCritical(imask);

    *pError = error;
    return state;
}


/*************************************************************************************************
 *  ����: �̻߳�û��⻥����                                                                     *
 *  ����: (1) pMutex �������ṹ��ַ                                                              *
 *        (2) option   ���������ģʽ                                                            *
 *        (3) timeo    ʱ������ģʽ�·��ʻ�������ʱ�޳���                                        *
 *        (4) pError   ��ϸ���ý��                                                              *
 *  ����: (1) eSuccess �����ɹ�                                                                  *
 *        (2) eFailure ����ʧ��                                                                  *
 *        (3) eError   ��������                                                                  *
 *  ˵����                                                                                       *
 *************************************************************************************************/
/*
 * �̲߳��÷�������ʽ��������ʽ����ʱ�޵ȴ���ʽ��û�����
 * Priority Ceilings Protocol
 * ������ɹ�, PCP�����µ�ǰ�߳����ȼ����ή��,ֱ�ӷ���
 * �����ʧ�ܲ����Ƿ�������ʽ���ʻ�������ֱ�ӷ���
 * �����ʧ�ܲ�����������ʽ���ʻ����������߳������ڻ����������������У�Ȼ����ȡ�
 */
TState xMutexLock(TMutex* pMutex, TOption option, TTimeTick timeo, TError* pError)
{
    TState state = eFailure;
    TError error = IPC_ERR_UNREADY;
    TBool HiRP = eFalse;
    TIpcContext* pContext;
    TReg32 imask;

    CpuEnterCritical(&imask);
    if (pMutex->Property & IPC_PROP_READY)
    {
        if (uKernelVariable.State == eThreadState)
        {
            state = LockMutex(pMutex, &HiRP, &error);

            if (uKernelVariable.Schedulable == eTrue)
            {
                if (state == eFailure)
                {
                    if (option & IPC_OPT_WAIT)
                    {
                        /* �õ���ǰ�̵߳�IPC�����Ľṹ��ַ */
                        pContext = &(uKernelVariable.CurrentThread->IpcContext);

                        /* �趨�߳����ڵȴ�����Դ����Ϣ */
                        uIpcSaveContext(pContext, (void*)pMutex, 0U, 0U, (option | IPC_OPT_MUTEX), &state, &error);

                        /* ��ǰ�߳������ڸû��������������У�ʱ�޻������޵ȴ�����IPC_OPT_TIMED�������� */
                        uIpcBlockThread(pContext, &(pMutex->Queue), timeo);

                        /* ��ǰ�̱߳������������̵߳���ִ�� */
                        uThreadSchedule();

                        CpuLeaveCritical(imask);
                        /*
                        * ��Ϊ��ǰ�߳��Ѿ�������IPC������߳��������У����Դ�������Ҫִ�б���̡߳�
                        * ���������ٴδ����߳�ʱ���ӱ����������С�
                        */
                        CpuEnterCritical(&imask);

                        /* ����̹߳�����Ϣ */
                        uIpcCleanContext(pContext);
                    }
                }
            }
        }
    }

    CpuLeaveCritical(imask);

    *pError = error;
    return state;
}


/*************************************************************************************************
 *  ����: ��ʼ��������                                                                           *
 *  ����: (1) pMute    �������ṹ��ַ                                                            *
 *        (2) priority �����������ȼ��컨��                                                      *
 *        (3) property �������ĳ�ʼ����                                                          *
 *        (4) pError   ��ϸ���ý��                                                              *
 *  ����: (1) eSuccess �����ɹ�                                                                  *
 *        (2) eFailure ����ʧ��                                                                  *
 *  ˵����                                                                                       *
 *************************************************************************************************/
TState xMutexCreate(TMutex* pMutex, TPriority priority, TProperty property, TError* pError)
{
    TState state = eFailure;
    TError error = IPC_ERR_FAULT;
    TReg32 imask;

    CpuEnterCritical(&imask);

    /* ֻ�������̴߳�������ñ����� */
    if (uKernelVariable.State == eThreadState)
    {
        if (!(pMutex->Property & IPC_PROP_READY))
        {
            property |= IPC_PROP_READY;
            pMutex->Property = property;
            pMutex->Nest = 0U;
            pMutex->Owner = (TThread*)0;
            pMutex->Priority = priority;

            pMutex->Queue.PrimaryHandle   = (TObjNode*)0;
            pMutex->Queue.AuxiliaryHandle = (TObjNode*)0;
            pMutex->Queue.Property        = &(pMutex->Property);

            pMutex->LockNode.Owner = (void*)pMutex;
            pMutex->LockNode.Data = (TBase32*)(&(pMutex->Priority));
            pMutex->LockNode.Next = 0;
            pMutex->LockNode.Prev = 0;
            pMutex->LockNode.Handle = (TObjNode**)0;

            error = IPC_ERR_NONE;
            state = eSuccess;
        }
    }

    CpuLeaveCritical(imask);

    *pError = error;
    return state;
}


/*************************************************************************************************
 *  ����: �����������ʼ��                                                                       *
 *  ����: (1) pMutex   �������ṹ��ַ                                                            *
 *        (2) pError   ��ϸ���ý��                                                              *
 *  ����: (1) eSuccess �����ɹ�                                                                  *
 *        (2) eFailure ����ʧ��                                                                  *
 *  ˵����                                                                                       *
 *************************************************************************************************/
TState xMutexDelete(TMutex* pMutex, TError* pError)
{
    TState state = eFailure;
    TError error = IPC_ERR_FAULT;
    TReg32 imask;
    TBool HiRP = eFalse;

    CpuEnterCritical(&imask);

    /* ֻ�������̴߳�������ñ����� */
    if (uKernelVariable.State == eThreadState)
    {
        if (pMutex->Property & IPC_PROP_READY)
        {
            /* ֻ�е����������߳�ռ�е�����£����п��ܴ��ڱ��������������߳� */
            if (pMutex->Owner != (TThread*)0)
            {
                /* �����������߳����������Ƴ� */
                state = RemoveLock(pMutex->Owner, pMutex, &HiRP, &error);

                /* �����������ϵ����еȴ��̶߳��ͷţ������̵߳ĵȴ��������IPC_ERR_DELETE��
                 * ������Щ�̵߳����ȼ�һ�������ڻ����������ߵ����ȼ�
                 */
                uIpcUnblockAll(&(pMutex->Queue), eFailure, IPC_ERR_DELETE,
                               (void**)0, &HiRP);
            }

            /* ��������������ȫ������ */
            memset(pMutex, 0U, sizeof(TMutex));

            /*
             * ���̻߳����£������ǰ�̵߳����ȼ��Ѿ��������߳̾������е�������ȼ���
             * �����ں˴�ʱ��û�йر��̵߳��ȣ���ô����Ҫ����һ���߳���ռ
             */
            if (//(uKernelVariable.State == eThreadState) &&
                (uKernelVariable.Schedulable == eTrue) &&
                (HiRP == eTrue))
            {
                uThreadSchedule();
            }

            state = eSuccess;
            error = IPC_ERR_NONE;
        }
        else
        {
            error = IPC_ERR_UNREADY;
        }
    }
    CpuLeaveCritical(imask);

    *pError = error;
    return state;
}


/*************************************************************************************************
 *  ����: ���û�����                                                                             *
 *  ����: (1) pMutex   �������ṹ��ַ                                                            *
 *        (2) pError   ��ϸ���ý��                                                              *
 *  ����: (1) eSuccess �����ɹ�                                                                  *
 *        (2) eFailure ����ʧ��                                                                  *
 *  ˵����                                                                                       *
 *************************************************************************************************/
TState xMutexReset(TMutex* pMutex, TError* pError)
{
    TState state = eFailure;
    TError error = IPC_ERR_FAULT;
    TReg32 imask;
    TBool HiRP = eFalse;

    CpuEnterCritical(&imask);

    /* ֻ�������̴߳�������ñ����� */
    if (uKernelVariable.State == eThreadState)
    {
        if (pMutex->Property & IPC_PROP_READY)
        {
            /* ֻ�е����������߳�ռ�е�����£����п��ܴ��ڱ��������������߳� */
            if (pMutex->Owner != (TThread*)0)
            {
                /* �����������߳����������Ƴ� */
                state = RemoveLock(pMutex->Owner, pMutex, &HiRP, &error);

                /* �����������ϵ����еȴ��̶߳��ͷţ������̵߳ĵȴ��������IPC_ERR_RESET��
                   ������Щ�̵߳����ȼ�һ�������ڻ����������ߵ����ȼ� */
                uIpcUnblockAll(&(pMutex->Queue), eFailure, IPC_ERR_RESET,
                               (void**)0, &HiRP);

                /* �ָ����������� */
                pMutex->Property &= IPC_RESET_MUTEX_PROP;
            }

            /* ռ�и���Դ�Ľ���Ϊ�� */
            pMutex->Property &= IPC_RESET_MUTEX_PROP;
            pMutex->Owner = (TThread*)0;
            pMutex->Nest = 0U;
            /* pMutex->Priority = keep recent value; */
            pMutex->LockNode.Owner = (void*)0;
            pMutex->LockNode.Data = (TBase32*)0;

            /*
             * ���̻߳����£������ǰ�̵߳����ȼ��Ѿ��������߳̾������е�������ȼ���
             * �����ں˴�ʱ��û�йر��̵߳��ȣ���ô����Ҫ����һ���߳���ռ
             */
            if (//(uKernelVariable.State == eThreadState) &&
                (uKernelVariable.Schedulable == eTrue) &&
                (HiRP == eTrue))
            {
                uThreadSchedule();
            }

            state = eSuccess;
            error = IPC_ERR_NONE;

        }
        else
        {
            error = IPC_ERR_UNREADY;
        }
    }
    CpuLeaveCritical(imask);

    *pError = error;
    return state;
}


/*************************************************************************************************
 *  ���ܣ�������������ֹ����,��ָ�����̴߳ӻ��������߳�������������ֹ����������                  *
 *  ������(1) pMutex   �������ṹ��ַ                                                            *
 *        (2) option   ����ѡ��                                                                  *
 *        (3) pThread  �̵߳�ַ                                                                  *
 *        (4) pError   ��ϸ���ý��                                                              *
 *  ����: (1) eSuccess �����ɹ�                                                                  *
 *        (2) eFailure ����ʧ��                                                                  *
 *  ˵����                                                                                       *
 *************************************************************************************************/
TState xMutexFlush(TMutex* pMutex, TError* pError)
{
    TState state = eFailure;
    TError error = IPC_ERR_FAULT;
    TReg32 imask;
    TBool HiRP = eFalse;

    CpuEnterCritical(&imask);

    /* ֻ�������̴߳�������ñ����� */
    if (uKernelVariable.State == eThreadState)
    {
        if (pMutex->Property & IPC_PROP_READY)
        {
            if (pMutex->Property & IPC_PROP_READY)
            {
                /* �����������������ϵ����еȴ��̶߳��ͷţ������̵߳ĵȴ��������TCLE_IPC_FLUSH  */
                uIpcUnblockAll(&(pMutex->Queue), eFailure, IPC_ERR_FLUSH, (void**)0, &HiRP);

                state = eSuccess;
                error = IPC_ERR_NONE;

                /*
                 * ���̻߳����£������ǰ�̵߳����ȼ��Ѿ��������߳̾������е�������ȼ���
                 * �����ں˴�ʱ��û�йر��̵߳��ȣ���ô����Ҫ����һ���߳���ռ
                 */
                if (//(uKernelVariable.State == eThreadState) &&
                    (uKernelVariable.Schedulable == eTrue) &&
                    (HiRP == eTrue))
                {
                    uThreadSchedule();
                }
            }
        }
        else
        {
            error = IPC_ERR_UNREADY;
        }
    }
    CpuLeaveCritical(imask);

    *pError = error;
    return state;
}
#endif

