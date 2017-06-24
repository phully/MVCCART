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
auto ARTableWithTuples2 =  new ARTTupleContainer();
auto ARTableWithTuples3 =  new ARTTupleContainer();

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

    BOOST_AUTO_TEST_CASE(test_load_ARTIndex_MVCC_twenty_thousand_keys_by_four_transaction)
    {
        cout << "\ntest 4-multi writes" << endl;

        ///Writer#1 Insert/Update from Disk
        auto WriteKeys1= [] (ARTTupleContainer& ARTable,size_t id,std::string& status)
        {
            int index=0;
            while (true)
            {
                ARTable.insertOrUpdateByKey(KeysToStore[index],mValue[index],id,status);
                index++;

                if(index > 50000)
                {
                    break;
                }
            }
        };

        auto WriteKeys2= [] (ARTTupleContainer& ARTable,size_t id,std::string& status)
        {
            int index=50000;
            while (true)
            {
                ARTable.insertOrUpdateByKey(KeysToStore[index],mValue[index],id,status);
                index++;

                if(index > 100000)
                {
                    break;
                }
            }
        };


        auto WriteKeys3= [] (ARTTupleContainer& ARTable,size_t id,std::string& status)
        {
            int index=100000;
            while (true)
            {
                ARTable.insertOrUpdateByKey(KeysToStore[index],mValue[index],id,status);
                index++;

                if(index > 150000)
                {
                    break;
                }
            }
        };

        auto WriteKeys4= [] (ARTTupleContainer& ARTable,size_t id,std::string& status)
        {
            int index=200000;
            while (true)
            {
                ARTable.insertOrUpdateByKey(KeysToStore[index],mValue[index],id,status);
                index++;

                if(index > 400)
                {
                    break;
                }
            }
        };


        auto start_time = std::chrono::high_resolution_clock::now();
        Transaction<TableOperationOnTupleFunc,ARTTupleContainer>* t1 = new Transaction<TableOperationOnTupleFunc,ARTTupleContainer>(WriteKeys1,*ARTableWithTuples3);
        Transaction<TableOperationOnTupleFunc,ARTTupleContainer>* t2 = new Transaction<TableOperationOnTupleFunc,ARTTupleContainer>(WriteKeys2,*ARTableWithTuples3);
        Transaction<TableOperationOnTupleFunc,ARTTupleContainer>* t3 = new Transaction<TableOperationOnTupleFunc,ARTTupleContainer>(WriteKeys3,*ARTableWithTuples3);
        Transaction<TableOperationOnTupleFunc,ARTTupleContainer>* t4 = new Transaction<TableOperationOnTupleFunc,ARTTupleContainer>(WriteKeys4,*ARTableWithTuples3);
        t1->CollectTransaction();
        t2->CollectTransaction();
        t3->CollectTransaction();
        t4->CollectTransaction();
        auto end_time= std::chrono::high_resolution_clock::now();

        cout<<"Multi Thread 4-Writers total Time->";
        cout << std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time).count() << ":";
        cout << std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count() << ":";

    }

    BOOST_AUTO_TEST_CASE(test_scan_parallel_vals)
    {
        cout << "\ntest_scan_by_4_transactions" << endl;
        ///Iterator Operation on TupleContainer
        auto scanAll = [] (ARTTupleContainer& ARTWithTuples,size_t id,std::string& status)
        {
            std::cout<<"iterate by::"<<id<<std::endl;
            KeyType prefix = "A";
            //ARTWithTuples.iterate(cb2,id);
        };

        auto start_time2 = std::chrono::high_resolution_clock::now();
        Transaction<TableOperationOnTupleFunc,ARTTupleContainer>* t2 = new Transaction<TableOperationOnTupleFunc,ARTTupleContainer>(scanAll,*ARTableWithTuples3);
        t2->CollectTransaction();
        auto end_time2= std::chrono::high_resolution_clock::now();
        cout << std::chrono::duration_cast<std::chrono::seconds>(end_time2 - start_time2).count() << ":";
        cout << std::chrono::duration_cast<std::chrono::microseconds>(end_time2 - start_time2).count() << ":";
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

    BOOST_AUTO_TEST_CASE(test_scan_prefixes_vals)
    {
        cout << "\ntest_scan_through_prefixes" << endl;
        ///Iterator Operation on TupleContainer
        auto scanAll = [] (ARTTupleContainer& ARTWithTuples,size_t id,std::string& status)
        {
            std::cout<<"iterate by::"<<id<<std::endl;
            KeyType prefix = "A";
            ARTWithTuples.iterate(cb_prefix,(char*)prefix,id);
        };

        auto start_time2 = std::chrono::high_resolution_clock::now();
        Transaction<TableOperationOnTupleFunc,ARTTupleContainer>* t2 = new Transaction<TableOperationOnTupleFunc,ARTTupleContainer>(scanAll,*ARTableWithTuples);
        t2->CollectTransaction();
        auto end_time2= std::chrono::high_resolution_clock::now();
        cout << std::chrono::duration_cast<std::chrono::seconds>(end_time2 - start_time2).count() << ":";
        cout << std::chrono::duration_cast<std::chrono::microseconds>(end_time2 - start_time2).count() << ":";
    }

    BOOST_AUTO_TEST_CASE(test_scan_all_vals)
    {
            cout << "test_scan_through_all_tree" << endl;
            ///Iterator Operation on TupleContainer
            auto scanAll = [] (ARTTupleContainer& ARTWithTuples,size_t id,std::string& status)
            {
                std::cout<<"iterate by::"<<id<<std::endl;
                ARTWithTuples.iterate(cb,id);
            };

            auto start_time2 = std::chrono::high_resolution_clock::now();
            Transaction<TableOperationOnTupleFunc,ARTTupleContainer>* t2 = new Transaction<TableOperationOnTupleFunc,ARTTupleContainer>(scanAll,*ARTableWithTuples);
            t2->CollectTransaction();
            auto end_time2= std::chrono::high_resolution_clock::now();
            cout << std::chrono::duration_cast<std::chrono::seconds>(end_time2 - start_time2).count() << ":";
            cout << std::chrono::duration_cast<std::chrono::microseconds>(end_time2 - start_time2).count() << ":";
        }

    BOOST_AUTO_TEST_CASE(test_load_ARTIndex_MVCC_twenty_thousand_keys_by_two_transaction)
    {
        cout << "test multi writes" << endl;

        ///Writer#1 Insert/Update from Disk
        auto WriteKeys1 = [](ARTTupleContainer &ARTable, size_t id, std::string &status) {
            int index = 0;
            while (true) {
                ARTable.insertOrUpdateByKey(KeysToStore[index], mValue[index], id, status);
                index++;

                if (index > 100000) {
                    break;
                }
            }
        };

        auto WriteKeys2 = [](ARTTupleContainer &ARTable, size_t id, std::string &status) {
            int index = 100000;
            while (true) {
                ARTable.insertOrUpdateByKey(KeysToStore[index], mValue[index], id, status);
                index++;

                if (index > 200000) {
                    break;
                }
            }
        };


        auto start_time = std::chrono::high_resolution_clock::now();
        Transaction<TableOperationOnTupleFunc, ARTTupleContainer> *t1 = new Transaction<TableOperationOnTupleFunc, ARTTupleContainer>(
                WriteKeys1, *ARTableWithTuples2);
        Transaction<TableOperationOnTupleFunc, ARTTupleContainer> *t2 = new Transaction<TableOperationOnTupleFunc, ARTTupleContainer>(
                WriteKeys2, *ARTableWithTuples2);
        t1->CollectTransaction();
        t2->CollectTransaction();
        auto end_time = std::chrono::high_resolution_clock::now();

        cout << "Multi 2 Thread Writer Time->";
        cout << std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time).count() << ":";
        cout << std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count() << ":";
    }


BOOST_AUTO_TEST_SUITE_END()

