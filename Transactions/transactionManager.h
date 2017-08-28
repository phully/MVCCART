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

#define INF 99999


boost::mutex cntmutex;
std::vector<size_t> active_transactionIds;
boost::atomic<int> nextTid;
boost::thread_group TransactionGroup;


auto Active = "active";
auto Preparing = "preparing";
auto Commit = "commit";
auto Abort = "abort";

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


namespace smart_ptr
{
    using boost::shared_ptr;
    using boost::make_shared;
    using boost::atomic_load;
    using boost::atomic_store;

    template <class T>
    bool atomic_compare_exchange_strong(shared_ptr<T> * p, shared_ptr<T> * v, shared_ptr<T> w)
    {
        return boost::atomic_compare_exchange(p, v, w);
    }

}

class TransactionManager
{
    public:
    static boost::mutex cntmutex;
    static boost::atomic<int> nextTid;
    boost::thread_group TransactionGroup;
    std::vector<size_t> active_transactions;

    void CollectTranscations()
    {
        TransactionGroup.join_all();
    }
};

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
void ThreadFunc3(T func,C& container , size_t id,std::pair<int,int> range,std::vector<void*>& ReadSet,std::vector<void*>& WriteSet)
{
    /// Set Transaction to active
    active_transactionIds.push_back(id);
    func(container,id,range);
}

void commitTransaction(size_t id)
{
    active_transactionIds.erase(std::remove(active_transactionIds.begin(), active_transactionIds.end(), id), active_transactionIds.end());
}

template <typename TransactionFunc, typename ARTContainer>
class Transaction
{
    public:
    size_t  Tid;
    boost::thread* TransactionThread;
    std::string status;




    Transaction(TransactionFunc func, ARTContainer& ART );
    Transaction(TransactionFunc func, ARTContainer& ART,std::pair<int,int> range);
    Transaction(TransactionFunc func, ARTContainer& ART,std::pair<int,int> range,std::vector<void*>& ReadSet,std::vector<void*>& WriteSet);


    void CollectTransaction()
    {
        TransactionThread->join();
        commitTransaction(Tid);
    }
};


template <typename TransactionFunc, typename ARTContainer>
Transaction<TransactionFunc,ARTContainer>::Transaction(TransactionFunc func, ARTContainer& ART )
{
    Tid=get_new_transaction_ID();
    TransactionThread = new boost::thread(&ThreadFunc1<TransactionFunc,ARTContainer>,func,boost::ref(ART),Tid);
    TransactionGroup.add_thread(TransactionThread);
}

template <typename TransactionFunc, typename ARTContainer>
Transaction<TransactionFunc,ARTContainer>::Transaction(TransactionFunc func, ARTContainer& ART,std::pair<int,int> range )
{
    Tid=get_new_transaction_ID();
    TransactionThread = new boost::thread(&ThreadFunc2<TransactionFunc,ARTContainer>,func,boost::ref(ART),Tid,range);
    TransactionGroup.add_thread(TransactionThread);

}

template <typename TransactionFunc, typename ARTContainer>
Transaction<TransactionFunc,ARTContainer>::Transaction(TransactionFunc func, ARTContainer& ART,std::pair<int,int> range,std::vector<void*>& ReadSet,std::vector<void*>& WriteSet)
{
    Tid=get_new_transaction_ID();
    TransactionThread = new boost::thread(&ThreadFunc3<TransactionFunc,ARTContainer>,func,boost::ref(ART),Tid,range,boost::ref(ReadSet),boost::ref(WriteSet));
    TransactionGroup.add_thread(TransactionThread);
}

#endif //MVCCART_TRANSACTIONMANAGER_H