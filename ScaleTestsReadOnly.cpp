#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE MVCC_TEST

#include <iostream>
#include <boost/test/unit_test.hpp>
#include <boost/lexical_cast.hpp>
#include "mvcc.hpp"
#include "ARTFULCpp.h"
#include "transactionManager.h"

#include <atomic>
#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <cassert>
#include "core/Tuple.hpp"
#include <boost/tuple/tuple.hpp>
#include <random>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include <boost/random/variate_generator.hpp>


using namespace std;


namespace
{
    auto hr_now() -> decltype(std::chrono::high_resolution_clock::now())
    {
        return std::chrono::high_resolution_clock::now();
    }

    string INIT = "init";
    string OVERWRITTEN = "overwritten";
    string UPDATED = "updated";
    string DISTURBED = "disturbed";
}


typedef pfabric::Tuple<unsigned long, int,string, double> RecordType;
typedef char KeyType[20];
typedef ARTFULCpp<RecordType,KeyType> ARTTupleContainer;
typedef std::function <void(ARTTupleContainer&,size_t id,std::string& status)> TableOperationOnTupleFunc;

auto ARTableWithTuples =  new ARTTupleContainer();
char KeysToStore[235890][20];
std::vector<RecordType> mValue;


static  int cb(void *data, const unsigned char* key, uint32_t key_len, void *val)
{
    if(val != NULL)
    {

        mvcc11::mvcc<RecordType>* _mvvcValue = reinterpret_cast<mvcc11::mvcc<RecordType>*>(val);
         //std::cout<<"###found K/V ="<<key<<"/"<<_mvvcValue->current()->value.getAttribute<1>()<<"/current="<<_mvvcValue->current()->version<<"\n";
        //if(_mvvcValue->current()->_older_snapshot != nullptr)
        //std::cout<<"/old="<<_mvvcValue->current()->_older_snapshot->version<<" / oldvalue="<<_mvvcValue->current()->_older_snapshot->value<<"\n";
    }
    return 0;
}

static  int cb2(void *data, const unsigned char* key, uint32_t key_len, void *val)
{
    if(val != NULL)
    {

        mvcc11::mvcc<RecordType>* _mvvcValue = reinterpret_cast<mvcc11::mvcc<RecordType>*>(val);
        std::cout<<"###found K/V ="<<key<<"/"<<_mvvcValue->current()->value.getAttribute<1>()<<"/current="<<_mvvcValue->current()->version<<"\n";
        //if(_mvvcValue->current()->_older_snapshot != nullptr)
        //std::cout<<"/old="<<_mvvcValue->current()->_older_snapshot->version<<" / oldvalue="<<_mvvcValue->current()->_older_snapshot->value<<"\n";
    }
    return 0;
}

static  int cb_prefix(void *data, const unsigned char* key, uint32_t key_len, void *val)
{
    if(val != NULL)
    {

        mvcc11::mvcc<RecordType>* _mvvcValue = reinterpret_cast<mvcc11::mvcc<RecordType>*>(val);
        //std::cout<<"###found prefix K/V  ="<<key<<"/"<<_mvvcValue->current()->value.getAttribute<1>()<<"/current="<<_mvvcValue->current()->version<<"\n";
    }
    return 0;
}

int current_time_nanoseconds(){
    struct timespec tm;
    clock_gettime(CLOCK_REALTIME, &tm);
    return tm.tv_nsec;
}

BOOST_AUTO_TEST_SUITE(MVCC_TESTS)

    BOOST_AUTO_TEST_CASE(test_from_loading_from_Buckets)
    {

        cout << "loading from buckets" << endl;
        int len, len2;
        char buf[20];
        char bufVal[50];

        /// Reading all Values to store against keys
        FILE *fvals = fopen("/Users/fuadshah/Desktop/MVCCART/test_data/uuid.txt", "r");
        FILE *fkeys = fopen("/Users/fuadshah/Desktop/MVCCART/test_data/words.txt", "r");
        int index = 0;

        while (fgets(bufVal, sizeof bufVal, fvals))
        {
            fgets(buf, sizeof buf, fkeys);
            len = strlen(bufVal);
            len2 = strlen(buf);
            buf[len2] = '\0';
            bufVal[len] = '\0';
            RecordType tuple = RecordType((unsigned long) index, index + 100, bufVal, index / 100.0);
            strcpy(KeysToStore[index] , buf);

            mValue.push_back(tuple);
            index++;

            if (index > 200000)
            {
                break;
            }
        }

    }

    BOOST_AUTO_TEST_CASE(test_load_ARTIndex_MVCC_twenty_thousand_keys_single_transaction)
    {
        cout << "test_load_ARTIndex_MVCC_twenty_thousand_keys_single_transaction" << endl;

        ///Writer#1 Insert/Update from Bucket
        auto WriteKeys1= [] (ARTTupleContainer& ARTable,size_t id,std::string& status)
        {
            int index=0;
            while (true)
            {
                ARTable.insertOrUpdateByKey(KeysToStore[index],mValue[index],id,status);
                index++;

                if(index > 235890)
                {
                    break;
                }
            }
        };


        auto start_time = std::chrono::high_resolution_clock::now();
        Transaction<TableOperationOnTupleFunc,ARTTupleContainer>* t1 = new Transaction<TableOperationOnTupleFunc,ARTTupleContainer>(WriteKeys1,*ARTableWithTuples);
        t1->CollectTransaction();
        auto end_time= std::chrono::high_resolution_clock::now();

        cout<<"Single Thread Writer Time->";
        cout << std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time).count() << ":";
        cout << std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count() << ":";
    }

    BOOST_AUTO_TEST_CASE(test_find_100_values)
    {
            cout << "test_scan_through_all_tree" << endl;
            ///Iterator Operation on TupleContainer
            auto findKeys1 = [] (ARTTupleContainer& ARTWithTuples,size_t id,std::string& status)
            {
                std::vector<void *> writeSet;
                std::vector<void *> ReadSet;

                random::mt19937 rng(current_time_nanoseconds());
                random::uniform_int_distribution<> randomKeys(0,200000);
                cout << randomKeys(rng) << endl;

                std::cout<<"Reading 100 keys'values:: by transaction:: "<<id<<std::endl;

                for(int i=0; i < 100; i++)
                {
                    char* keysToFind = KeysToStore[randomKeys(rng)];
                    void* val = ARTWithTuples.findValueByKey(keysToFind);
                    mvcc11::mvcc<RecordType>* _mvvcValue = reinterpret_cast<mvcc11::mvcc<RecordType>*>(val);
                    std::cout<<"###found K/V ="<<keysToFind<<"/"<<_mvvcValue->current()->value.getAttribute<1>()<<"/current="<<_mvvcValue->current()->version<<"\n";


                }

            };

            auto start_time2 = std::chrono::high_resolution_clock::now();
            Transaction<TableOperationOnTupleFunc,ARTTupleContainer>* t2 =  new Transaction<TableOperationOnTupleFunc,ARTTupleContainer>(findKeys1,*ARTableWithTuples);

            t2->CollectTransaction();
            auto end_time2= std::chrono::high_resolution_clock::now();
            cout << std::chrono::duration_cast<std::chrono::seconds>(end_time2 - start_time2).count() << ":";
            cout << std::chrono::duration_cast<std::chrono::microseconds>(end_time2 - start_time2).count() << ":";
        }

BOOST_AUTO_TEST_SUITE_END()

