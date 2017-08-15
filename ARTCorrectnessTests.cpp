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
auto ARTable_for_updates =  new ARTTupleContainer();


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


///home/muum8236/code/MVCCART/test_data

char * rootpath_uuid = "/Users/fuadshah/Desktop/MVCCART/test_data/uuid.txt";
char * rootpath_words = "/Users/fuadshah/Desktop/MVCCART/test_data/words.txt";

BOOST_AUTO_TEST_SUITE(MVCC_TESTS)



    BOOST_AUTO_TEST_CASE(test_loading_Buckets_from_TextFIle)
    {
        cout << "loading_Buckets_from_TextFIle" << endl;
        int len, len2;
        char buf[20];
        char bufVal[50];
        char bufVal2[20];

        /// Reading all Values to store against keys


        FILE *fvals = fopen(rootpath_uuid, "r");
        FILE *fkeys = fopen(rootpath_words, "r");
        int index = 0;

        while (fgets(bufVal, sizeof bufVal, fvals))
        {
            fgets(buf, sizeof buf, fkeys);
            len = strlen(bufVal);
            len2 = strlen(buf);
            buf[len2] = '\0';
            bufVal[len] = '\0';

            if(buf != "")
            {
                RecordType tuple = RecordType(buf,
                                              (unsigned long) index,
                                              index + 100,
                                              fmt::format("String/{}", buf),
                                              index / 100.0);

                strcpy(KeysToStore[index], buf);
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
    }

    /*
     * Testing correctness on concurrent Writes 100% write intensive, scaled till 8 transactions in concurrent
    */

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

        cout<<endl<<"Single Thread Writer Time->";
        cout << std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time).count() << ":";
        cout << std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count() << ":";
    }

    BOOST_AUTO_TEST_CASE(testing_random_10000_keys_loaded_by_single_transaction)
    {
        cout << "testing_10000 keys_loaded_by_single_transaction, randomly" << endl;
        ///Iterator Operation on TupleContainer
        auto findKeys1 = [] (ARTTupleContainer& ARTWithTuples,size_t id,std::string& status)
        {
            std::vector<RecordType> writeSet;
            std::vector<RecordType> ReadSet;

            random::mt19937 rng(current_time_nanoseconds());
            random::uniform_int_distribution<> randomKeys(0,200000);
            cout << randomKeys(rng) << endl;

            std::cout<<"Evaluating 10,000 random key'values:: by transaction:: "<<id<<std::endl;
            int totalCachedMissed = 0;
            for(int i=0; i < 10000; i++)
            {
                char* keysToFind = KeysToStore[randomKeys(rng)];
                auto val = ARTWithTuples.findValueByKey(keysToFind);

                if(val == nullptr)
                {
                    totalCachedMissed++;
                }
                else
                {
                    auto tp = val->value;
                    unsigned long index =  tp.getAttribute<1>();
                    BOOST_REQUIRE(tp.getAttribute<0>() == vectorValues[index].getAttribute<0>());
                    BOOST_REQUIRE(tp.getAttribute<1>() == vectorValues[index].getAttribute<1>());
                    BOOST_REQUIRE(tp.getAttribute<2>() == (vectorValues[index].getAttribute<2>()));
                    //BOOST_TEST(tp.getAttribute<3>() == fmt::format("String/{}", keysToFind));
                    BOOST_REQUIRE(tp.getAttribute<4>() == (index) / 100.0);
                }

            }
            cout<<"Total Cached missed out of 10,000 random keys ="<<totalCachedMissed<<":: by transaction#"<<id<<endl;

        };

        auto start_time2 = std::chrono::high_resolution_clock::now();
        Transaction<TableOperationOnTupleFunc,ARTTupleContainer>* t2 =  new Transaction<TableOperationOnTupleFunc,ARTTupleContainer>(findKeys1,*ARTableWithTuples);

        t2->CollectTransaction();
        auto end_time2= std::chrono::high_resolution_clock::now();
        cout << std::chrono::duration_cast<std::chrono::seconds>(end_time2 - start_time2).count() << ":";
        cout << std::chrono::duration_cast<std::chrono::microseconds>(end_time2 - start_time2).count() << ":";

        RESET_AND_DELETE(*ARTableWithTuples);

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

    BOOST_AUTO_TEST_CASE(testing_random_10000_keys_loaded_by_two_transactions)
    {
        cout << "testing_random_1000_keys_loaded_by_two_transactions" << endl;
        ///Iterator Operation on TupleContainer

        auto findKeys1 = [] (ARTTupleContainer& ARTWithTuples,size_t id,std::string& status)
        {
            std::vector<RecordType> writeSet;
            std::vector<RecordType> ReadSet;

            random::mt19937 rng(current_time_nanoseconds());
            random::uniform_int_distribution<> randomKeys(0,100000);
            cout << randomKeys(rng) << endl;

            std::cout<<"Evaluating 1000 random key'values :: by transaction:: "<<id<<std::endl;
            int totalCachedMissed = 0;
            for(int i=0; i < 10000; i++)
            {
                char* keysToFind = KeysToStore[randomKeys(rng)];
                auto val = ARTWithTuples.findValueByKey(keysToFind);

                if(val == nullptr)
                {
                    totalCachedMissed++;
                }
                else
                {
                    auto tp = val->value;
                    unsigned long index =  tp.getAttribute<1>();
                    BOOST_REQUIRE(tp.getAttribute<0>() == vectorValues[index].getAttribute<0>());
                    BOOST_REQUIRE(tp.getAttribute<1>() == vectorValues[index].getAttribute<1>());
                    BOOST_REQUIRE(tp.getAttribute<2>() == (vectorValues[index].getAttribute<2>()));
                    //BOOST_REQUIRE(tp.getAttribute<3>() == fmt::format("String/{}", keysToFind));
                    BOOST_REQUIRE(tp.getAttribute<4>() == (index) / 100.0);
                }

            }
            cout<<"Total Cached missed out of 10,000 random keys"<<totalCachedMissed<<" by transaction#"<<id<<endl;

        };


        auto findKeys2 = [] (ARTTupleContainer& ARTWithTuples,size_t id,std::string& status)
        {
            std::vector<RecordType> writeSet;
            std::vector<RecordType> ReadSet;

            random::mt19937 rng(current_time_nanoseconds());
            random::uniform_int_distribution<> randomKeys(100000,200000);
            cout << randomKeys(rng) << endl;

            std::cout<<"Evaluating 1000 random key'values :: by transaction:: "<<id<<std::endl;
            int totalCachedMissed = 0;
            for(int i=0; i < 10000; i++)
            {
                char* keysToFind = KeysToStore[randomKeys(rng)];
                auto val = ARTWithTuples.findValueByKey(keysToFind);

                if(val == nullptr)
                {
                    totalCachedMissed++;
                }
                else
                {
                    auto tp = val->value;
                    unsigned long index =  tp.getAttribute<1>();
                    BOOST_REQUIRE(tp.getAttribute<0>() == vectorValues[index].getAttribute<0>());
                    BOOST_REQUIRE(tp.getAttribute<1>() == vectorValues[index].getAttribute<1>());
                    BOOST_REQUIRE(tp.getAttribute<2>() == (vectorValues[index].getAttribute<2>()));
                    //BOOST_REQUIRE(tp.getAttribute<3>() == fmt::format("String/{}", keysToFind));
                    BOOST_REQUIRE(tp.getAttribute<4>() == (index) / 100.0);
                }

            }
            cout<<"Total Cached missed out of 10,000 random keys"<<totalCachedMissed<<" by transaction#"<<id<<endl;

        };

        auto start_time2 = std::chrono::high_resolution_clock::now();
        Transaction<TableOperationOnTupleFunc,ARTTupleContainer>* t1 =  new Transaction<TableOperationOnTupleFunc,ARTTupleContainer>(findKeys1,*ARTableWithTuples2);
        Transaction<TableOperationOnTupleFunc,ARTTupleContainer>* t2 =  new Transaction<TableOperationOnTupleFunc,ARTTupleContainer>(findKeys1,*ARTableWithTuples2);

        t1->CollectTransaction();
        t2->CollectTransaction();
        auto end_time2= std::chrono::high_resolution_clock::now();
        cout << std::chrono::duration_cast<std::chrono::seconds>(end_time2 - start_time2).count() << ":";
        cout << std::chrono::duration_cast<std::chrono::microseconds>(end_time2 - start_time2).count() << ":";

        RESET_AND_DELETE(*ARTableWithTuples2);

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
            std::vector<RecordType> writeSet;
            std::vector<RecordType> ReadSet;

            random::mt19937 rng(current_time_nanoseconds());
            random::uniform_int_distribution<> randomKeys(0,50000);
            cout << randomKeys(rng) << endl;

            std::cout<<"Evaluating 1000 random key'values :: by transaction:: "<<id<<std::endl;
            int totalCachedMissed = 0;
            for(int i=0; i < 10000; i++)
            {
                char* keysToFind = KeysToStore[randomKeys(rng)];
                auto val = ARTWithTuples.findValueByKey(keysToFind);

                if(val == nullptr)
                {
                    totalCachedMissed++;
                }
                else
                {
                    auto tp = val->value;
                    unsigned long index =  tp.getAttribute<1>();
                    BOOST_REQUIRE(tp.getAttribute<0>() == vectorValues[index].getAttribute<0>());
                    BOOST_REQUIRE(tp.getAttribute<1>() == vectorValues[index].getAttribute<1>());
                    BOOST_REQUIRE(tp.getAttribute<2>() == (vectorValues[index].getAttribute<2>()));
                    //BOOST_REQUIRE(tp.getAttribute<3>() == fmt::format("String/{}", keysToFind));
                    BOOST_REQUIRE(tp.getAttribute<4>() == (index) / 100.0);
                }

            }
            cout<<"Total Cached missed out of 10,000 random keys"<<totalCachedMissed<<" by transaction#"<<id<<endl;

        };


        auto findKeys2 = [] (ARTTupleContainer& ARTWithTuples,size_t id,std::string& status)
        {
            std::vector<RecordType> writeSet;
            std::vector<RecordType> ReadSet;

            random::mt19937 rng(current_time_nanoseconds());
            random::uniform_int_distribution<> randomKeys(50000,100000);
            cout << randomKeys(rng) << endl;

            std::cout<<"Evaluating 1000 random key'values :: by transaction:: "<<id<<std::endl;
            int totalCachedMissed = 0;
            for(int i=0; i < 10000; i++)
            {
                char* keysToFind = KeysToStore[randomKeys(rng)];
                auto val = ARTWithTuples.findValueByKey(keysToFind);

                if(val == nullptr)
                {
                    totalCachedMissed++;
                }
                else
                {
                    auto tp = val->value;
                    unsigned long index =  tp.getAttribute<1>();
                    BOOST_REQUIRE(tp.getAttribute<0>() == vectorValues[index].getAttribute<0>());
                    BOOST_REQUIRE(tp.getAttribute<1>() == vectorValues[index].getAttribute<1>());
                    BOOST_REQUIRE(tp.getAttribute<2>() == (vectorValues[index].getAttribute<2>()));
                    //BOOST_REQUIRE(tp.getAttribute<3>() == fmt::format("String/{}", keysToFind));
                    BOOST_REQUIRE(tp.getAttribute<4>() == (index) / 100.0);
                }

            }
            cout<<"Total Cached missed out of 10,000 random keys"<<totalCachedMissed<<" by transaction#"<<id<<endl;

        };

        auto findKeys3 = [] (ARTTupleContainer& ARTWithTuples,size_t id,std::string& status)
        {
            std::vector<RecordType> writeSet;
            std::vector<RecordType> ReadSet;

            random::mt19937 rng(current_time_nanoseconds());
            random::uniform_int_distribution<> randomKeys(100000,150000);
            cout << randomKeys(rng) << endl;

            std::cout<<"Evaluating 1000 random key'values :: by transaction:: "<<id<<std::endl;
            int totalCachedMissed = 0;
            for(int i=0; i < 10000; i++)
            {
                char* keysToFind = KeysToStore[randomKeys(rng)];
                auto val = ARTWithTuples.findValueByKey(keysToFind);

                if(val == nullptr)
                {
                    totalCachedMissed++;
                }
                else
                {
                    auto tp = val->value;
                    unsigned long index =  tp.getAttribute<1>();
                    BOOST_REQUIRE(tp.getAttribute<0>() == vectorValues[index].getAttribute<0>());
                    BOOST_REQUIRE(tp.getAttribute<1>() == vectorValues[index].getAttribute<1>());
                    BOOST_REQUIRE(tp.getAttribute<2>() == (vectorValues[index].getAttribute<2>()));
                    //BOOST_REQUIRE(tp.getAttribute<3>() == fmt::format("String/{}", keysToFind));
                    BOOST_REQUIRE(tp.getAttribute<4>() == (index) / 100.0);
                }

            }
            cout<<"Total Cached missed out of 10,000 random keys"<<totalCachedMissed<<" by transaction#"<<id<<endl;

        };


        auto findKeys4 = [] (ARTTupleContainer& ARTWithTuples,size_t id,std::string& status)
        {
            std::vector<RecordType> writeSet;
            std::vector<RecordType> ReadSet;

            random::mt19937 rng(current_time_nanoseconds());
            random::uniform_int_distribution<> randomKeys(150000,200000);
            cout << randomKeys(rng) << endl;

            std::cout<<"Evaluating 1000 random key'values :: by transaction:: "<<id<<std::endl;
            int totalCachedMissed = 0;
            for(int i=0; i < 10000; i++)
            {
                char* keysToFind = KeysToStore[randomKeys(rng)];
                auto val = ARTWithTuples.findValueByKey(keysToFind);

                if(val == nullptr)
                {
                    totalCachedMissed++;
                }
                else
                {
                    auto tp = val->value;
                    unsigned long index =  tp.getAttribute<1>();
                    BOOST_REQUIRE(tp.getAttribute<0>() == vectorValues[index].getAttribute<0>());
                    BOOST_REQUIRE(tp.getAttribute<1>() == vectorValues[index].getAttribute<1>());
                    BOOST_REQUIRE(tp.getAttribute<2>() == (vectorValues[index].getAttribute<2>()));
                    //BOOST_REQUIRE(tp.getAttribute<3>() == fmt::format("String/{}", keysToFind));
                    BOOST_REQUIRE(tp.getAttribute<4>() == (index) / 100.0);
                }

            }
            cout<<"Total Cached missed out of 10,000 random keys"<<totalCachedMissed<<" by transaction#"<<id<<endl;

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

        RESET_AND_DELETE(*ARTableWithTuples4);

    }

    BOOST_AUTO_TEST_CASE(test_load_ARTIndex_MVCC_two_hundred_thousand_keys_eight_transactions)
    {
        cout << "test_load_ARTIndex_MVCC_two_hundred_thousand_keys_eight_transactions" << endl;

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

        cout << "Multi 8 Thread Writers Time->";
        cout << std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time).count() << ":";
        cout << std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count() << ":";
    }

    BOOST_AUTO_TEST_CASE(testing_random_1000_keys_loaded_by_8_transactions)
    {
        cout << "testing_random_1000_keys_loaded_by_Eight_transactions, randomly" << endl;
        ///Iterator Operation on TupleContainer

        auto findKeys1 = [] (ARTTupleContainer& ARTWithTuples,size_t id,std::string& status)
        {
            std::vector<RecordType> writeSet;
            std::vector<RecordType> ReadSet;

            random::mt19937 rng(current_time_nanoseconds());
            random::uniform_int_distribution<> randomKeys(0,25000);
            cout << randomKeys(rng) << endl;

            std::cout<<"Evaluating 1000 random key'values :: by transaction:: "<<id<<std::endl;
            int totalCachedMissed = 0;
            for(int i=0; i < 10000; i++)
            {
                char* keysToFind = KeysToStore[randomKeys(rng)];
                auto val = ARTWithTuples.findValueByKey(keysToFind);

                if(val == nullptr)
                {
                    totalCachedMissed++;
                }
                else
                {
                    auto tp = val->value;
                    unsigned long index =  tp.getAttribute<1>();
                    /*BOOST_REQUIRE(tp.getAttribute<0>() == vectorValues[index].getAttribute<0>());
                    BOOST_REQUIRE(tp.getAttribute<1>() == vectorValues[index].getAttribute<1>());
                    BOOST_REQUIRE(tp.getAttribute<2>() == (vectorValues[index].getAttribute<2>()));
                    //BOOST_REQUIRE(tp.getAttribute<3>() == fmt::format("String/{}", keysToFind));
                    BOOST_REQUIRE(tp.getAttribute<4>() == (index) / 100.0);*/
                }

            }
            cout<<"Total Cached missed out of 10,000 random keys"<<totalCachedMissed<<" by transaction#"<<id<<endl;

        };


        auto findKeys2 = [] (ARTTupleContainer& ARTWithTuples,size_t id,std::string& status)
        {
            std::vector<RecordType> writeSet;
            std::vector<RecordType> ReadSet;

            random::mt19937 rng(current_time_nanoseconds());
            random::uniform_int_distribution<> randomKeys(25000,50000);
            cout << randomKeys(rng) << endl;

            std::cout<<"Evaluating 1000 random key'values :: by transaction:: "<<id<<std::endl;
            int totalCachedMissed = 0;
            for(int i=0; i < 10000; i++)
            {
                char* keysToFind = KeysToStore[randomKeys(rng)];
                auto val = ARTWithTuples.findValueByKey(keysToFind);

                if(val == nullptr)
                {
                    totalCachedMissed++;
                }
                else
                {
                    auto tp = val->value;
                    unsigned long index =  tp.getAttribute<1>();
                    /*BOOST_REQUIRE(tp.getAttribute<0>() == vectorValues[index].getAttribute<0>());
                    BOOST_REQUIRE(tp.getAttribute<1>() == vectorValues[index].getAttribute<1>());
                    BOOST_REQUIRE(tp.getAttribute<2>() == (vectorValues[index].getAttribute<2>()));
                    //BOOST_REQUIRE(tp.getAttribute<3>() == fmt::format("String/{}", keysToFind));
                    BOOST_REQUIRE(tp.getAttribute<4>() == (index) / 100.0);*/
                }

            }
            cout<<"Total Cached missed out of 10,000 random keys"<<totalCachedMissed<<" by transaction#"<<id<<endl;

        };

        auto findKeys3 = [] (ARTTupleContainer& ARTWithTuples,size_t id,std::string& status)
        {
            std::vector<RecordType> writeSet;
            std::vector<RecordType> ReadSet;

            random::mt19937 rng(current_time_nanoseconds());
            random::uniform_int_distribution<> randomKeys(50000,75000);
            cout << randomKeys(rng) << endl;

            std::cout<<"Evaluating 1000 random key'values :: by transaction:: "<<id<<std::endl;
            int totalCachedMissed = 0;
            for(int i=0; i < 10000; i++)
            {
                char* keysToFind = KeysToStore[randomKeys(rng)];
                auto val = ARTWithTuples.findValueByKey(keysToFind);

                if(val == nullptr)
                {
                    totalCachedMissed++;
                }
                else
                {
                    auto tp = val->value;
                    unsigned long index =  tp.getAttribute<1>();
                    /*BOOST_REQUIRE(tp.getAttribute<0>() == vectorValues[index].getAttribute<0>());
                    BOOST_REQUIRE(tp.getAttribute<1>() == vectorValues[index].getAttribute<1>());
                    BOOST_REQUIRE(tp.getAttribute<2>() == (vectorValues[index].getAttribute<2>()));
                    //BOOST_REQUIRE(tp.getAttribute<3>() == fmt::format("String/{}", keysToFind));
                    BOOST_REQUIRE(tp.getAttribute<4>() == (index) / 100.0);*/
                }

            }
            cout<<"Total Cached missed out of 10,000 random keys"<<totalCachedMissed<<" by transaction#"<<id<<endl;

        };


        auto findKeys4 = [] (ARTTupleContainer& ARTWithTuples,size_t id,std::string& status)
        {
            std::vector<RecordType> writeSet;
            std::vector<RecordType> ReadSet;

            random::mt19937 rng(current_time_nanoseconds());
            random::uniform_int_distribution<> randomKeys(75000,100000);
            cout << randomKeys(rng) << endl;

            std::cout<<"Evaluating 1000 random key'values :: by transaction:: "<<id<<std::endl;
            int totalCachedMissed = 0;
            for(int i=0; i < 10000; i++)
            {
                char* keysToFind = KeysToStore[randomKeys(rng)];
                auto val = ARTWithTuples.findValueByKey(keysToFind);

                if(val == nullptr)
                {
                    totalCachedMissed++;
                }
                else
                {
                    auto tp = val->value;
                    unsigned long index =  tp.getAttribute<1>();
                    /*BOOST_REQUIRE(tp.getAttribute<0>() == vectorValues[index].getAttribute<0>());
                    BOOST_REQUIRE(tp.getAttribute<1>() == vectorValues[index].getAttribute<1>());
                    BOOST_REQUIRE(tp.getAttribute<2>() == (vectorValues[index].getAttribute<2>()));
                    //BOOST_REQUIRE(tp.getAttribute<3>() == fmt::format("String/{}", keysToFind));
                    BOOST_REQUIRE(tp.getAttribute<4>() == (index) / 100.0);*/
                }

            }
            cout<<"Total Cached missed out of 10,000 random keys"<<totalCachedMissed<<" by transaction#"<<id<<endl;

        };

        auto findKeys5 = [] (ARTTupleContainer& ARTWithTuples,size_t id,std::string& status)
        {
            std::vector<RecordType> writeSet;
            std::vector<RecordType> ReadSet;

            random::mt19937 rng(current_time_nanoseconds());
            random::uniform_int_distribution<> randomKeys(100000,125000);
            cout << randomKeys(rng) << endl;

            std::cout<<"Evaluating 1000 random key'values :: by transaction:: "<<id<<std::endl;
            int totalCachedMissed = 0;
            for(int i=0; i < 10000; i++)
            {
                char* keysToFind = KeysToStore[randomKeys(rng)];
                auto val = ARTWithTuples.findValueByKey(keysToFind);

                if(val == nullptr)
                {
                    totalCachedMissed++;
                }
                else
                {
                    auto tp = val->value;
                    unsigned long index =  tp.getAttribute<1>();
                    /*BOOST_REQUIRE(tp.getAttribute<0>() == vectorValues[index].getAttribute<0>());
                    BOOST_REQUIRE(tp.getAttribute<1>() == vectorValues[index].getAttribute<1>());
                    BOOST_REQUIRE(tp.getAttribute<2>() == (vectorValues[index].getAttribute<2>()));
                    //BOOST_REQUIRE(tp.getAttribute<3>() == fmt::format("String/{}", keysToFind));
                    BOOST_REQUIRE(tp.getAttribute<4>() == (index) / 100.0);*/
                }

            }
            cout<<"Total Cached missed out of 10,000 random keys"<<totalCachedMissed<<" by transaction#"<<id<<endl;

        };


        auto findKeys6 = [] (ARTTupleContainer& ARTWithTuples,size_t id,std::string& status)
        {
            std::vector<RecordType> writeSet;
            std::vector<RecordType> ReadSet;

            random::mt19937 rng(current_time_nanoseconds());
            random::uniform_int_distribution<> randomKeys(125000,150000);
            cout << randomKeys(rng) << endl;

            std::cout<<"Evaluating 1000 random key'values :: by transaction:: "<<id<<std::endl;
            int totalCachedMissed = 0;
            for(int i=0; i < 10000; i++)
            {
                char* keysToFind = KeysToStore[randomKeys(rng)];
                auto val = ARTWithTuples.findValueByKey(keysToFind);

                if(val == nullptr)
                {
                    totalCachedMissed++;
                }
                else
                {
                    auto tp = val->value;
                    unsigned long index =  tp.getAttribute<1>();
                    /*BOOST_REQUIRE(tp.getAttribute<0>() == vectorValues[index].getAttribute<0>());
                    BOOST_REQUIRE(tp.getAttribute<1>() == vectorValues[index].getAttribute<1>());
                    BOOST_REQUIRE(tp.getAttribute<2>() == (vectorValues[index].getAttribute<2>()));
                    //BOOST_REQUIRE(tp.getAttribute<3>() == fmt::format("String/{}", keysToFind));
                    BOOST_REQUIRE(tp.getAttribute<4>() == (index) / 100.0);*/
                }

            }
            cout<<"Total Cached missed out of 10,000 random keys"<<totalCachedMissed<<" by transaction#"<<id<<endl;

        };

        auto findKeys7 = [] (ARTTupleContainer& ARTWithTuples,size_t id,std::string& status)
        {
            std::vector<RecordType> writeSet;
            std::vector<RecordType> ReadSet;

            random::mt19937 rng(current_time_nanoseconds());
            random::uniform_int_distribution<> randomKeys(150000,175000);
            cout << randomKeys(rng) << endl;

            std::cout<<"Evaluating 1000 random key'values :: by transaction:: "<<id<<std::endl;
            int totalCachedMissed = 0;
            for(int i=0; i < 10000; i++)
            {
                char* keysToFind = KeysToStore[randomKeys(rng)];
                auto val = ARTWithTuples.findValueByKey(keysToFind);

                if(val == nullptr)
                {
                    totalCachedMissed++;
                }
                else
                {
                    auto tp = val->value;
                    unsigned long index =  tp.getAttribute<1>();
                    /*BOOST_REQUIRE(tp.getAttribute<0>() == vectorValues[index].getAttribute<0>());
                    BOOST_REQUIRE(tp.getAttribute<1>() == vectorValues[index].getAttribute<1>());
                    BOOST_REQUIRE(tp.getAttribute<2>() == (vectorValues[index].getAttribute<2>()));
                    //BOOST_REQUIRE(tp.getAttribute<3>() == fmt::format("String/{}", keysToFind));
                    BOOST_REQUIRE(tp.getAttribute<4>() == (index) / 100.0);*/
                }

            }
            cout<<"Total Cached missed out of 10,000 random keys"<<totalCachedMissed<<" by transaction#"<<id<<endl;

        };


        auto findKeys8 = [] (ARTTupleContainer& ARTWithTuples,size_t id,std::string& status)
        {
            std::vector<RecordType> writeSet;
            std::vector<RecordType> ReadSet;

            random::mt19937 rng(current_time_nanoseconds());
            random::uniform_int_distribution<> randomKeys(175000,200000);
            cout << randomKeys(rng) << endl;

            std::cout<<"Evaluating 1000 random key'values :: by transaction:: "<<id<<std::endl;
            int totalCachedMissed = 0;
            for(int i=0; i < 10000; i++)
            {
                char* keysToFind = KeysToStore[randomKeys(rng)];
                auto val = ARTWithTuples.findValueByKey(keysToFind);

                if(val == nullptr)
                {
                    totalCachedMissed++;
                }
                else
                {
                    auto tp = val->value;
                    unsigned long index =  tp.getAttribute<1>();
                    /*BOOST_REQUIRE(tp.getAttribute<0>() == vectorValues[index].getAttribute<0>());
                    BOOST_REQUIRE(tp.getAttribute<1>() == vectorValues[index].getAttribute<1>());
                    BOOST_REQUIRE(tp.getAttribute<2>() == (vectorValues[index].getAttribute<2>()));
                    //BOOST_REQUIRE(tp.getAttribute<3>() == fmt::format("String/{}", keysToFind));
                    BOOST_REQUIRE(tp.getAttribute<4>() == (index) / 100.0);*/
                }

            }
            cout<<"Total Cached missed out of 10,000 random keys "<<totalCachedMissed<<" by transaction#"<<id<<endl;

        };


        auto iterateAll = [](ARTTupleContainer &ARTWithTuples, size_t id, std::string &status) {
            std::vector<void *> writeSet;
            std::vector<void *> ReadSet;



            std::cout << "evaluation of all keys in tree:: " << id << std::endl;

            for (int i = 0; i < 1000; i++)
            {

                ARTWithTuples.iterate(cb,"Aa",id);
            }

        };

        auto start_time2 = std::chrono::high_resolution_clock::now();
        Transaction<TableOperationOnTupleFunc, ARTTupleContainer> *t1 = new Transaction<TableOperationOnTupleFunc, ARTTupleContainer>(
                findKeys1, *ARTableWithTuples8);
        Transaction<TableOperationOnTupleFunc, ARTTupleContainer> *t2 = new Transaction<TableOperationOnTupleFunc, ARTTupleContainer>(
                findKeys2, *ARTableWithTuples8);
        Transaction<TableOperationOnTupleFunc, ARTTupleContainer> *t3 = new Transaction<TableOperationOnTupleFunc, ARTTupleContainer>(
                findKeys3, *ARTableWithTuples8);
        Transaction<TableOperationOnTupleFunc, ARTTupleContainer> *t4 = new Transaction<TableOperationOnTupleFunc, ARTTupleContainer>(
                findKeys4, *ARTableWithTuples8);

        Transaction<TableOperationOnTupleFunc, ARTTupleContainer> *t5 = new Transaction<TableOperationOnTupleFunc, ARTTupleContainer>(
                findKeys5, *ARTableWithTuples8);
        Transaction<TableOperationOnTupleFunc, ARTTupleContainer> *t6 = new Transaction<TableOperationOnTupleFunc, ARTTupleContainer>(
                findKeys6, *ARTableWithTuples8);
        Transaction<TableOperationOnTupleFunc, ARTTupleContainer> *t7 = new Transaction<TableOperationOnTupleFunc, ARTTupleContainer>(
                findKeys7, *ARTableWithTuples8);
        Transaction<TableOperationOnTupleFunc, ARTTupleContainer> *t8 = new Transaction<TableOperationOnTupleFunc, ARTTupleContainer>(
                findKeys8, *ARTableWithTuples8);


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

        RESET_AND_DELETE(*ARTableWithTuples8);


    }

/*
    BOOST_AUTO_TEST_CASE(testing_update_intensive)
    {
        cout << "testing_Updates_by_transactions" << endl;

        auto WriteKeys1 = [](ARTTupleContainer &ARTable, size_t id, std::string &status)
        {
            int index = 0;
            while (true)
            {
               auto result =  ARTable.insertOrUpdateByKey(KeysToStore[index], vectorValues[index], id, status);
               if(result != NULL)
               {
                   cout<<"cannot insert "<<KeysToStore[index]<<endl;
               }
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

            for(int i=0; i < 25000; i++)
            {
                char* keysToFind = KeysToStore[i];
                auto result= ARTWithTuples.insertOrUpdateByKey(keysToFind,updater,id,status);
            }
        };

        auto update2 = [] (ARTTupleContainer& ARTWithTuples,size_t id,std::string& status)
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

            for(int i=25000; i < 50000; i++)
            {
                char* keysToFind = KeysToStore[i];
                auto result= ARTWithTuples.insertOrUpdateByKey(keysToFind,updater,id,status);
                //BOOST_CHECK(result != NULL);
            }
        };

        auto update3 = [] (ARTTupleContainer& ARTWithTuples,size_t id,std::string& status)
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

            for(int i=50000; i < 75000; i++)
            {
                char* keysToFind = KeysToStore[i];
                auto result= ARTWithTuples.insertOrUpdateByKey(keysToFind,updater,id,status);
                //BOOST_CHECK(result != NULL);
            }
        };


        auto update4 = [] (ARTTupleContainer& ARTWithTuples,size_t id,std::string& status)
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

            for(int i=75000; i < 100000; i++)
            {
                char* keysToFind = KeysToStore[i];
                auto result= ARTWithTuples.insertOrUpdateByKey(keysToFind,updater,id,status);
                //BOOST_CHECK(result != NULL);
            }
        };

        auto update5 = [] (ARTTupleContainer& ARTWithTuples,size_t id,std::string& status)
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

            for(int i=100000; i < 125000; i++)
            {
                char* keysToFind = KeysToStore[i];
                auto result= ARTWithTuples.insertOrUpdateByKey(keysToFind,updater,id,status);
                //BOOST_CHECK(result != NULL);
            }
        };

        auto update6 = [] (ARTTupleContainer& ARTWithTuples,size_t id,std::string& status)
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

            for(int i=125000; i < 150000; i++)
            {
                char* keysToFind = KeysToStore[i];
                auto result= ARTWithTuples.insertOrUpdateByKey(keysToFind,updater,id,status);
                //BOOST_CHECK(result != NULL);
            }
        };

        auto update7 = [] (ARTTupleContainer& ARTWithTuples,size_t id,std::string& status)
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

            for(int i=150000; i < 175000; i++)
            {
                char* keysToFind = KeysToStore[i];
                auto result= ARTWithTuples.insertOrUpdateByKey(keysToFind,updater,id,status);
                //BOOST_CHECK(result != NULL);

            }
        };

        auto update8 = [] (ARTTupleContainer& ARTWithTuples,size_t id,std::string& status)
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

            for(int i=175000; i < 200000; i++)
            {
                char* keysToFind = KeysToStore[i];
                auto result= ARTWithTuples.insertOrUpdateByKey(keysToFind,updater,id,status);
                //BOOST_CHECK(result != NULL);
            }
        };

        cout<<"starting new inserts"<<endl;
        Transaction<TableOperationOnTupleFunc, ARTTupleContainer> *t0 = new Transaction<TableOperationOnTupleFunc, ARTTupleContainer>(WriteKeys1, *ARTable_for_updates);
        t0->CollectTransaction();
        cout<<"starting 8 updaters "<<endl;
        auto start_time = std::chrono::high_resolution_clock::now();
        Transaction<TableOperationOnTupleFunc, ARTTupleContainer> *t1 = new Transaction<TableOperationOnTupleFunc, ARTTupleContainer>(update1, *ARTable_for_updates);
        Transaction<TableOperationOnTupleFunc, ARTTupleContainer> *t2 = new Transaction<TableOperationOnTupleFunc, ARTTupleContainer>(update2, *ARTable_for_updates);
        Transaction<TableOperationOnTupleFunc, ARTTupleContainer> *t3 = new Transaction<TableOperationOnTupleFunc, ARTTupleContainer>(update3, *ARTable_for_updates);
        Transaction<TableOperationOnTupleFunc, ARTTupleContainer> *t4 = new Transaction<TableOperationOnTupleFunc, ARTTupleContainer>(update4, *ARTable_for_updates);
        Transaction<TableOperationOnTupleFunc, ARTTupleContainer> *t5 = new Transaction<TableOperationOnTupleFunc, ARTTupleContainer>(update5, *ARTable_for_updates);
        Transaction<TableOperationOnTupleFunc, ARTTupleContainer> *t6 = new Transaction<TableOperationOnTupleFunc, ARTTupleContainer>(update6, *ARTable_for_updates);
        Transaction<TableOperationOnTupleFunc, ARTTupleContainer> *t7 = new Transaction<TableOperationOnTupleFunc, ARTTupleContainer>(update7, *ARTable_for_updates);
        Transaction<TableOperationOnTupleFunc, ARTTupleContainer> *t8 = new Transaction<TableOperationOnTupleFunc, ARTTupleContainer>(update8, *ARTable_for_updates);


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

    BOOST_AUTO_TEST_CASE(Verifying_updates_intensive)
    {
        cout << "Verifying updates" << endl;
        ///Iterator Operation on TupleContainer
        auto findKeys1 = [](ARTTupleContainer &ARTWithTuples, size_t id, std::string &status) {
            std::vector<void *> writeSet;
            std::vector<void *> ReadSet;


            random::mt19937 rng(current_time_nanoseconds());
            random::uniform_int_distribution<> randomKeys1(0,200000);
            //cout << randomKeys(rng) << endl;

            for (int i = 0; i < 50000; i++)
            {
                char* keysToFind = KeysToStore[randomKeys1(rng)];
                void *val = ARTWithTuples.findValueByKey(keysToFind);
                mvcc11::mvcc<RecordType> *_mvccValue = reinterpret_cast<mvcc11::mvcc<RecordType> *>(val);

                auto tp = _mvccValue->current()->value;

                unsigned long index = tp.getAttribute<1>();
                BOOST_TEST(tp.getAttribute<0>() == vectorValues[index].getAttribute<0>());
                BOOST_TEST(tp.getAttribute<1>() == vectorValues[index].getAttribute<1>());
                BOOST_TEST(tp.getAttribute<2>() == (vectorValues[index].getAttribute<2>()) + 100);
                BOOST_TEST(tp.getAttribute<3>() == fmt::format("String/{}", keysToFind));
                BOOST_TEST(tp.getAttribute<4>() == (index) / 200.0);


            }
        };

        auto start_time2 = std::chrono::high_resolution_clock::now();
        Transaction<TableOperationOnTupleFunc, ARTTupleContainer> *t2 = new Transaction<TableOperationOnTupleFunc, ARTTupleContainer>(
                findKeys1, *ARTableWithTuples8);
        t2->CollectTransaction();
        auto end_time2 = std::chrono::high_resolution_clock::now();
        cout << std::chrono::duration_cast<std::chrono::seconds>(end_time2 - start_time2).count() << ":";
        cout << std::chrono::duration_cast<std::chrono::microseconds>(end_time2 - start_time2).count() << ":";
    }
*/
BOOST_AUTO_TEST_SUITE_END()

