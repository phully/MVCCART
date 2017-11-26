#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE MVCC_TEST

#include <iostream>
#include <boost/test/unit_test.hpp>
#include <boost/lexical_cast.hpp>
#include "mvcc/mvcc.hpp"
#include "ART/ArtCPP.hpp"
#include "Transactions/transactionManager.h"

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
#include "generated/settings.h"

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
typedef ArtCPP<RecordType,KeyType> ARTTupleContainer;
typedef std::function <void(ARTTupleContainer&,size_t id)> TableOperationOnTupleFunc;

char KeysToStore[235890][20];
std::vector<RecordType> vectorValues;



auto ARTable_for_updates1 =  new ARTTupleContainer();
auto ARTable_for_updates2 =  new ARTTupleContainer();
auto ARTable_for_updates4 =  new ARTTupleContainer();
auto ARTable_for_updates8 =  new ARTTupleContainer();






void RESET_AND_DELETE(ARTTupleContainer& ART )
{
    ART.DestroyAdaptiveRadixTreeTable();
    reset_transaction_ID();

}

using snapshot_type = mvcc11::snapshot<RecordType>;
typedef smart_ptr::shared_ptr<snapshot_type const> const_snapshot_ptr;

static  int cb(void *data, const unsigned char* key, uint32_t key_len, const_snapshot_ptr val)
{
    if(val != NULL)
    {

        //mvcc11::mvcc<RecordType> *_mvccValue = reinterpret_cast<mvcc11::mvcc<RecordType> *>(val);
        //auto _mvccValue = val->value;
        auto tp = val->value;

        unsigned long index = tp.getAttribute<1>();
        cout<<"found val "<<tp.getAttribute<0>()<<endl;
        BOOST_TEST(tp.getAttribute<0>() == vectorValues[index].getAttribute<0>());
        BOOST_TEST(tp.getAttribute<1>() == vectorValues[index].getAttribute<1>());
        BOOST_TEST(tp.getAttribute<2>() == (vectorValues[index].getAttribute<2>()));
        //BOOST_TEST(tp.getAttribute<3>() == fmt::format("String/{}", keysToFind));
        BOOST_TEST(tp.getAttribute<4>() == (index) / 100.0);
        //BOOST_TEST(tp.getAttribute<3>() == fmt::format("String/{}", keysToFind));
        //mvcc11::mvcc<RecordType>* _mvvcValue = reinterpret_cast<mvcc11::mvcc<RecordType>*>(val);
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


char * rootpath_words = GetWordsTestFile();

BOOST_AUTO_TEST_SUITE(MVCC_TESTS)



    BOOST_AUTO_TEST_CASE(test_loading_Buckets_from_TextFIle)
    {
        cout << "loading_Buckets_from_TextFIle" << endl;
        int keyLen;
        char buf[20];

        /// Reading all Values to store against keys
        FILE *fkeys = fopen(rootpath_words, "r");
        int index = 0;
        int maxLen= 0;
        while (fgets(buf, sizeof buf, fkeys))
        {
            keyLen = strlen(buf);
            buf[keyLen] = '\0';

            if(keyLen > maxLen)
                maxLen = keyLen;

            if(buf != "")
            {
                RecordType tuple = RecordType(buf,
                                              (unsigned long) index,
                                              index + 100,
                                              fmt::format("String/{}", buf),
                                              index / 100.0);
                strcpy(KeysToStore[index],buf);
                vectorValues.push_back(tuple);
                index++;
            }
            else
            {
                cout << "empty key" << endl;
            }
            if (index > 200000)
            {
                break;
            }
        }
        cout<<"max key length in document = "<<maxLen<<endl;
    }


    /*
     * Testing correctness on concurrent Updates 100% update intensive, scaled till 8 transactions
    */



    BOOST_AUTO_TEST_CASE(testing_update_intensive_Two_transactions)
    {
        ///Writer#1 Insert/Update from Bucket
        auto WriteKeys1= [] (ARTTupleContainer& ARTable,size_t id)
        {
            int index=0;
            while (true)
            {
                ARTable.insertOrUpdateByKey(KeysToStore[index],vectorValues[index],id);
                index++;

                if(index > 200000)
                {
                    break;
                }
            }
        };

        auto start_timeWriter = std::chrono::high_resolution_clock::now();
        Transaction<TableOperationOnTupleFunc,ARTTupleContainer>* writerTransaction = new
                Transaction<TableOperationOnTupleFunc,ARTTupleContainer>(WriteKeys1,*ARTable_for_updates2);
        writerTransaction->CollectTransaction();
        auto end_timeWriter= std::chrono::high_resolution_clock::now();

        cout<<endl<<"Single Thread Writer Time->";
        cout << std::chrono::duration_cast<std::chrono::seconds>(end_timeWriter - start_timeWriter).count() << ":";
        cout << std::chrono::duration_cast<std::chrono::microseconds>(end_timeWriter - start_timeWriter).count() << ":";




        cout << "testing_update_intensive_4_transactions" << endl;

        auto update1 = [] (ARTTupleContainer& ARTWithTuples,size_t id)
        {
            std::vector<RecordType> UpdateSet;

            std::function<RecordType(RecordType&)> updater = [](RecordType& record)
            {

                string attr0 = record.getAttribute<0>();
                unsigned long attr1 = record.getAttribute<1>();
                int attr2 = record.getAttribute<2>();
                string attr3 =  fmt::format("String/{}",record.getAttribute<0>());
                double attr4 =  attr1/200.0;

                RecordType tuple = RecordType(attr0,
                                              attr1,
                                              attr2,
                                              attr3,
                                              attr4);
                return tuple;
            };

            for(int i=0; i < 100000; i++)
            {
                char* keysToFind = KeysToStore[i];
                auto result= ARTWithTuples.insertOrUpdateByKey(keysToFind,updater,id);
            }
        };

        auto update2 = [] (ARTTupleContainer& ARTWithTuples,size_t id)
        {
            std::vector<void *> writeSet;
            std::vector<void *> ReadSet;

            std::function<RecordType(RecordType&)> updater = [](RecordType& record)
            {

                string attr0 = record.getAttribute<0>();
                unsigned long attr1 = record.getAttribute<1>();
                int attr2 = record.getAttribute<2>();
                string attr3 =  fmt::format("String/{}",record.getAttribute<0>());
                double attr4 =  attr1/200.0;

                RecordType tuple = RecordType(attr0,
                                              attr1,
                                              attr2,
                                              attr3,
                                              attr4);
                return tuple;
            };

            for(int i=100000; i < 200000; i++)
            {
                char* keysToFind = KeysToStore[i];
                auto result= ARTWithTuples.insertOrUpdateByKey(keysToFind,updater,id);
                //BOOST_CHECK(result != NULL);
            }
        };



        cout<<"starting 2 updaters "<<endl;
        auto start_time = std::chrono::high_resolution_clock::now();
        Transaction<TableOperationOnTupleFunc, ARTTupleContainer> *t1 = new Transaction<TableOperationOnTupleFunc, ARTTupleContainer>(update1, *ARTable_for_updates2);
        Transaction<TableOperationOnTupleFunc, ARTTupleContainer> *t2 = new Transaction<TableOperationOnTupleFunc, ARTTupleContainer>(update2, *ARTable_for_updates2);
        t1->CollectTransaction();
        t2->CollectTransaction();
        auto end_time = std::chrono::high_resolution_clock::now();
        cout<<"Time for 2 updaters "<<endl;

        cout << std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time).count() << ":";
        cout << std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count() << ":";
    }

    BOOST_AUTO_TEST_CASE(Verifying_updates_intensive_by_Two_Transactions)
    {
        cout << "Verifying_updates_intensive_by_Two_Transactions" << endl;
        ///Iterator Operation on TupleContainer
        auto findKeys1 = [](ARTTupleContainer &ARTWithTuples, size_t id) {
            std::vector<void *> writeSet;
            std::vector<void *> ReadSet;


            random::mt19937 rng(current_time_nanoseconds());
            random::uniform_int_distribution<> randomKeys1(0,100000);
            //cout << randomKeys(rng) << endl;
            int totalCachedMissed=0;
            for (int i = 0; i < 50000; i++)
            {
                char* keysToFind = KeysToStore[randomKeys1(rng)];
                auto val = ARTWithTuples.findValueByKey(keysToFind,id);


                if(val == nullptr)
                {
                    totalCachedMissed++;
                }
                else
                {
                    auto tp = val->value;
                    unsigned long index = tp.getAttribute<1>();
                    BOOST_TEST(tp.getAttribute<0>() == vectorValues[index].getAttribute<0>());
                    BOOST_TEST(tp.getAttribute<1>() == vectorValues[index].getAttribute<1>());
                    BOOST_TEST(tp.getAttribute<2>() == (int)(vectorValues[index].getAttribute<2>()));
                    //BOOST_TEST(tp.getAttribute<3>() == fmt::format("String/{}", keysToFind));
                    BOOST_TEST(tp.getAttribute<4>() == (vectorValues[index].getAttribute<1>()) / 200.0);
                }

            }

            cout<<"Total Cached missed out of 50,000 random keys ="<<totalCachedMissed<<" by transaction#"<<id<<endl;

        };

        auto findKeys2 = [](ARTTupleContainer &ARTWithTuples, size_t id) {
            std::vector<void *> writeSet;
            std::vector<void *> ReadSet;


            random::mt19937 rng(current_time_nanoseconds());
            random::uniform_int_distribution<> randomKeys1(100000,200000);
            //cout << randomKeys(rng) << endl;
            int totalCachedMissed=0;
            for (int i = 0; i < 50000; i++)
            {
                char* keysToFind = KeysToStore[randomKeys1(rng)];
                auto val = ARTWithTuples.findValueByKey(keysToFind,id);


                if(val == nullptr)
                {
                    totalCachedMissed++;
                }
                else
                {
                    auto tp = val->value;
                    unsigned long index = tp.getAttribute<1>();
                    BOOST_TEST(tp.getAttribute<0>() == vectorValues[index].getAttribute<0>());
                    BOOST_TEST(tp.getAttribute<1>() == vectorValues[index].getAttribute<1>());
                    BOOST_TEST(tp.getAttribute<2>() == (int)(vectorValues[index].getAttribute<2>()));
                    //BOOST_TEST(tp.getAttribute<3>() == fmt::format("String/{}", keysToFind));
                    BOOST_TEST(tp.getAttribute<4>() == (vectorValues[index].getAttribute<1>()) / 200.0);
                }

            }

            cout<<"Total Cached missed out of 50,000 random keys ="<<totalCachedMissed<<" by transaction#"<<id<<endl;

        };

        auto start_time2 = std::chrono::high_resolution_clock::now();
        Transaction<TableOperationOnTupleFunc, ARTTupleContainer> *t1 = new Transaction<TableOperationOnTupleFunc, ARTTupleContainer>(
                findKeys1, *ARTable_for_updates2);
        Transaction<TableOperationOnTupleFunc, ARTTupleContainer> *t2 = new Transaction<TableOperationOnTupleFunc, ARTTupleContainer>(
                findKeys2, *ARTable_for_updates2);
        t1->CollectTransaction();
        t2->CollectTransaction();
        auto end_time2 = std::chrono::high_resolution_clock::now();


        cout << std::chrono::duration_cast<std::chrono::seconds>(end_time2 - start_time2).count() << ":";
        cout << std::chrono::duration_cast<std::chrono::microseconds>(end_time2 - start_time2).count() << ":";

        RESET_AND_DELETE(*ARTable_for_updates2);

    }


    BOOST_AUTO_TEST_CASE(testing_update_intensive_Four_transactions)
    {


        ///Writer#1 Insert/Update from Bucket
        auto WriteKeys1= [] (ARTTupleContainer& ARTable,size_t id)
        {
            int index=0;
            while (true)
            {
                ARTable.insertOrUpdateByKey(KeysToStore[index],vectorValues[index],id);
                index++;

                if(index > 200000)
                {
                    break;
                }
            }
        };

        auto start_timeWriter = std::chrono::high_resolution_clock::now();
        Transaction<TableOperationOnTupleFunc,ARTTupleContainer>* writerTransaction = new Transaction<TableOperationOnTupleFunc,ARTTupleContainer>(WriteKeys1,*ARTable_for_updates4);
        writerTransaction->CollectTransaction();
        auto end_timeWriter= std::chrono::high_resolution_clock::now();

        cout<<endl<<"Single Thread Writer Time->";
        cout << std::chrono::duration_cast<std::chrono::seconds>(end_timeWriter - start_timeWriter).count() << ":";
        cout << std::chrono::duration_cast<std::chrono::microseconds>(end_timeWriter - start_timeWriter).count() << ":";




        cout << "testing_update_intensive_4_transactions" << endl;

        auto update1 = [] (ARTTupleContainer& ARTWithTuples,size_t id)
        {
            std::vector<RecordType> UpdateSet;

            std::function<RecordType(RecordType&)> updater = [](RecordType& record)
            {

                string attr0 = record.getAttribute<0>();
                unsigned long attr1 = record.getAttribute<1>();
                int attr2 = record.getAttribute<2>();
                string attr3 =  fmt::format("String/{}",record.getAttribute<0>());
                double attr4 =  attr1/200.0;

                RecordType tuple = RecordType(attr0,
                                              attr1,
                                              attr2,
                                              attr3,
                                              attr4);
                return tuple;
            };

            for(int i=0; i < 50000; i++)
            {
                char* keysToFind = KeysToStore[i];
                auto result= ARTWithTuples.insertOrUpdateByKey(keysToFind,updater,id);
            }
        };

        auto update2 = [] (ARTTupleContainer& ARTWithTuples,size_t id)
        {
            std::vector<void *> writeSet;
            std::vector<void *> ReadSet;

            std::function<RecordType(RecordType&)> updater = [](RecordType& record)
            {

                string attr0 = record.getAttribute<0>();
                unsigned long attr1 = record.getAttribute<1>();
                int attr2 = record.getAttribute<2>();
                string attr3 =  fmt::format("String/{}",record.getAttribute<0>());
                double attr4 =  attr1/200.0;

                RecordType tuple = RecordType(attr0,
                                              attr1,
                                              attr2,
                                              attr3,
                                              attr4);
                return tuple;
            };

            for(int i=50000; i < 100000; i++)
            {
                char* keysToFind = KeysToStore[i];
                auto result= ARTWithTuples.insertOrUpdateByKey(keysToFind,updater,id);
                //BOOST_CHECK(result != NULL);
            }
        };

        auto update3 = [] (ARTTupleContainer& ARTWithTuples,size_t id)
        {
            std::vector<void *> writeSet;
            std::vector<void *> ReadSet;

            std::function<RecordType(RecordType&)> updater = [](RecordType& record)
            {

                string attr0 = record.getAttribute<0>();
                unsigned long attr1 = record.getAttribute<1>();
                int attr2 = record.getAttribute<2>();
                string attr3 =  fmt::format("String/{}",record.getAttribute<0>());
                double attr4 =  attr1/200.0;

                RecordType tuple = RecordType(attr0,
                                              attr1,
                                              attr2,
                                              attr3,
                                              attr4);
                return tuple;
            };

            for(int i=100000; i < 150000; i++)
            {
                char* keysToFind = KeysToStore[i];
                auto result= ARTWithTuples.insertOrUpdateByKey(keysToFind,updater,id);
                //BOOST_CHECK(result != NULL);
            }
        };


        auto update4 = [] (ARTTupleContainer& ARTWithTuples,size_t id)
        {
            std::vector<void *> writeSet;
            std::vector<void *> ReadSet;

            std::function<RecordType(RecordType&)> updater = [](RecordType& record)
            {

                string attr0 = record.getAttribute<0>();
                unsigned long attr1 = record.getAttribute<1>();
                int attr2 = record.getAttribute<2>();
                string attr3 =  fmt::format("String/{}",record.getAttribute<0>());
                double attr4 =  attr1/200.0;

                RecordType tuple = RecordType(attr0,
                                              attr1,
                                              attr2,
                                              attr3,
                                              attr4);
                return tuple;
            };

            for(int i=150000; i < 200000; i++)
            {
                char* keysToFind = KeysToStore[i];
                auto result= ARTWithTuples.insertOrUpdateByKey(keysToFind,updater,id);
                //BOOST_CHECK(result != NULL);
            }
        };


        cout<<"starting 8 updaters "<<endl;
        auto start_time = std::chrono::high_resolution_clock::now();
        Transaction<TableOperationOnTupleFunc, ARTTupleContainer> *t1 = new Transaction<TableOperationOnTupleFunc, ARTTupleContainer>(update1, *ARTable_for_updates4);
        Transaction<TableOperationOnTupleFunc, ARTTupleContainer> *t2 = new Transaction<TableOperationOnTupleFunc, ARTTupleContainer>(update2, *ARTable_for_updates4);
        Transaction<TableOperationOnTupleFunc, ARTTupleContainer> *t3 = new Transaction<TableOperationOnTupleFunc, ARTTupleContainer>(update3, *ARTable_for_updates4);
        Transaction<TableOperationOnTupleFunc, ARTTupleContainer> *t4 = new Transaction<TableOperationOnTupleFunc, ARTTupleContainer>(update4, *ARTable_for_updates4);


        t1->CollectTransaction();
        t2->CollectTransaction();
        t3->CollectTransaction();
        t4->CollectTransaction();

        auto end_time = std::chrono::high_resolution_clock::now();
        cout<<"4 Updater Time::  "<<endl;

        cout << std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time).count() << ":";
        cout << std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count() << ":";
    }

    BOOST_AUTO_TEST_CASE(Verifying_updates_intensive_by_Four_Transactions)
    {
        cout << "Verifying_updates_intensive_by_Four_Transactions" << endl;
        ///Iterator Operation on TupleContainer

        auto findKeys1 = [](ARTTupleContainer &ARTWithTuples, size_t id) {
            std::vector<void *> writeSet;
            std::vector<void *> ReadSet;


            random::mt19937 rng(current_time_nanoseconds());
            random::uniform_int_distribution<> randomKeys1(0,50000);
            //cout << randomKeys(rng) << endl;
            int totalCachedMissed=0;
            for (int i = 0; i < 50000; i++)
            {
                char* keysToFind = KeysToStore[randomKeys1(rng)];
                auto val = ARTWithTuples.findValueByKey(keysToFind,id);


                if(val == nullptr)
                {
                    totalCachedMissed++;
                }
                else
                {
                    auto tp = val->value;
                    unsigned long index = tp.getAttribute<1>();
                    BOOST_TEST(tp.getAttribute<0>() == vectorValues[index].getAttribute<0>());
                    BOOST_TEST(tp.getAttribute<1>() == vectorValues[index].getAttribute<1>());
                    BOOST_TEST(tp.getAttribute<2>() == (int)(vectorValues[index].getAttribute<2>()));
                    //BOOST_TEST(tp.getAttribute<3>() == fmt::format("String/{}", keysToFind));
                    BOOST_TEST(tp.getAttribute<4>() == (vectorValues[index].getAttribute<1>()) / 200.0);
                }

            }

            cout<<"Total Cached missed out of 50,000 random keys ="<<totalCachedMissed<<" by transaction#"<<id<<endl;

        };

        auto findKeys2 = [](ARTTupleContainer &ARTWithTuples, size_t id) {
            std::vector<void *> writeSet;
            std::vector<void *> ReadSet;


            random::mt19937 rng(current_time_nanoseconds());
            random::uniform_int_distribution<> randomKeys1(50000,100000);
            //cout << randomKeys(rng) << endl;
            int totalCachedMissed=0;
            for (int i = 0; i < 50000; i++)
            {
                char* keysToFind = KeysToStore[randomKeys1(rng)];
                auto val = ARTWithTuples.findValueByKey(keysToFind,id);


                if(val == nullptr)
                {
                    totalCachedMissed++;
                }
                else
                {
                    auto tp = val->value;
                    unsigned long index = tp.getAttribute<1>();
                    BOOST_TEST(tp.getAttribute<0>() == vectorValues[index].getAttribute<0>());
                    BOOST_TEST(tp.getAttribute<1>() == vectorValues[index].getAttribute<1>());
                    BOOST_TEST(tp.getAttribute<2>() == (int)(vectorValues[index].getAttribute<2>()));
                    //BOOST_TEST(tp.getAttribute<3>() == fmt::format("String/{}", keysToFind));
                    BOOST_TEST(tp.getAttribute<4>() == (vectorValues[index].getAttribute<1>()) / 200.0);
                }

            }

            cout<<"Total Cached missed out of 50,000 random keys ="<<totalCachedMissed<<" by transaction#"<<id<<endl;

        };

        auto findKeys3 = [](ARTTupleContainer &ARTWithTuples, size_t id) {
            std::vector<void *> writeSet;
            std::vector<void *> ReadSet;


            random::mt19937 rng(current_time_nanoseconds());
            random::uniform_int_distribution<> randomKeys1(100000,150000);
            //cout << randomKeys(rng) << endl;
            int totalCachedMissed=0;
            for (int i = 0; i < 50000; i++)
            {
                char* keysToFind = KeysToStore[randomKeys1(rng)];
                auto val = ARTWithTuples.findValueByKey(keysToFind,id);


                if(val == nullptr)
                {
                    totalCachedMissed++;
                }
                else
                {
                    auto tp = val->value;
                    unsigned long index = tp.getAttribute<1>();
                    BOOST_TEST(tp.getAttribute<0>() == vectorValues[index].getAttribute<0>());
                    BOOST_TEST(tp.getAttribute<1>() == vectorValues[index].getAttribute<1>());
                    BOOST_TEST(tp.getAttribute<2>() == (int)(vectorValues[index].getAttribute<2>()));
                    //BOOST_TEST(tp.getAttribute<3>() == fmt::format("String/{}", keysToFind));
                    BOOST_TEST(tp.getAttribute<4>() == (vectorValues[index].getAttribute<1>()) / 200.0);
                }

            }

            cout<<"Total Cached missed out of 50,000 random keys ="<<totalCachedMissed<<" by transaction#"<<id<<endl;

        };



        auto findKeys4 = [](ARTTupleContainer &ARTWithTuples, size_t id) {
            std::vector<void *> writeSet;
            std::vector<void *> ReadSet;


            random::mt19937 rng(current_time_nanoseconds());
            random::uniform_int_distribution<> randomKeys1(150000,200000);
            //cout << randomKeys(rng) << endl;
            int totalCachedMissed=0;
            for (int i = 0; i < 50000; i++)
            {
                char* keysToFind = KeysToStore[randomKeys1(rng)];
                auto val = ARTWithTuples.findValueByKey(keysToFind,id);


                if(val == nullptr)
                {
                    totalCachedMissed++;
                }
                else
                {
                    auto tp = val->value;
                    unsigned long index = tp.getAttribute<1>();
                    BOOST_TEST(tp.getAttribute<0>() == vectorValues[index].getAttribute<0>());
                    BOOST_TEST(tp.getAttribute<1>() == vectorValues[index].getAttribute<1>());
                    BOOST_TEST(tp.getAttribute<2>() == (int)(vectorValues[index].getAttribute<2>()));
                    //BOOST_TEST(tp.getAttribute<3>() == fmt::format("String/{}", keysToFind));
                    BOOST_TEST(tp.getAttribute<4>() == (vectorValues[index].getAttribute<1>()) / 200.0);
                }

            }

            cout<<"Total Cached missed out of 50,000 random keys ="<<totalCachedMissed<<" by transaction#"<<id<<endl;

        };



        auto start_time2 = std::chrono::high_resolution_clock::now();
        Transaction<TableOperationOnTupleFunc, ARTTupleContainer> *t1 = new Transaction<TableOperationOnTupleFunc, ARTTupleContainer>(
                findKeys1, *ARTable_for_updates4);
        Transaction<TableOperationOnTupleFunc, ARTTupleContainer> *t2 = new Transaction<TableOperationOnTupleFunc, ARTTupleContainer>(
                findKeys1, *ARTable_for_updates4);
        Transaction<TableOperationOnTupleFunc, ARTTupleContainer> *t3 = new Transaction<TableOperationOnTupleFunc, ARTTupleContainer>(
                findKeys1, *ARTable_for_updates4);
        Transaction<TableOperationOnTupleFunc, ARTTupleContainer> *t4 = new Transaction<TableOperationOnTupleFunc, ARTTupleContainer>(
                findKeys1, *ARTable_for_updates4);
        t1->CollectTransaction();
        t2->CollectTransaction();
        t3->CollectTransaction();
        t4->CollectTransaction();

        auto end_time2 = std::chrono::high_resolution_clock::now();

        cout<<"Total time by 4 evaluation transactions::"<<endl;
        cout << std::chrono::duration_cast<std::chrono::seconds>(end_time2 - start_time2).count() << ":";
        cout << std::chrono::duration_cast<std::chrono::microseconds>(end_time2 - start_time2).count() << ":";

        RESET_AND_DELETE(*ARTable_for_updates4);


    }


    BOOST_AUTO_TEST_CASE(testing_update_intensive_Eight_transactions)
    {


        ///Writer#1 Insert/Update from Bucket
        auto WriteKeys1= [] (ARTTupleContainer& ARTable,size_t id)
        {
            int index=0;
            while (true)
            {
                ARTable.insertOrUpdateByKey(KeysToStore[index],vectorValues[index],id);
                index++;

                if(index > 200000)
                {
                    break;
                }
            }
        };

        auto start_timeWriter = std::chrono::high_resolution_clock::now();
        Transaction<TableOperationOnTupleFunc,ARTTupleContainer>* writerTransaction = new Transaction<TableOperationOnTupleFunc,ARTTupleContainer>(WriteKeys1,*ARTable_for_updates8);
        writerTransaction->CollectTransaction();
        auto end_timeWriter= std::chrono::high_resolution_clock::now();

        cout<<endl<<"Single Thread Writer Time->";
        cout << std::chrono::duration_cast<std::chrono::seconds>(end_timeWriter - start_timeWriter).count() << ":";
        cout << std::chrono::duration_cast<std::chrono::microseconds>(end_timeWriter - start_timeWriter).count() << ":";




        cout << "testing_update_intensive_single_transaction" << endl;

        auto update1 = [] (ARTTupleContainer& ARTWithTuples,size_t id)
        {
            std::vector<RecordType> UpdateSet;

            std::function<RecordType(RecordType&)> updater = [](RecordType& record)
            {

                string attr0 = record.getAttribute<0>();
                unsigned long attr1 = record.getAttribute<1>();
                int attr2 = record.getAttribute<2>();
                string attr3 =  fmt::format("String/{}",record.getAttribute<0>());
                double attr4 =  attr1/200.0;

                RecordType tuple = RecordType(attr0,
                                              attr1,
                                              attr2,
                                              attr3,
                                              attr4);
                return tuple;
            };

            for(int i=0; i < 25000; i++)
            {
                char* keysToFind = KeysToStore[i];
                auto result= ARTWithTuples.insertOrUpdateByKey(keysToFind,updater,id);
            }
        };

        auto update2 = [] (ARTTupleContainer& ARTWithTuples,size_t id)
        {
            std::vector<void *> writeSet;
            std::vector<void *> ReadSet;

            std::function<RecordType(RecordType&)> updater = [](RecordType& record)
            {

                string attr0 = record.getAttribute<0>();
                unsigned long attr1 = record.getAttribute<1>();
                int attr2 = record.getAttribute<2>();
                string attr3 =  fmt::format("String/{}",record.getAttribute<0>());
                double attr4 =  attr1/200.0;

                RecordType tuple = RecordType(attr0,
                                              attr1,
                                              attr2,
                                              attr3,
                                              attr4);
                return tuple;
            };

            for(int i=25000; i < 50000; i++)
            {
                char* keysToFind = KeysToStore[i];
                auto result= ARTWithTuples.insertOrUpdateByKey(keysToFind,updater,id);
                //BOOST_CHECK(result != NULL);
            }
        };

        auto update3 = [] (ARTTupleContainer& ARTWithTuples,size_t id)
        {
            std::vector<void *> writeSet;
            std::vector<void *> ReadSet;

            std::function<RecordType(RecordType&)> updater = [](RecordType& record)
            {

                string attr0 = record.getAttribute<0>();
                unsigned long attr1 = record.getAttribute<1>();
                int attr2 = record.getAttribute<2>();
                string attr3 =  fmt::format("String/{}",record.getAttribute<0>());
                double attr4 =  attr1/200.0;

                RecordType tuple = RecordType(attr0,
                                              attr1,
                                              attr2,
                                              attr3,
                                              attr4);
                return tuple;
            };

            for(int i=50000; i < 75000; i++)
            {
                char* keysToFind = KeysToStore[i];
                auto result= ARTWithTuples.insertOrUpdateByKey(keysToFind,updater,id);
                //BOOST_CHECK(result != NULL);
            }
        };


        auto update4 = [] (ARTTupleContainer& ARTWithTuples,size_t id)
        {
            std::vector<void *> writeSet;
            std::vector<void *> ReadSet;

            std::function<RecordType(RecordType&)> updater = [](RecordType& record)
            {

                string attr0 = record.getAttribute<0>();
                unsigned long attr1 = record.getAttribute<1>();
                int attr2 = record.getAttribute<2>();
                string attr3 =  fmt::format("String/{}",record.getAttribute<0>());
                double attr4 =  attr1/200.0;

                RecordType tuple = RecordType(attr0,
                                              attr1,
                                              attr2,
                                              attr3,
                                              attr4);
                return tuple;
            };

            for(int i=75000; i < 100000; i++)
            {
                char* keysToFind = KeysToStore[i];
                auto result= ARTWithTuples.insertOrUpdateByKey(keysToFind,updater,id);
                //BOOST_CHECK(result != NULL);
            }
        };

        auto update5 = [] (ARTTupleContainer& ARTWithTuples,size_t id)
        {
            std::vector<void *> writeSet;
            std::vector<void *> ReadSet;

            std::function<RecordType(RecordType&)> updater = [](RecordType& record)
            {

                string attr0 = record.getAttribute<0>();
                unsigned long attr1 = record.getAttribute<1>();
                int attr2 = record.getAttribute<2>();
                string attr3 =  fmt::format("String/{}",record.getAttribute<0>());
                double attr4 =  attr1/200.0;

                RecordType tuple = RecordType(attr0,
                                              attr1,
                                              attr2,
                                              attr3,
                                              attr4);
                return tuple;
            };

            for(int i=100000; i < 125000; i++)
            {
                char* keysToFind = KeysToStore[i];
                auto result= ARTWithTuples.insertOrUpdateByKey(keysToFind,updater,id);
                //BOOST_CHECK(result != NULL);
            }
        };

        auto update6 = [] (ARTTupleContainer& ARTWithTuples,size_t id)
        {
            std::vector<void *> writeSet;
            std::vector<void *> ReadSet;

            std::function<RecordType(RecordType&)> updater = [](RecordType& record)
            {

                string attr0 = record.getAttribute<0>();
                unsigned long attr1 = record.getAttribute<1>();
                int attr2 = record.getAttribute<2>();
                string attr3 =  fmt::format("String/{}",record.getAttribute<0>());
                double attr4 =  attr1/200.0;

                RecordType tuple = RecordType(attr0,
                                              attr1,
                                              attr2,
                                              attr3,
                                              attr4);
                return tuple;
            };

            for(int i=125000; i < 150000; i++)
            {
                char* keysToFind = KeysToStore[i];
                auto result= ARTWithTuples.insertOrUpdateByKey(keysToFind,updater,id);
                //BOOST_CHECK(result != NULL);
            }
        };

        auto update7 = [] (ARTTupleContainer& ARTWithTuples,size_t id)
        {
            std::vector<void *> writeSet;
            std::vector<void *> ReadSet;

            std::function<RecordType(RecordType&)> updater = [](RecordType& record)
            {

                string attr0 = record.getAttribute<0>();
                unsigned long attr1 = record.getAttribute<1>();
                int attr2 = record.getAttribute<2>();
                string attr3 =  fmt::format("String/{}",record.getAttribute<0>());
                double attr4 =  attr1/200.0;

                RecordType tuple = RecordType(attr0,
                                              attr1,
                                              attr2,
                                              attr3,
                                              attr4);
                return tuple;
            };

            for(int i=150000; i < 175000; i++)
            {
                char* keysToFind = KeysToStore[i];
                auto result= ARTWithTuples.insertOrUpdateByKey(keysToFind,updater,id);
                //BOOST_CHECK(result != NULL);

            }
        };

        auto update8 = [] (ARTTupleContainer& ARTWithTuples,size_t id)
        {
            std::vector<void *> writeSet;
            std::vector<void *> ReadSet;

            std::function<RecordType(RecordType&)> updater = [](RecordType& record)
            {

                string attr0 = record.getAttribute<0>();
                unsigned long attr1 = record.getAttribute<1>();
                int attr2 = record.getAttribute<2>();
                string attr3 =  fmt::format("String/{}",record.getAttribute<0>());
                double attr4 =  attr1/200.0;

                RecordType tuple = RecordType(attr0,
                                              attr1,
                                              attr2,
                                              attr3,
                                              attr4);
                return tuple;
            };

            for(int i=175000; i < 200000; i++)
            {
                char* keysToFind = KeysToStore[i];
                auto result= ARTWithTuples.insertOrUpdateByKey(keysToFind,updater,id);
                //BOOST_CHECK(result != NULL);
            }
        };

        cout<<"starting 8 updaters "<<endl;
        auto start_time = std::chrono::high_resolution_clock::now();
        Transaction<TableOperationOnTupleFunc, ARTTupleContainer> *t1 = new Transaction<TableOperationOnTupleFunc, ARTTupleContainer>(update1, *ARTable_for_updates8);
        Transaction<TableOperationOnTupleFunc, ARTTupleContainer> *t2 = new Transaction<TableOperationOnTupleFunc, ARTTupleContainer>(update2, *ARTable_for_updates8);
        Transaction<TableOperationOnTupleFunc, ARTTupleContainer> *t3 = new Transaction<TableOperationOnTupleFunc, ARTTupleContainer>(update3, *ARTable_for_updates8);
        Transaction<TableOperationOnTupleFunc, ARTTupleContainer> *t4 = new Transaction<TableOperationOnTupleFunc, ARTTupleContainer>(update4, *ARTable_for_updates8);
        Transaction<TableOperationOnTupleFunc, ARTTupleContainer> *t5 = new Transaction<TableOperationOnTupleFunc, ARTTupleContainer>(update5, *ARTable_for_updates8);
        Transaction<TableOperationOnTupleFunc, ARTTupleContainer> *t6 = new Transaction<TableOperationOnTupleFunc, ARTTupleContainer>(update6, *ARTable_for_updates8);
        Transaction<TableOperationOnTupleFunc, ARTTupleContainer> *t7 = new Transaction<TableOperationOnTupleFunc, ARTTupleContainer>(update7, *ARTable_for_updates8);
        Transaction<TableOperationOnTupleFunc, ARTTupleContainer> *t8 = new Transaction<TableOperationOnTupleFunc, ARTTupleContainer>(update8, *ARTable_for_updates8);


        t1->CollectTransaction();
        t2->CollectTransaction();
        t3->CollectTransaction();
        t4->CollectTransaction();
        t5->CollectTransaction();
        t6->CollectTransaction();
        t7->CollectTransaction();
        t8->CollectTransaction();

        auto end_time = std::chrono::high_resolution_clock::now();
        cout << std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time).count() << ":";
        cout << std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count() << ":";
    }

    BOOST_AUTO_TEST_CASE(Verifying_updates_intensive_by_Eight_Transactions)
    {
        cout << "Verifying updates" << endl;
        ///Iterator Operation on TupleContainer
        auto findKeys1 = [](ARTTupleContainer &ARTWithTuples, size_t id) {
            std::vector<void *> writeSet;
            std::vector<void *> ReadSet;


            random::mt19937 rng(current_time_nanoseconds());
            random::uniform_int_distribution<> randomKeys1(0,25000);
            //cout << randomKeys(rng) << endl;
            int totalCachedMissed=0;
            for (int i = 0; i < 25000; i++)
            {
                char* keysToFind = KeysToStore[randomKeys1(rng)];
                auto val = ARTWithTuples.findValueByKey(keysToFind,id);


                if(val == nullptr)
                {
                    totalCachedMissed++;
                }
                else
                {
                    auto tp = val->value;
                    unsigned long index = tp.getAttribute<1>();
                    BOOST_TEST(tp.getAttribute<0>() == vectorValues[index].getAttribute<0>());
                    BOOST_TEST(tp.getAttribute<1>() == vectorValues[index].getAttribute<1>());
                    BOOST_TEST(tp.getAttribute<2>() == (int)(vectorValues[index].getAttribute<2>()));
                    //BOOST_TEST(tp.getAttribute<3>() == fmt::format("String/{}", keysToFind));
                    BOOST_TEST(tp.getAttribute<4>() == (vectorValues[index].getAttribute<1>()) / 200.0);
                }

            }

            cout<<"Total Cached missed out of 50,000 random keys ="<<totalCachedMissed<<" by transaction#"<<id<<endl;

        };

        auto findKeys2 = [](ARTTupleContainer &ARTWithTuples, size_t id) {
            std::vector<void *> writeSet;
            std::vector<void *> ReadSet;


            random::mt19937 rng(current_time_nanoseconds());
            random::uniform_int_distribution<> randomKeys1(25000,50000);
            //cout << randomKeys(rng) << endl;
            int totalCachedMissed=0;
            for (int i = 0; i < 25000; i++)
            {
                char* keysToFind = KeysToStore[randomKeys1(rng)];
                auto val = ARTWithTuples.findValueByKey(keysToFind,id);


                if(val == nullptr)
                {
                    totalCachedMissed++;
                }
                else
                {
                    auto tp = val->value;
                    unsigned long index = tp.getAttribute<1>();
                    BOOST_TEST(tp.getAttribute<0>() == vectorValues[index].getAttribute<0>());
                    BOOST_TEST(tp.getAttribute<1>() == vectorValues[index].getAttribute<1>());
                    BOOST_TEST(tp.getAttribute<2>() == (int)(vectorValues[index].getAttribute<2>()));
                    //BOOST_TEST(tp.getAttribute<3>() == fmt::format("String/{}", keysToFind));
                    BOOST_TEST(tp.getAttribute<4>() == (vectorValues[index].getAttribute<1>()) / 200.0);
                }
            }

            cout<<"Total Cached missed out of 50,000 random keys ="<<totalCachedMissed<<" by transaction#"<<id<<endl;

        };


        auto findKeys3 = [](ARTTupleContainer &ARTWithTuples, size_t id) {
            std::vector<void *> writeSet;
            std::vector<void *> ReadSet;


            random::mt19937 rng(current_time_nanoseconds());
            random::uniform_int_distribution<> randomKeys1(50000,75000);
            //cout << randomKeys(rng) << endl;
            int totalCachedMissed=0;
            for (int i = 0; i < 25000; i++)
            {
                char* keysToFind = KeysToStore[randomKeys1(rng)];
                auto val = ARTWithTuples.findValueByKey(keysToFind,id);


                if(val == nullptr)
                {
                    totalCachedMissed++;
                }
                else
                {
                    auto tp = val->value;
                    unsigned long index = tp.getAttribute<1>();
                    BOOST_TEST(tp.getAttribute<0>() == vectorValues[index].getAttribute<0>());
                    BOOST_TEST(tp.getAttribute<1>() == vectorValues[index].getAttribute<1>());
                    BOOST_TEST(tp.getAttribute<2>() == (int)(vectorValues[index].getAttribute<2>()));
                    //BOOST_TEST(tp.getAttribute<3>() == fmt::format("String/{}", keysToFind));
                    BOOST_TEST(tp.getAttribute<4>() == (vectorValues[index].getAttribute<1>()) / 200.0);
                }

            }

            cout<<"Total Cached missed out of 50,000 random keys ="<<totalCachedMissed<<" by transaction#"<<id<<endl;

        };

        auto findKeys4 = [](ARTTupleContainer &ARTWithTuples, size_t id) {
            std::vector<void *> writeSet;
            std::vector<void *> ReadSet;


            random::mt19937 rng(current_time_nanoseconds());
            random::uniform_int_distribution<> randomKeys1(75000,100000);
            //cout << randomKeys(rng) << endl;
            int totalCachedMissed=0;
            for (int i = 0; i < 25000; i++)
            {
                char* keysToFind = KeysToStore[randomKeys1(rng)];
                auto val = ARTWithTuples.findValueByKey(keysToFind,id);


                if(val == nullptr)
                {
                    totalCachedMissed++;
                }
                else
                {
                    auto tp = val->value;
                    unsigned long index = tp.getAttribute<1>();
                    BOOST_TEST(tp.getAttribute<0>() == vectorValues[index].getAttribute<0>());
                    BOOST_TEST(tp.getAttribute<1>() == vectorValues[index].getAttribute<1>());
                    BOOST_TEST(tp.getAttribute<2>() == (int)(vectorValues[index].getAttribute<2>()));
                    //BOOST_TEST(tp.getAttribute<3>() == fmt::format("String/{}", keysToFind));
                    BOOST_TEST(tp.getAttribute<4>() == (vectorValues[index].getAttribute<1>()) / 200.0);
                }

            }

            cout<<"Total Cached missed out of 50,000 random keys ="<<totalCachedMissed<<" by transaction#"<<id<<endl;

        };

        auto findKeys5 = [](ARTTupleContainer &ARTWithTuples, size_t id) {
            std::vector<void *> writeSet;
            std::vector<void *> ReadSet;


            random::mt19937 rng(current_time_nanoseconds());
            random::uniform_int_distribution<> randomKeys1(100000,125000);
            //cout << randomKeys(rng) << endl;
            int totalCachedMissed=0;
            for (int i = 0; i < 25000; i++)
            {
                char* keysToFind = KeysToStore[randomKeys1(rng)];
                auto val = ARTWithTuples.findValueByKey(keysToFind,id);


                if(val == nullptr)
                {
                    totalCachedMissed++;
                }
                else
                {
                    auto tp = val->value;
                    unsigned long index = tp.getAttribute<1>();
                    BOOST_TEST(tp.getAttribute<0>() == vectorValues[index].getAttribute<0>());
                    BOOST_TEST(tp.getAttribute<1>() == vectorValues[index].getAttribute<1>());
                    BOOST_TEST(tp.getAttribute<2>() == (int)(vectorValues[index].getAttribute<2>()));
                    //BOOST_TEST(tp.getAttribute<3>() == fmt::format("String/{}", keysToFind));
                    BOOST_TEST(tp.getAttribute<4>() == (vectorValues[index].getAttribute<1>()) / 200.0);
                }

            }

            cout<<"Total Cached missed out of 50,000 random keys ="<<totalCachedMissed<<" by transaction#"<<id<<endl;

        };

        auto findKeys6 = [](ARTTupleContainer &ARTWithTuples, size_t id) {
            std::vector<void *> writeSet;
            std::vector<void *> ReadSet;


            random::mt19937 rng(current_time_nanoseconds());
            random::uniform_int_distribution<> randomKeys1(125000,1500000);
            //cout << randomKeys(rng) << endl;
            int totalCachedMissed=0;
            for (int i = 0; i < 25000; i++)
            {
                char* keysToFind = KeysToStore[randomKeys1(rng)];
                auto val = ARTWithTuples.findValueByKey(keysToFind,id);


                if(val == nullptr)
                {
                    totalCachedMissed++;
                }
                else
                {
                    auto tp = val->value;
                    unsigned long index = tp.getAttribute<1>();
                    BOOST_TEST(tp.getAttribute<0>() == vectorValues[index].getAttribute<0>());
                    BOOST_TEST(tp.getAttribute<1>() == vectorValues[index].getAttribute<1>());
                    BOOST_TEST(tp.getAttribute<2>() == (int)(vectorValues[index].getAttribute<2>()));
                    //BOOST_TEST(tp.getAttribute<3>() == fmt::format("String/{}", keysToFind));
                    BOOST_TEST(tp.getAttribute<4>() == (vectorValues[index].getAttribute<1>()) / 200.0);
                }

            }

            cout<<"Total Cached missed out of 50,000 random keys ="<<totalCachedMissed<<" by transaction#"<<id<<endl;

        };

        auto findKeys7 = [](ARTTupleContainer &ARTWithTuples, size_t id) {
            std::vector<void *> writeSet;
            std::vector<void *> ReadSet;


            random::mt19937 rng(current_time_nanoseconds());
            random::uniform_int_distribution<> randomKeys1(150000,175000);
            //cout << randomKeys(rng) << endl;
            int totalCachedMissed=0;
            for (int i = 0; i < 25000; i++)
            {
                char* keysToFind = KeysToStore[randomKeys1(rng)];
                auto val = ARTWithTuples.findValueByKey(keysToFind,id);


                if(val == nullptr)
                {
                    totalCachedMissed++;
                }
                else
                {
                    auto tp = val->value;
                    unsigned long index = tp.getAttribute<1>();
                    BOOST_TEST(tp.getAttribute<0>() == vectorValues[index].getAttribute<0>());
                    BOOST_TEST(tp.getAttribute<1>() == vectorValues[index].getAttribute<1>());
                    BOOST_TEST(tp.getAttribute<2>() == (int)(vectorValues[index].getAttribute<2>()));
                    //BOOST_TEST(tp.getAttribute<3>() == fmt::format("String/{}", keysToFind));
                    BOOST_TEST(tp.getAttribute<4>() == (vectorValues[index].getAttribute<1>()) / 200.0);
                }

            }

            cout<<"Total Cached missed out of 50,000 random keys ="<<totalCachedMissed<<" by transaction#"<<id<<endl;

        };


        auto findKeys8 = [](ARTTupleContainer &ARTWithTuples, size_t id) {
            std::vector<void *> writeSet;
            std::vector<void *> ReadSet;


            random::mt19937 rng(current_time_nanoseconds());
            random::uniform_int_distribution<> randomKeys1(175000,200000);
            //cout << randomKeys(rng) << endl;
            int totalCachedMissed=0;
            for (int i = 0; i < 25000; i++)
            {
                char* keysToFind = KeysToStore[randomKeys1(rng)];
                auto val = ARTWithTuples.findValueByKey(keysToFind,id);


                if(val == nullptr)
                {
                    totalCachedMissed++;
                }
                else
                {
                    auto tp = val->value;
                    unsigned long index = tp.getAttribute<1>();
                    BOOST_TEST(tp.getAttribute<0>() == vectorValues[index].getAttribute<0>());
                    BOOST_TEST(tp.getAttribute<1>() == vectorValues[index].getAttribute<1>());
                    BOOST_TEST(tp.getAttribute<2>() == (int)(vectorValues[index].getAttribute<2>()));
                    //BOOST_TEST(tp.getAttribute<3>() == fmt::format("String/{}", keysToFind));
                    BOOST_TEST(tp.getAttribute<4>() == (vectorValues[index].getAttribute<1>()) / 200.0);
                }

            }

            cout<<"Total Cached missed out of 50,000 random keys ="<<totalCachedMissed<<" by transaction#"<<id<<endl;

        };

        auto start_time2 = std::chrono::high_resolution_clock::now();
        Transaction<TableOperationOnTupleFunc, ARTTupleContainer> *t1 = new Transaction<TableOperationOnTupleFunc, ARTTupleContainer>(
                findKeys1, *ARTable_for_updates8);
        Transaction<TableOperationOnTupleFunc, ARTTupleContainer> *t2 = new Transaction<TableOperationOnTupleFunc, ARTTupleContainer>(
                findKeys2, *ARTable_for_updates8);
        Transaction<TableOperationOnTupleFunc, ARTTupleContainer> *t3 = new Transaction<TableOperationOnTupleFunc, ARTTupleContainer>(
                findKeys3, *ARTable_for_updates8);
        Transaction<TableOperationOnTupleFunc, ARTTupleContainer> *t4 = new Transaction<TableOperationOnTupleFunc, ARTTupleContainer>(
                findKeys4, *ARTable_for_updates8);
        Transaction<TableOperationOnTupleFunc, ARTTupleContainer> *t5 = new Transaction<TableOperationOnTupleFunc, ARTTupleContainer>(
                findKeys5, *ARTable_for_updates8);
        Transaction<TableOperationOnTupleFunc, ARTTupleContainer> *t6 = new Transaction<TableOperationOnTupleFunc, ARTTupleContainer>(
                findKeys6, *ARTable_for_updates8);
        Transaction<TableOperationOnTupleFunc, ARTTupleContainer> *t7 = new Transaction<TableOperationOnTupleFunc, ARTTupleContainer>(
                findKeys7, *ARTable_for_updates8);
        Transaction<TableOperationOnTupleFunc, ARTTupleContainer> *t8 = new Transaction<TableOperationOnTupleFunc, ARTTupleContainer>(
                findKeys8, *ARTable_for_updates8);


        t1->CollectTransaction();
        t2->CollectTransaction();
        t3->CollectTransaction();
        t4->CollectTransaction();
        t5->CollectTransaction();
        t6->CollectTransaction();
        t7->CollectTransaction();
        t8->CollectTransaction();

        auto end_time2 = std::chrono::high_resolution_clock::now();
        cout << std::chrono::duration_cast<std::chrono::seconds>(end_time2 - start_time2).count() << ":";
        cout << std::chrono::duration_cast<std::chrono::microseconds>(end_time2 - start_time2).count() << ":";

        RESET_AND_DELETE(*ARTable_for_updates8);
    }

BOOST_AUTO_TEST_SUITE_END()

