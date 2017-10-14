//
// Created by Players Inc on 19/04/2017.
//

#ifndef MVCCART_TRANSACTIONMANAGER_H
#define MVCCART_TRANSACTIONMANAGER_H


#include <iostream>
#include <inttypes.h>
#include <boost/thread.hpp>
#include <boost/thread/locks.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/tuple/tuple.hpp>
#include <stdint.h>
#include <stdbool.h>
#include "core/Tuple.hpp"
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <assert.h>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread/thread.hpp>
#include <boost/signals2.hpp>
#include "mvcc/snapshot.hpp"
#include "GlobalEpoch.hpp"


///Transaction Globals
boost::mutex cntmutex;
std::vector<size_t> active_transactionIds;
std::map<size_t,std::string> TransactionsStatus;
boost::atomic<int> nextTid;
boost::thread_group TransactionGroup;


size_t get_new_transaction_ID()
{
    ///cntmutex.lock(); //emulates strict order
    size_t Tid;
    nextTid.fetch_add(1, boost::memory_order_relaxed);
    Tid = nextTid.load(boost::memory_order_relaxed);
    ///cntmutex.unlock();
    //boost::this_thread::sleep(boost::posix_time::milliseconds(10));
    return Tid;
}

size_t reset_transaction_ID()
{
    int temp = (-1)*nextTid;
    nextTid.fetch_add(temp, boost::memory_order_relaxed);
}


template <typename T, typename C>
void ThreadFunc1(T func,C& container , size_t id)
{
    /// Set Transaction to active
    active_transactionIds.push_back(id);
    func(container,id);
}

template <typename T, typename C>
void ThreadFunc2(T func,C& container , size_t id,std::pair<int,int> range)
{
    /// Set Transaction to active
    active_transactionIds.push_back(id);
    func(container,id,range);
}

template <typename T, typename C>
void ThreadFunc3(T func,C& container , size_t id,int numVersions,int keyIndex,int delayms)
{
    /// Set Transaction to active
    active_transactionIds.push_back(id);
    func(container,id,numVersions,keyIndex,delayms);
}


template <typename T, typename C>
void ThreadFunc4(T func,C& container , size_t id,std::pair<int,int> range,std::vector<void*>& ReadSet,std::vector<void*>& WriteSet)
{
    std::cout<<"starting transaction"<<id<<std::endl;
    // Set Transaction to active
    //vectorPair vPair = std::pair<std::vector<void*>,std::vector<void*>>(ReadSet,WriteSet);
    //activeTxns.insert(std::pair<size_t,vectorPair>(id,vPair));
    active_transactionIds.push_back(id);
    func(container,id,range);
}

void commitTransaction(size_t id)
{
    TransactionsStatus[id]= "Committed";
    //activeTxns.erase(id);
    //active_transactionIds.erase(std::remove(active_transactionIds.begin(), active_transactionIds.end(), id), active_transactionIds.end());
}

template <typename TransactionFunc, typename ARTContainer>
class Transaction
{

public:
    size_t  Tid;
    boost::thread* TransactionThread;
    std::string status;
    Epoch myEpoch;


    std::vector<void*> ReadSet;
    std::vector<void*> WriteSet;
    Transaction(TransactionFunc func, ARTContainer& ART);
    Transaction(TransactionFunc func, ARTContainer& ART,int numVersions,int keyIndex,int delayms);
    Transaction(TransactionFunc func, ARTContainer& ART,std::pair<int,int> range);

    void CollectTransaction()
    {
        TransactionThread->join();
        commitTransaction(Tid);
        myEpoch.counter--;
    }
};

template <typename TransactionFunc, typename ARTContainer>
Transaction<TransactionFunc,ARTContainer>::Transaction(TransactionFunc func, ARTContainer& ART )
{

    Tid=get_new_transaction_ID();
    TransactionsStatus[Tid]= "Active";
    myEpoch = myEpochGlobal.getActiveEpoch();
    myEpoch.addTxnToEpoch(Tid);
    TransactionThread = new boost::thread(&ThreadFunc1<TransactionFunc,ARTContainer>,func,boost::ref(ART),Tid);
    TransactionGroup.add_thread(TransactionThread);
}

template <typename TransactionFunc, typename ARTContainer>
Transaction<TransactionFunc,ARTContainer>::Transaction(TransactionFunc func, ARTContainer& ART,std::pair<int,int> range)
{
    Tid=get_new_transaction_ID();
    TransactionsStatus[Tid]= "Active";
    myEpoch = myEpochGlobal.getActiveEpoch();
    myEpoch.addTxnToEpoch(Tid);
    TransactionThread = new boost::thread(&ThreadFunc4<TransactionFunc,ARTContainer>,func,
                                          boost::ref(ART),Tid,range,boost::ref(ReadSet),boost::ref(WriteSet));
    TransactionGroup.add_thread(TransactionThread);

}

template <typename TransactionFunc, typename ARTContainer>
Transaction<TransactionFunc,ARTContainer>::Transaction(TransactionFunc func, ARTContainer& ART,int numVersions,int keyIndex,int delayms)
{
    Tid=get_new_transaction_ID();
    TransactionsStatus[Tid]= "Active";
    myEpoch = myEpochGlobal.getActiveEpoch();
    myEpoch.addTxnToEpoch(Tid);
    TransactionThread = new boost::thread(&ThreadFunc3<TransactionFunc,ARTContainer>,func,boost::ref(ART),Tid,numVersions,keyIndex,delayms);
    TransactionGroup.add_thread(TransactionThread);
}

#endif //MVCCART_TRANSACTIONMANAGER_H