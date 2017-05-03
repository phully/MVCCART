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


boost::mutex cntmutex;
boost::atomic<int> nextTid;

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

} // namespace smart_ptr

class TransactionManager
{
/*public:
    static TransactionManager * instance()
    {
        TransactionManager * tmp = instance_.load(boost::memory_order_consume);
        if (!tmp)
        {
            boost::mutex::scoped_lock guard(instantiation_mutex);
            tmp = instance_.load(boost::memory_order_consume);
            if (!tmp) {
                tmp = new TransactionManager;
                instance_.store(tmp, boost::memory_order_release);
            }
        }
        return tmp;
    }
private:
    static boost::atomic<TransactionManager *> instance_;
    static boost::mutex instantiation_mutex;
*/
public:
    static boost::mutex cntmutex;
    static boost::atomic<int> nextTid;
    //static boost::atomic<int> nextTid;
    boost::thread_group TransactionGroup;

    void CollectTranscations()
    {
        TransactionGroup.join_all();
    }

};
template <typename T, typename C>
void todo(T func,C& container , size_t id)
{
    //this_thread::sleep_for(chrono::seconds(1));
    func(container,id);
    std::cout<<"Thread ID= "<<boost::this_thread::get_id()<<std::endl;
    std::cout<<id<<std::endl;
}

template <typename TransactionFunc, typename ARTContainer>
class Transaction
{
public:
    int  Tid;
    boost::thread* TransactionThread;

    Transaction(TransactionFunc func, ARTContainer& ART )
    {
        ///atomic load and increment
        //create a new function for handling Mutex Locks generically defined
        //cntmutex.lock(); //emulates strict order
        nextTid.fetch_add(1, boost::memory_order_relaxed);
        Tid = nextTid.load(boost::memory_order_relaxed);
        TransactionThread = new boost::thread(&todo<TransactionFunc,ARTContainer>,func,ART,Tid);
        //cntmutex.unlock();
    }

    void CollectTransaction()
    {
        TransactionThread->join();
    }
};






#endif //MVCCART_TRANSACTIONMANAGER_H
