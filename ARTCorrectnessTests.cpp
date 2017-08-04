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
#include "fmt-master/fmt/format.h"


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


typedef pfabric::Tuple<string,unsigned long, int,string, double> RecordType;
typedef char KeyType[20];
typedef ARTFULCpp<RecordType,KeyType> ARTTupleContainer;
typedef std::function <void(ARTTupleContainer&,size_t id,std::string& status)> TableOperationOnTupleFunc;

char KeysToStore[235890][20];
std::vector<RecordType> vectorValues;


auto ARTableWithTuples =  new ARTTupleContainer();
auto ARTableWithTuples2 =  new ARTTupleContainer();
auto ARTableWithTuples4 =  new ARTTupleContainer();
auto ARTableWithTuples8 =  new ARTTupleContainer();

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

    BOOST_AUTO_TEST_CASE(test_loading_Buckets_from_TextFIle)
    {

        cout << "test_loading_Buckets_from_TextFIle" << endl;
        int len, len2;
        char buf[20];
        char bufVal[50];
        char bufVal2[20];

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

            RecordType tuple = RecordType(buf,(unsigned long) index, index + 100, fmt::format("String/{}",buf), index / 100.0);
            strcpy(KeysToStore[index] , buf);
            vectorValues.push_back(tuple);
            index++;

            if (index > 200000)
            {
                break;
            }
        }

    }


    BOOST_AUTO_TEST_CASE(test_load_ARTIndex_MVCC_two_hundred_thousand_keys_single_transaction)
    {
        cout << "test_load_ARTIndex_MVCC_two_hundred_thousand_keys_single_transaction" << endl;

        ///Writer#1 Insert/Update from Bucket
        auto WriteKeys1= [] (ARTTupleContainer& ARTable,size_t id,std::string& status)
        {
            int index=0;
            while (true)
            {
                ARTable.insertOrUpdateByKey(KeysToStore[index],vectorValues[index],id,status);
                index++;

                if(index > 200000)
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

    BOOST_AUTO_TEST_CASE(testing_random_1000_keys_loaded_by_single_transaction)
    {
        cout << "testing_1000keys_loaded_by_single_transaction, randomly between 0-200,000" << endl;
        ///Iterator Operation on TupleContainer
        auto findKeys1 = [] (ARTTupleContainer& ARTWithTuples,size_t id,std::string& status)
        {
            std::vector<void *> writeSet;
            std::vector<void *> ReadSet;

            random::mt19937 rng(current_time_nanoseconds());
            random::uniform_int_distribution<> randomKeys(0,200000);
            cout << randomKeys(rng) << endl;

            std::cout<<"Reading 100 keys'values:: by transaction:: "<<id<<std::endl;

            for(int i=0; i < 1000; i++)
            {
                char* keysToFind = KeysToStore[randomKeys(rng)];
                void* val = ARTWithTuples.findValueByKey(keysToFind);
                mvcc11::mvcc<RecordType>* _mvccValue = reinterpret_cast<mvcc11::mvcc<RecordType>*>(val);

                auto tp= _mvccValue->current()->value;

                BOOST_REQUIRE(tp.getAttribute<3>() == fmt::format("String/{}",keysToFind));
                std::cout<<"###found K/V = "<<tp.getAttribute<3>()<<"\n";
            }

        };

        auto start_time2 = std::chrono::high_resolution_clock::now();
        Transaction<TableOperationOnTupleFunc,ARTTupleContainer>* t2 =  new Transaction<TableOperationOnTupleFunc,ARTTupleContainer>(findKeys1,*ARTableWithTuples);

        t2->CollectTransaction();
        auto end_time2= std::chrono::high_resolution_clock::now();
        cout << std::chrono::duration_cast<std::chrono::seconds>(end_time2 - start_time2).count() << ":";
        cout << std::chrono::duration_cast<std::chrono::microseconds>(end_time2 - start_time2).count() << ":";
    }

    BOOST_AUTO_TEST_CASE(test_load_ARTIndex_MVCC_two_hundred_thousand_keys_two_transactions)
    {
        cout << "test_load_ARTIndex_MVCC_two_hundred_thousand_keys_two_transactions" << endl;

        ///Writer#1 Insert/Update from Disk
        auto WriteKeys1 = [](ARTTupleContainer &ARTable, size_t id, std::string &status) {
            int index = 0;
            while (true) {
                ARTable.insertOrUpdateByKey(KeysToStore[index], vectorValues[index], id, status);
                index++;

                if (index > 100000) {
                    break;
                }
            }
        };

        auto WriteKeys2 = [](ARTTupleContainer &ARTable, size_t id, std::string &status) {
            int index = 100000;
            while (true) {
                ARTable.insertOrUpdateByKey(KeysToStore[index], vectorValues[index], id, status);
                index++;

                if (index > 200000) {
                    break;
                }
            }
        };


        auto start_time = std::chrono::high_resolution_clock::now();
        Transaction<TableOperationOnTupleFunc, ARTTupleContainer> *t1 = new Transaction<TableOperationOnTupleFunc, ARTTupleContainer>(WriteKeys1, *ARTableWithTuples2);
        Transaction<TableOperationOnTupleFunc, ARTTupleContainer> *t2 = new Transaction<TableOperationOnTupleFunc, ARTTupleContainer>(WriteKeys2, *ARTableWithTuples2);

        t1->CollectTransaction();
        t2->CollectTransaction();

        auto end_time = std::chrono::high_resolution_clock::now();

        cout << "Multi 2 Thread Writer Time->";
        cout << std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time).count() << ":";
        cout << std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count() << ":";
    }

    BOOST_AUTO_TEST_CASE(testing_random_1000_keys_loaded_by_two_transactions)
    {
        cout << "testing_random_1000_keys_loaded_by_two_transactions, randomly between 0-100,000,100,000-200,000-" << endl;
        ///Iterator Operation on TupleContainer

        auto findKeys1 = [] (ARTTupleContainer& ARTWithTuples,size_t id,std::string& status)
        {
            std::vector<void *> writeSet;
            std::vector<void *> ReadSet;

            random::mt19937 rng(current_time_nanoseconds());
            random::uniform_int_distribution<> randomKeys1(0,100000);
            //cout << randomKeys(rng) << endl;

            std::cout<<"Reading 100 keys'values:: by transaction:: "<<id<<std::endl;

            for(int i=0; i < 1000; i++)
            {
                char* keysToFind = KeysToStore[randomKeys1(rng)];
                void* val = ARTWithTuples.findValueByKey(keysToFind);
                mvcc11::mvcc<RecordType>* _mvccValue = reinterpret_cast<mvcc11::mvcc<RecordType>*>(val);

                auto tp= _mvccValue->current()->value;
                BOOST_REQUIRE(tp.getAttribute<3>() == fmt::format("String/{}",keysToFind));
                std::cout<<"###found K/V = "<<tp.getAttribute<3>()<<"\n";
            }

        };


        auto findKeys2 = [] (ARTTupleContainer& ARTWithTuples,size_t id,std::string& status)
        {
            std::vector<void *> writeSet;
            std::vector<void *> ReadSet;

            random::mt19937 rng(current_time_nanoseconds());
            random::uniform_int_distribution<> randomKeys1(100000,200000);
            //cout << randomKeys(rng) << endl;

            std::cout<<"Reading 100 keys'values:: by transaction:: "<<id<<std::endl;

            for(int i=0; i < 1000; i++)
            {
                char* keysToFind = KeysToStore[randomKeys1(rng)];
                void* val = ARTWithTuples.findValueByKey(keysToFind);
                mvcc11::mvcc<RecordType>* _mvccValue = reinterpret_cast<mvcc11::mvcc<RecordType>*>(val);

                auto tp= _mvccValue->current()->value;
                BOOST_REQUIRE(tp.getAttribute<3>() == fmt::format("String/{}",keysToFind));
                std::cout<<"###found K/V = "<<tp.getAttribute<3>()<<"\n";
            }

        };

        auto start_time2 = std::chrono::high_resolution_clock::now();
        Transaction<TableOperationOnTupleFunc,ARTTupleContainer>* t1 =  new Transaction<TableOperationOnTupleFunc,ARTTupleContainer>(findKeys1,*ARTableWithTuples2);
        Transaction<TableOperationOnTupleFunc,ARTTupleContainer>* t2 =  new Transaction<TableOperationOnTupleFunc,ARTTupleContainer>(findKeys1,*ARTableWithTuples2);

        t1->CollectTransaction();
        t2->CollectTransaction();
        auto end_time2= std::chrono::high_resolution_clock::now();
        cout << std::chrono::duration_cast<std::chrono::seconds>(end_time2 - start_time2).count() << ":";
        cout << std::chrono::duration_cast<std::chrono::microseconds>(end_time2 - start_time2).count() << ":";
    }

    BOOST_AUTO_TEST_CASE(test_load_ARTIndex_MVCC_two_hundred_thousand_keys_four_transactions)
    {
        cout << "test_load_ARTIndex_MVCC_two_hundred_thousand_keys_four_transactions" << endl;

        auto WriteKeys1 = [](ARTTupleContainer &ARTable, size_t id, std::string &status) {
            int index = 0;
            while (true)
            {
                ARTable.insertOrUpdateByKey(KeysToStore[index], vectorValues[index], id, status);
                index++;

                if (index > 50000) {
                    break;
                }
            }
        };

        auto WriteKeys2 = [](ARTTupleContainer &ARTable, size_t id, std::string &status) {
            int index = 50000;
            while (true)
            {
                ARTable.insertOrUpdateByKey(KeysToStore[index], vectorValues[index], id, status);
                index++;

                if (index > 100000) {
                    break;
                }
            }
        };

        auto WriteKeys3 = [](ARTTupleContainer &ARTable, size_t id, std::string &status) {
            int index = 100000;
            while (true)
            {
                ARTable.insertOrUpdateByKey(KeysToStore[index], vectorValues[index], id, status);
                index++;

                if (index > 150000) {
                    break;
                }
            }
        };

        auto WriteKeys4 = [](ARTTupleContainer &ARTable, size_t id, std::string &status) {
            int index = 150000;
            while (true)
            {
                ARTable.insertOrUpdateByKey(KeysToStore[index], vectorValues[index], id, status);
                index++;

                if (index > 200000)
                {
                    break;
                }
            }
        };

        auto start_time = std::chrono::high_resolution_clock::now();
        Transaction<TableOperationOnTupleFunc, ARTTupleContainer> *t1 = new Transaction<TableOperationOnTupleFunc, ARTTupleContainer>(WriteKeys1, *ARTableWithTuples4);
        Transaction<TableOperationOnTupleFunc, ARTTupleContainer> *t2 = new Transaction<TableOperationOnTupleFunc, ARTTupleContainer>(WriteKeys2, *ARTableWithTuples4);
        Transaction<TableOperationOnTupleFunc, ARTTupleContainer> *t3 = new Transaction<TableOperationOnTupleFunc, ARTTupleContainer>(WriteKeys3, *ARTableWithTuples4);
        Transaction<TableOperationOnTupleFunc, ARTTupleContainer> *t4 = new Transaction<TableOperationOnTupleFunc, ARTTupleContainer>(WriteKeys4, *ARTableWithTuples4);

        t1->CollectTransaction();
        t2->CollectTransaction();
        t3->CollectTransaction();
        t4->CollectTransaction();

        auto end_time = std::chrono::high_resolution_clock::now();

        cout << "Multi 4 Thread Writers Time->";
        cout << std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time).count() << ":";
        cout << std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count() << ":";
    }

    BOOST_AUTO_TEST_CASE(testing_random_1000_keys_loaded_by_four_transactions)
    {
        cout << "testing_random_1000_keys_loaded_by_four_transactions, randomly" << endl;
        ///Iterator Operation on TupleContainer

        auto findKeys1 = [] (ARTTupleContainer& ARTWithTuples,size_t id,std::string& status)
        {
            std::vector<void *> writeSet;
            std::vector<void *> ReadSet;

            random::mt19937 rng(current_time_nanoseconds());
            random::uniform_int_distribution<> randomKeys1(0,50000);
            //cout << randomKeys(rng) << endl;

            std::cout<<"Reading 100 keys'values:: by transaction:: "<<id<<std::endl;

            for(int i=0; i < 1000; i++)
            {
                char* keysToFind = KeysToStore[randomKeys1(rng)];
                void* val = ARTWithTuples.findValueByKey(keysToFind);
                mvcc11::mvcc<RecordType>* _mvccValue = reinterpret_cast<mvcc11::mvcc<RecordType>*>(val);

                auto tp= _mvccValue->current()->value;
                BOOST_REQUIRE(tp.getAttribute<3>() == fmt::format("String/{}",keysToFind));
                std::cout<<"###found K/V = "<<tp.getAttribute<3>()<<"\n";
            }

        };


        auto findKeys2 = [] (ARTTupleContainer& ARTWithTuples,size_t id,std::string& status)
        {
            std::vector<void *> writeSet;
            std::vector<void *> ReadSet;

            random::mt19937 rng(current_time_nanoseconds());
            random::uniform_int_distribution<> randomKeys1(50000,100000);
            //cout << randomKeys(rng) << endl;

            std::cout<<"Reading 100 keys'values:: by transaction:: "<<id<<std::endl;

            for(int i=0; i < 1000; i++)
            {
                char* keysToFind = KeysToStore[randomKeys1(rng)];
                void* val = ARTWithTuples.findValueByKey(keysToFind);
                mvcc11::mvcc<RecordType>* _mvccValue = reinterpret_cast<mvcc11::mvcc<RecordType>*>(val);

                auto tp= _mvccValue->current()->value;
                BOOST_REQUIRE(tp.getAttribute<3>() == fmt::format("String/{}",keysToFind));
                std::cout<<"###found K/V = "<<tp.getAttribute<3>()<<"\n";
            }

        };

        auto findKeys3 = [] (ARTTupleContainer& ARTWithTuples,size_t id,std::string& status)
        {
            std::vector<void *> writeSet;
            std::vector<void *> ReadSet;

            random::mt19937 rng(current_time_nanoseconds());
            random::uniform_int_distribution<> randomKeys1(100000,150000);
            //cout << randomKeys(rng) << endl;

            std::cout<<"Reading 100 keys'values:: by transaction:: "<<id<<std::endl;

            for(int i=0; i < 1000; i++)
            {
                char* keysToFind = KeysToStore[randomKeys1(rng)];
                void* val = ARTWithTuples.findValueByKey(keysToFind);
                mvcc11::mvcc<RecordType>* _mvccValue = reinterpret_cast<mvcc11::mvcc<RecordType>*>(val);

                auto tp= _mvccValue->current()->value;
                BOOST_REQUIRE(tp.getAttribute<3>() == fmt::format("String/{}",keysToFind));
                std::cout<<"###found K/V = "<<tp.getAttribute<3>()<<"\n";
            }

        };


        auto findKeys4 = [] (ARTTupleContainer& ARTWithTuples,size_t id,std::string& status)
        {
            std::vector<void *> writeSet;
            std::vector<void *> ReadSet;

            random::mt19937 rng(current_time_nanoseconds());
            random::uniform_int_distribution<> randomKeys1(150000,200000);
            //cout << randomKeys(rng) << endl;

            std::cout<<"Reading 100 keys'values:: by transaction:: "<<id<<std::endl;

            for(int i=0; i < 1000; i++)
            {
                char* keysToFind = KeysToStore[randomKeys1(rng)];
                void* val = ARTWithTuples.findValueByKey(keysToFind);
                mvcc11::mvcc<RecordType>* _mvccValue = reinterpret_cast<mvcc11::mvcc<RecordType>*>(val);

                auto tp= _mvccValue->current()->value;
                BOOST_REQUIRE(tp.getAttribute<3>() == fmt::format("String/{}",keysToFind));
                std::cout<<"###found K/V = "<<tp.getAttribute<3>()<<"\n";
            }

        };

        auto start_time2 = std::chrono::high_resolution_clock::now();
        Transaction<TableOperationOnTupleFunc,ARTTupleContainer>* t1 =  new Transaction<TableOperationOnTupleFunc,ARTTupleContainer>(findKeys1,*ARTableWithTuples4);
        Transaction<TableOperationOnTupleFunc,ARTTupleContainer>* t2 =  new Transaction<TableOperationOnTupleFunc,ARTTupleContainer>(findKeys1,*ARTableWithTuples4);
        Transaction<TableOperationOnTupleFunc,ARTTupleContainer>* t3 =  new Transaction<TableOperationOnTupleFunc,ARTTupleContainer>(findKeys1,*ARTableWithTuples4);
        Transaction<TableOperationOnTupleFunc,ARTTupleContainer>* t4 =  new Transaction<TableOperationOnTupleFunc,ARTTupleContainer>(findKeys1,*ARTableWithTuples4);


        t1->CollectTransaction();
        t2->CollectTransaction();
        t3->CollectTransaction();
        t4->CollectTransaction();

        auto end_time2= std::chrono::high_resolution_clock::now();
        cout << std::chrono::duration_cast<std::chrono::seconds>(end_time2 - start_time2).count() << ":";
        cout << std::chrono::duration_cast<std::chrono::microseconds>(end_time2 - start_time2).count() << ":";
    }

    BOOST_AUTO_TEST_CASE(test_load_ARTIndex_MVCC_two_hundred_thousand_keys_eight_transactions)
    {
        cout << "test_load_ARTIndex_MVCC_two_hundred_thousand_keys_four_transactions" << endl;

        auto WriteKeys1 = [](ARTTupleContainer &ARTable, size_t id, std::string &status) {
            int index = 0;
            while (true)
            {
                ARTable.insertOrUpdateByKey(KeysToStore[index], vectorValues[index], id, status);
                index++;

                if (index > 25000) {
                    break;
                }
            }
        };

        auto WriteKeys2 = [](ARTTupleContainer &ARTable, size_t id, std::string &status) {
            int index = 25000;
            while (true)
            {
                ARTable.insertOrUpdateByKey(KeysToStore[index], vectorValues[index], id, status);
                index++;

                if (index > 50000) {
                    break;
                }
            }
        };

        auto WriteKeys3 = [](ARTTupleContainer &ARTable, size_t id, std::string &status) {
            int index = 50000;
            while (true)
            {
                ARTable.insertOrUpdateByKey(KeysToStore[index], vectorValues[index], id, status);
                index++;

                if (index > 75000) {
                    break;
                }
            }
        };

        auto WriteKeys4 = [](ARTTupleContainer &ARTable, size_t id, std::string &status) {
            int index = 75000;
            while (true)
            {
                ARTable.insertOrUpdateByKey(KeysToStore[index], vectorValues[index], id, status);
                index++;

                if (index > 100000)
                {
                    break;
                }
            }
        };

        auto WriteKeys5 = [](ARTTupleContainer &ARTable, size_t id, std::string &status) {
            int index = 100000;
            while (true)
            {
                ARTable.insertOrUpdateByKey(KeysToStore[index], vectorValues[index], id, status);
                index++;

                if (index > 125000) {
                    break;
                }
            }
        };

        auto WriteKeys6 = [](ARTTupleContainer &ARTable, size_t id, std::string &status) {
            int index = 125000;
            while (true)
            {
                ARTable.insertOrUpdateByKey(KeysToStore[index], vectorValues[index], id, status);
                index++;

                if (index > 150000) {
                    break;
                }
            }
        };

        auto WriteKeys7 = [](ARTTupleContainer &ARTable, size_t id, std::string &status) {
            int index = 150000;
            while (true)
            {
                ARTable.insertOrUpdateByKey(KeysToStore[index], vectorValues[index], id, status);
                index++;

                if (index > 175000) {
                    break;
                }
            }
        };

        auto WriteKeys8 = [](ARTTupleContainer &ARTable, size_t id, std::string &status) {
            int index = 175000;
            while (true)
            {
                ARTable.insertOrUpdateByKey(KeysToStore[index], vectorValues[index], id, status);
                index++;

                if (index > 200000)
                {
                    break;
                }
            }
        };

        auto start_time = std::chrono::high_resolution_clock::now();
        Transaction<TableOperationOnTupleFunc, ARTTupleContainer> *t1 = new Transaction<TableOperationOnTupleFunc, ARTTupleContainer>(WriteKeys1, *ARTableWithTuples8);
        Transaction<TableOperationOnTupleFunc, ARTTupleContainer> *t2 = new Transaction<TableOperationOnTupleFunc, ARTTupleContainer>(WriteKeys2, *ARTableWithTuples8);
        Transaction<TableOperationOnTupleFunc, ARTTupleContainer> *t3 = new Transaction<TableOperationOnTupleFunc, ARTTupleContainer>(WriteKeys3, *ARTableWithTuples8);
        Transaction<TableOperationOnTupleFunc, ARTTupleContainer> *t4 = new Transaction<TableOperationOnTupleFunc, ARTTupleContainer>(WriteKeys4, *ARTableWithTuples8);

        Transaction<TableOperationOnTupleFunc, ARTTupleContainer> *t5 = new Transaction<TableOperationOnTupleFunc, ARTTupleContainer>(WriteKeys5, *ARTableWithTuples8);
        Transaction<TableOperationOnTupleFunc, ARTTupleContainer> *t6 = new Transaction<TableOperationOnTupleFunc, ARTTupleContainer>(WriteKeys6, *ARTableWithTuples8);
        Transaction<TableOperationOnTupleFunc, ARTTupleContainer> *t7 = new Transaction<TableOperationOnTupleFunc, ARTTupleContainer>(WriteKeys7, *ARTableWithTuples8);
        Transaction<TableOperationOnTupleFunc, ARTTupleContainer> *t8 = new Transaction<TableOperationOnTupleFunc, ARTTupleContainer>(WriteKeys8, *ARTableWithTuples8);


        t1->CollectTransaction();
        t2->CollectTransaction();
        t3->CollectTransaction();
        t4->CollectTransaction();

        t5->CollectTransaction();
        t6->CollectTransaction();
        t7->CollectTransaction();
        t8->CollectTransaction();

        auto end_time = std::chrono::high_resolution_clock::now();

        cout << "Multi 4 Thread Writers Time->";
        cout << std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time).count() << ":";
        cout << std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count() << ":";
    }

    BOOST_AUTO_TEST_CASE(testing_random_1000_keys_loaded_by_8_transactions)
    {
        cout << "testing_random_1000_keys_loaded_by_four_transactions, randomly" << endl;
        ///Iterator Operation on TupleContainer

        auto findKeys1 = [] (ARTTupleContainer& ARTWithTuples,size_t id,std::string& status)
        {
            std::vector<void *> writeSet;
            std::vector<void *> ReadSet;

            random::mt19937 rng(current_time_nanoseconds());
            random::uniform_int_distribution<> randomKeys1(0,25000);
            //cout << randomKeys(rng) << endl;

            std::cout<<"Reading 100 keys'values:: by transaction:: "<<id<<std::endl;

            for(int i=0; i < 1000; i++)
            {
                char* keysToFind = KeysToStore[randomKeys1(rng)];
                void* val = ARTWithTuples.findValueByKey(keysToFind);
                mvcc11::mvcc<RecordType>* _mvccValue = reinterpret_cast<mvcc11::mvcc<RecordType>*>(val);

                auto tp= _mvccValue->current()->value;
                BOOST_REQUIRE(tp.getAttribute<3>() == fmt::format("String/{}",keysToFind));
                std::cout<<"###found K/V = "<<tp.getAttribute<3>()<<"\n";
            }

        };


        auto findKeys2 = [] (ARTTupleContainer& ARTWithTuples,size_t id,std::string& status)
        {
            std::vector<void *> writeSet;
            std::vector<void *> ReadSet;

            random::mt19937 rng(current_time_nanoseconds());
            random::uniform_int_distribution<> randomKeys1(25000,50000);
            //cout << randomKeys(rng) << endl;

            std::cout<<"Reading 100 keys'values:: by transaction:: "<<id<<std::endl;

            for(int i=0; i < 1000; i++)
            {
                char* keysToFind = KeysToStore[randomKeys1(rng)];
                void* val = ARTWithTuples.findValueByKey(keysToFind);
                mvcc11::mvcc<RecordType>* _mvccValue = reinterpret_cast<mvcc11::mvcc<RecordType>*>(val);

                auto tp= _mvccValue->current()->value;
                BOOST_REQUIRE(tp.getAttribute<3>() == fmt::format("String/{}",keysToFind));
                std::cout<<"###found K/V = "<<tp.getAttribute<3>()<<"\n";
            }

        };

        auto findKeys3 = [] (ARTTupleContainer& ARTWithTuples,size_t id,std::string& status)
        {
            std::vector<void *> writeSet;
            std::vector<void *> ReadSet;

            random::mt19937 rng(current_time_nanoseconds());
            random::uniform_int_distribution<> randomKeys1(50000,75000);
            //cout << randomKeys(rng) << endl;

            std::cout<<"Reading 100 keys'values:: by transaction:: "<<id<<std::endl;

            for(int i=0; i < 1000; i++)
            {
                char* keysToFind = KeysToStore[randomKeys1(rng)];
                void* val = ARTWithTuples.findValueByKey(keysToFind);
                mvcc11::mvcc<RecordType>* _mvccValue = reinterpret_cast<mvcc11::mvcc<RecordType>*>(val);

                auto tp= _mvccValue->current()->value;
                BOOST_REQUIRE(tp.getAttribute<3>() == fmt::format("String/{}",keysToFind));
                std::cout<<"###found K/V = "<<tp.getAttribute<3>()<<"\n";
            }

        };


        auto findKeys4 = [] (ARTTupleContainer& ARTWithTuples,size_t id,std::string& status)
        {
            std::vector<void *> writeSet;
            std::vector<void *> ReadSet;

            random::mt19937 rng(current_time_nanoseconds());
            random::uniform_int_distribution<> randomKeys1(75000,100000);
            //cout << randomKeys(rng) << endl;

            std::cout<<"Reading 100 keys'values:: by transaction:: "<<id<<std::endl;

            for(int i=0; i < 1000; i++)
            {
                char* keysToFind = KeysToStore[randomKeys1(rng)];
                void* val = ARTWithTuples.findValueByKey(keysToFind);
                mvcc11::mvcc<RecordType>* _mvccValue = reinterpret_cast<mvcc11::mvcc<RecordType>*>(val);

                auto tp= _mvccValue->current()->value;
                BOOST_REQUIRE(tp.getAttribute<3>() == fmt::format("String/{}",keysToFind));
                std::cout<<"###found K/V = "<<tp.getAttribute<3>()<<"\n";
            }

        };

        auto findKeys5 = [] (ARTTupleContainer& ARTWithTuples,size_t id,std::string& status)
        {
            std::vector<void *> writeSet;
            std::vector<void *> ReadSet;

            random::mt19937 rng(current_time_nanoseconds());
            random::uniform_int_distribution<> randomKeys1(100000,125000);
            //cout << randomKeys(rng) << endl;

            std::cout<<"Reading 100 keys'values:: by transaction:: "<<id<<std::endl;

            for(int i=0; i < 1000; i++)
            {
                char* keysToFind = KeysToStore[randomKeys1(rng)];
                void* val = ARTWithTuples.findValueByKey(keysToFind);
                mvcc11::mvcc<RecordType>* _mvccValue = reinterpret_cast<mvcc11::mvcc<RecordType>*>(val);

                auto tp= _mvccValue->current()->value;
                BOOST_REQUIRE(tp.getAttribute<3>() == fmt::format("String/{}",keysToFind));
                std::cout<<"###found K/V = "<<tp.getAttribute<3>()<<"\n";
            }

        };


        auto findKeys6 = [] (ARTTupleContainer& ARTWithTuples,size_t id,std::string& status)
        {
            std::vector<void *> writeSet;
            std::vector<void *> ReadSet;

            random::mt19937 rng(current_time_nanoseconds());
            random::uniform_int_distribution<> randomKeys1(125000,150000);
            //cout << randomKeys(rng) << endl;

            std::cout<<"Reading 100 keys'values:: by transaction:: "<<id<<std::endl;

            for(int i=0; i < 1000; i++)
            {
                char* keysToFind = KeysToStore[randomKeys1(rng)];
                void* val = ARTWithTuples.findValueByKey(keysToFind);
                mvcc11::mvcc<RecordType>* _mvccValue = reinterpret_cast<mvcc11::mvcc<RecordType>*>(val);

                auto tp= _mvccValue->current()->value;
                BOOST_REQUIRE(tp.getAttribute<3>() == fmt::format("String/{}",keysToFind));
                std::cout<<"###found K/V = "<<tp.getAttribute<3>()<<"\n";
            }

        };

        auto findKeys7 = [] (ARTTupleContainer& ARTWithTuples,size_t id,std::string& status)
        {
            std::vector<void *> writeSet;
            std::vector<void *> ReadSet;

            random::mt19937 rng(current_time_nanoseconds());
            random::uniform_int_distribution<> randomKeys1(150000,175000);
            //cout << randomKeys(rng) << endl;

            std::cout<<"Reading 100 keys'values:: by transaction:: "<<id<<std::endl;

            for(int i=0; i < 1000; i++)
            {
                char* keysToFind = KeysToStore[randomKeys1(rng)];
                void* val = ARTWithTuples.findValueByKey(keysToFind);
                mvcc11::mvcc<RecordType>* _mvccValue = reinterpret_cast<mvcc11::mvcc<RecordType>*>(val);

                auto tp= _mvccValue->current()->value;
                BOOST_REQUIRE(tp.getAttribute<3>() == fmt::format("String/{}",keysToFind));
                std::cout<<"###found K/V = "<<tp.getAttribute<3>()<<"\n";
            }

        };


        auto findKeys8 = [] (ARTTupleContainer& ARTWithTuples,size_t id,std::string& status)
        {
            std::vector<void *> writeSet;
            std::vector<void *> ReadSet;

            random::mt19937 rng(current_time_nanoseconds());
            random::uniform_int_distribution<> randomKeys1(175000,200000);
            //cout << randomKeys(rng) << endl;

            std::cout<<"Reading 100 keys'values:: by transaction:: "<<id<<std::endl;

            for(int i=0; i < 1000; i++)
            {
                char* keysToFind = KeysToStore[randomKeys1(rng)];
                void* val = ARTWithTuples.findValueByKey(keysToFind);
                mvcc11::mvcc<RecordType>* _mvccValue = reinterpret_cast<mvcc11::mvcc<RecordType>*>(val);

                auto tp= _mvccValue->current()->value;
                BOOST_REQUIRE(tp.getAttribute<3>() == fmt::format("String/{}",keysToFind));
                std::cout<<"###found K/V = "<<tp.getAttribute<3>()<<"\n";
            }

        };

        auto start_time2 = std::chrono::high_resolution_clock::now();
        Transaction<TableOperationOnTupleFunc,ARTTupleContainer>* t1 =  new Transaction<TableOperationOnTupleFunc,ARTTupleContainer>(findKeys1,*ARTableWithTuples8);
        Transaction<TableOperationOnTupleFunc,ARTTupleContainer>* t2 =  new Transaction<TableOperationOnTupleFunc,ARTTupleContainer>(findKeys1,*ARTableWithTuples8);
        Transaction<TableOperationOnTupleFunc,ARTTupleContainer>* t3 =  new Transaction<TableOperationOnTupleFunc,ARTTupleContainer>(findKeys1,*ARTableWithTuples8);
        Transaction<TableOperationOnTupleFunc,ARTTupleContainer>* t4 =  new Transaction<TableOperationOnTupleFunc,ARTTupleContainer>(findKeys1,*ARTableWithTuples8);

        Transaction<TableOperationOnTupleFunc,ARTTupleContainer>* t5 =  new Transaction<TableOperationOnTupleFunc,ARTTupleContainer>(findKeys1,*ARTableWithTuples8);
        Transaction<TableOperationOnTupleFunc,ARTTupleContainer>* t6 =  new Transaction<TableOperationOnTupleFunc,ARTTupleContainer>(findKeys1,*ARTableWithTuples8);
        Transaction<TableOperationOnTupleFunc,ARTTupleContainer>* t7 =  new Transaction<TableOperationOnTupleFunc,ARTTupleContainer>(findKeys1,*ARTableWithTuples8);
        Transaction<TableOperationOnTupleFunc,ARTTupleContainer>* t8 =  new Transaction<TableOperationOnTupleFunc,ARTTupleContainer>(findKeys1,*ARTableWithTuples8);


        t1->CollectTransaction();
        t2->CollectTransaction();
        t3->CollectTransaction();
        t4->CollectTransaction();
        t5->CollectTransaction();
        t6->CollectTransaction();
        t7->CollectTransaction();
        t8->CollectTransaction();

        auto end_time2= std::chrono::high_resolution_clock::now();
        cout << std::chrono::duration_cast<std::chrono::seconds>(end_time2 - start_time2).count() << ":";
        cout << std::chrono::duration_cast<std::chrono::microseconds>(end_time2 - start_time2).count() << ":";
    }


    /*
     * Testing correctness on concurrent updates
     */

    BOOST_AUTO_TEST_CASE(testing_Updates_by_8_transactions)
    {
        cout << "testing_Updates_by_transactions" << endl;

        auto WriteKeys1 = [](ARTTupleContainer &ARTable, size_t id, std::string &status)
        {
            int index = 0;
            while (true)
            {
                ARTable.insertOrUpdateByKey(KeysToStore[index], vectorValues[index], id, status);
                index++;

                if (index > 200000)
                {
                    break;
                }
            }
        };



        auto update1 = [] (ARTTupleContainer& ARTWithTuples,size_t id,std::string& status)
        {
            std::vector<void *> writeSet;
            std::vector<void *> ReadSet;

            std::function<RecordType(RecordType&)> updater = [](RecordType& record)
            {

                RecordType tuple = RecordType(record.getAttribute<0>(),
                                              record.getAttribute<1>(),
                                              record.getAttribute<2>() + 100,
                                              fmt::format("String/{}",record.getAttribute<0>()),
                                              record.getAttribute<1>() / 200.0);
                return tuple;
            };

            for(int i=0; i < 1000; i++)
            {
                char* keysToFind = KeysToStore[i];
                ARTWithTuples.insertOrUpdateByKey(keysToFind,updater,id,status);
            }
        };
        cout<<"starting new inserts"<<endl;
        Transaction<TableOperationOnTupleFunc, ARTTupleContainer> *t1 = new Transaction<TableOperationOnTupleFunc, ARTTupleContainer>(WriteKeys1, *ARTableWithTuples8);
        t1->CollectTransaction();
        cout<<"starting updates "<<endl;
        auto start_time = std::chrono::high_resolution_clock::now();
        Transaction<TableOperationOnTupleFunc, ARTTupleContainer> *t2 = new Transaction<TableOperationOnTupleFunc, ARTTupleContainer>(update1, *ARTableWithTuples8);
        auto end_time = std::chrono::high_resolution_clock::now();

        cout << std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time).count() << ":";
        cout << std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count() << ":";
    }

    BOOST_AUTO_TEST_CASE(Verifying_updates)
    {
        cout << "Verifying updates" << endl;
        ///Iterator Operation on TupleContainer
        auto findKeys1 = [] (ARTTupleContainer& ARTWithTuples,size_t id,std::string& status)
        {
            std::vector<void *> writeSet;
            std::vector<void *> ReadSet;
            for(int i=0; i < 10; i++)
            {
                char* keysToFind = KeysToStore[i];
                void* val = ARTWithTuples.findValueByKey(keysToFind);
                mvcc11::mvcc<RecordType>* _mvccValue = reinterpret_cast<mvcc11::mvcc<RecordType>*>(val);

                auto tp= _mvccValue->current()->value;

                //BOOST_REQUIRE(tp.getAttribute<3>() == fmt::format("String/{}",keysToFind));
                std::cout<<"###found K/V = "<<tp.getAttribute<2>()<<"\n";
            }

        };

        auto start_time2 = std::chrono::high_resolution_clock::now();
        Transaction<TableOperationOnTupleFunc,ARTTupleContainer>* t2 =  new Transaction<TableOperationOnTupleFunc,ARTTupleContainer>(findKeys1,*ARTableWithTuples8);

        t2->CollectTransaction();
        auto end_time2= std::chrono::high_resolution_clock::now();
        cout << std::chrono::duration_cast<std::chrono::seconds>(end_time2 - start_time2).count() << ":";
        cout << std::chrono::duration_cast<std::chrono::microseconds>(end_time2 - start_time2).count() << ":";
    }



BOOST_AUTO_TEST_SUITE_END()

