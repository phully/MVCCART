#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE MVCC_TEST

#include <iostream>
#include <boost/test/unit_test.hpp>
#include <boost/lexical_cast.hpp>
#include "mvcc/mvcc.hpp"
#include "ART/ARTFULCpp.h"
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
using snapshot_type = mvcc11::snapshot<RecordType>;
typedef smart_ptr::shared_ptr<snapshot_type const> const_snapshot_ptr;

char KeysToStore[235890][20];
std::vector<RecordType> vectorValues;



auto ARTable1 =  new ARTTupleContainer();
auto ARTable2 =  new ARTTupleContainer();
auto ARTable4 =  new ARTTupleContainer();
auto ARTable8 =  new ARTTupleContainer();


void RESET_AND_DELETE(ARTTupleContainer& ART )
{
    ART.DestroyAdaptiveRadixTreeTable();
    reset_transaction_ID();

}




int current_time_nanoseconds(){
    struct timespec tm;
    clock_gettime(CLOCK_REALTIME, &tm);
    return tm.tv_nsec;
}


///home/muum8236/code/MVCCART/test_data
char * rootpath_words = "/Users/fuadshah/Desktop/MVCCART/test_data/words.txt";

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


    BOOST_AUTO_TEST_CASE(Write200000keys)
    {


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

        auto start_timeWriter = std::chrono::high_resolution_clock::now();
        Transaction<TableOperationOnTupleFunc,ARTTupleContainer>* writerTransaction = new
                Transaction<TableOperationOnTupleFunc,ARTTupleContainer>(WriteKeys1,*ARTable1);
        writerTransaction->CollectTransaction();
        auto end_timeWriter= std::chrono::high_resolution_clock::now();

        cout<<endl<<"Single Thread Writer Time->";
        cout << std::chrono::duration_cast<std::chrono::seconds>(end_timeWriter - start_timeWriter).count() << ":";
        cout << std::chrono::duration_cast<std::chrono::microseconds>(end_timeWriter - start_timeWriter).count() << ":"<<endl;
    }

    BOOST_AUTO_TEST_CASE(ReadOnly100Ops2Transactions)
    {
        cout << "ReadOnly100Ops2Transactions" << endl;
        ///Iterator Operation on TupleContainer
        auto findKeys1 = [](ARTTupleContainer &ARTWithTuples, size_t id, std::string &status) {
            std::vector<void *> writeSet;
            std::vector<void *> ReadSet;


            random::mt19937 rng(current_time_nanoseconds());
            random::uniform_int_distribution<> randomKeys1(0,100000);
            //cout << randomKeys(rng) << endl;
            int totalCachedMissed=0;
            for (int i = 0; i < 100; i++)
            {
                char* keysToFind = KeysToStore[randomKeys1(rng)];
                auto val = ARTWithTuples.findValueByKey(keysToFind);


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
                    BOOST_TEST(tp.getAttribute<4>() == (vectorValues[index].getAttribute<1>()) / 100.0);
                }
            }

            cout<<"Total Cached missed out of 100 random keys ="<<totalCachedMissed<<" by transaction#"<<id<<endl;
        };

        auto findKeys2 = [](ARTTupleContainer &ARTWithTuples, size_t id, std::string &status) {
            std::vector<void *> writeSet;
            std::vector<void *> ReadSet;


            random::mt19937 rng(current_time_nanoseconds());
            random::uniform_int_distribution<> randomKeys1(100000,200000);
            //cout << randomKeys(rng) << endl;
            int totalCachedMissed=0;
            for (int i = 0; i < 100; i++)
            {
                char* keysToFind = KeysToStore[randomKeys1(rng)];
                auto val = ARTWithTuples.findValueByKey(keysToFind);


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
                    BOOST_TEST(tp.getAttribute<4>() == (vectorValues[index].getAttribute<1>()) / 100.0);
                }

            }

            cout<<"Total Cached missed out of 100 random keys ="<<totalCachedMissed<<" by transaction#"<<id<<endl;

        };

        auto start_time2 = std::chrono::high_resolution_clock::now();
        Transaction<TableOperationOnTupleFunc, ARTTupleContainer> *t1 = new Transaction<TableOperationOnTupleFunc, ARTTupleContainer>(
                findKeys1, *ARTable1);
        Transaction<TableOperationOnTupleFunc, ARTTupleContainer> *t2 = new Transaction<TableOperationOnTupleFunc, ARTTupleContainer>(
                findKeys2, *ARTable1);
        t1->CollectTransaction();
        t2->CollectTransaction();
        auto end_time2 = std::chrono::high_resolution_clock::now();


        cout<<"Total time by ReadOnly100Ops2Transactions::"<<endl;

        cout << std::chrono::duration_cast<std::chrono::seconds>(end_time2 - start_time2).count() << ":";
        cout << std::chrono::duration_cast<std::chrono::microseconds>(end_time2 - start_time2).count() << ":"<<endl;


    }

    BOOST_AUTO_TEST_CASE(ReadOnly100Ops4Transactions)
    {
        cout << "ReadOnly100Ops4Transactions" << endl;
        ///Iterator Operation on TupleContainer
        auto findKeys1 = [](ARTTupleContainer &ARTWithTuples, size_t id, std::string &status) {
            std::vector<void *> writeSet;
            std::vector<void *> ReadSet;


            random::mt19937 rng(current_time_nanoseconds());
            random::uniform_int_distribution<> randomKeys1(0,50000);
            //cout << randomKeys(rng) << endl;
            int totalCachedMissed=0;
            for (int i = 0; i < 100; i++)
            {
                char* keysToFind = KeysToStore[randomKeys1(rng)];
                auto val = ARTWithTuples.findValueByKey(keysToFind);


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
                    BOOST_TEST(tp.getAttribute<4>() == (vectorValues[index].getAttribute<1>()) / 100.0);
                }
            }

            cout<<"Total Cached missed out of 100 random keys ="<<totalCachedMissed<<" by transaction#"<<id<<endl;
        };

        auto findKeys2 = [](ARTTupleContainer &ARTWithTuples, size_t id, std::string &status) {
            std::vector<void *> writeSet;
            std::vector<void *> ReadSet;


            random::mt19937 rng(current_time_nanoseconds());
            random::uniform_int_distribution<> randomKeys1(50000,100000);
            //cout << randomKeys(rng) << endl;
            int totalCachedMissed=0;
            for (int i = 0; i < 100; i++)
            {
                char* keysToFind = KeysToStore[randomKeys1(rng)];
                auto val = ARTWithTuples.findValueByKey(keysToFind);


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
                    BOOST_TEST(tp.getAttribute<4>() == (vectorValues[index].getAttribute<1>()) / 100.0);
                }

            }

            cout<<"Total Cached missed out of 100 random keys ="<<totalCachedMissed<<" by transaction#"<<id<<endl;

        };

        auto findKeys3 = [](ARTTupleContainer &ARTWithTuples, size_t id, std::string &status) {
            std::vector<void *> writeSet;
            std::vector<void *> ReadSet;


            random::mt19937 rng(current_time_nanoseconds());
            random::uniform_int_distribution<> randomKeys1(100000,150000);
            //cout << randomKeys(rng) << endl;
            int totalCachedMissed=0;
            for (int i = 0; i < 100; i++)
            {
                char* keysToFind = KeysToStore[randomKeys1(rng)];
                auto val = ARTWithTuples.findValueByKey(keysToFind);


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
                    BOOST_TEST(tp.getAttribute<4>() == (vectorValues[index].getAttribute<1>()) / 100.0);
                }
            }

            cout<<"Total Cached missed out of 100 random keys ="<<totalCachedMissed<<" by transaction#"<<id<<endl;
        };

        auto findKeys4 = [](ARTTupleContainer &ARTWithTuples, size_t id, std::string &status) {
            std::vector<void *> writeSet;
            std::vector<void *> ReadSet;


            random::mt19937 rng(current_time_nanoseconds());
            random::uniform_int_distribution<> randomKeys1(150000,200000);
            //cout << randomKeys(rng) << endl;
            int totalCachedMissed=0;
            for (int i = 0; i < 100; i++)
            {
                char* keysToFind = KeysToStore[randomKeys1(rng)];
                auto val = ARTWithTuples.findValueByKey(keysToFind);


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
                    BOOST_TEST(tp.getAttribute<4>() == (vectorValues[index].getAttribute<1>()) / 100.0);
                }

            }

            cout<<"Total Cached missed out of 100 random keys ="<<totalCachedMissed<<" by transaction#"<<id<<endl;

        };

        auto start_time2 = std::chrono::high_resolution_clock::now();
        Transaction<TableOperationOnTupleFunc, ARTTupleContainer> *t1 = new Transaction<TableOperationOnTupleFunc, ARTTupleContainer>(
                findKeys1, *ARTable1);
        Transaction<TableOperationOnTupleFunc, ARTTupleContainer> *t2 = new Transaction<TableOperationOnTupleFunc, ARTTupleContainer>(
                findKeys2, *ARTable1);

        Transaction<TableOperationOnTupleFunc, ARTTupleContainer> *t3 = new Transaction<TableOperationOnTupleFunc, ARTTupleContainer>(
                findKeys3, *ARTable1);
        Transaction<TableOperationOnTupleFunc, ARTTupleContainer> *t4 = new Transaction<TableOperationOnTupleFunc, ARTTupleContainer>(
                findKeys4, *ARTable1);
        t1->CollectTransaction();
        t2->CollectTransaction();
        t3->CollectTransaction();
        t4->CollectTransaction();
        auto end_time2 = std::chrono::high_resolution_clock::now();


        cout<<"Total time by ReadOnly100Ops4Transactions::"<<endl;

        cout << std::chrono::duration_cast<std::chrono::seconds>(end_time2 - start_time2).count() << ":";
        cout << std::chrono::duration_cast<std::chrono::microseconds>(end_time2 - start_time2).count() << ":"<<endl;


    }

    BOOST_AUTO_TEST_CASE(ReadOnly100Ops8Transactions)
    {
        cout << "ReadOnly100Ops4Transactions" << endl;
        ///Iterator Operation on TupleContainer
        auto findKeys1 = [](ARTTupleContainer &ARTWithTuples, size_t id, std::string &status) {
            std::vector<void *> writeSet;
            std::vector<void *> ReadSet;


        random::mt19937 rng(current_time_nanoseconds());
        random::uniform_int_distribution<> randomKeys1(0,25000);
        //cout << randomKeys(rng) << endl;
        int totalCachedMissed=0;
        for (int i = 0; i < 100; i++)
        {
            char* keysToFind = KeysToStore[randomKeys1(rng)];
            auto val = ARTWithTuples.findValueByKey(keysToFind);


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
                BOOST_TEST(tp.getAttribute<4>() == (vectorValues[index].getAttribute<1>()) / 100.0);
            }
        }

        cout<<"Total Cached missed out of 100 random keys ="<<totalCachedMissed<<" by transaction#"<<id<<endl;
    };

    auto findKeys2 = [](ARTTupleContainer &ARTWithTuples, size_t id, std::string &status) {
        std::vector<void *> writeSet;
        std::vector<void *> ReadSet;


        random::mt19937 rng(current_time_nanoseconds());
        random::uniform_int_distribution<> randomKeys1(25000,50000);
        //cout << randomKeys(rng) << endl;
        int totalCachedMissed=0;
        for (int i = 0; i < 100; i++)
        {
            char* keysToFind = KeysToStore[randomKeys1(rng)];
            auto val = ARTWithTuples.findValueByKey(keysToFind);


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
                BOOST_TEST(tp.getAttribute<4>() == (vectorValues[index].getAttribute<1>()) / 100.0);
            }

        }

        cout<<"Total Cached missed out of 100 random keys ="<<totalCachedMissed<<" by transaction#"<<id<<endl;

    };

    auto findKeys3 = [](ARTTupleContainer &ARTWithTuples, size_t id, std::string &status) {
        std::vector<void *> writeSet;
        std::vector<void *> ReadSet;


        random::mt19937 rng(current_time_nanoseconds());
        random::uniform_int_distribution<> randomKeys1(50000,75000);
        //cout << randomKeys(rng) << endl;
        int totalCachedMissed=0;
        for (int i = 0; i < 100; i++)
        {
            char* keysToFind = KeysToStore[randomKeys1(rng)];
            auto val = ARTWithTuples.findValueByKey(keysToFind);


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
                BOOST_TEST(tp.getAttribute<4>() == (vectorValues[index].getAttribute<1>()) / 100.0);
            }
        }

        cout<<"Total Cached missed out of 100 random keys ="<<totalCachedMissed<<" by transaction#"<<id<<endl;
    };

    auto findKeys4 = [](ARTTupleContainer &ARTWithTuples, size_t id, std::string &status) {
        std::vector<void *> writeSet;
        std::vector<void *> ReadSet;


        random::mt19937 rng(current_time_nanoseconds());
        random::uniform_int_distribution<> randomKeys1(75000,100000);
        //cout << randomKeys(rng) << endl;
        int totalCachedMissed=0;
        for (int i = 0; i < 100; i++)
        {
            char* keysToFind = KeysToStore[randomKeys1(rng)];
            auto val = ARTWithTuples.findValueByKey(keysToFind);


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
                BOOST_TEST(tp.getAttribute<4>() == (vectorValues[index].getAttribute<1>()) / 100.0);
            }

        }

        cout<<"Total Cached missed out of 100 random keys ="<<totalCachedMissed<<" by transaction#"<<id<<endl;

    };

    auto findKeys5 = [](ARTTupleContainer &ARTWithTuples, size_t id, std::string &status) {
        std::vector<void *> writeSet;
        std::vector<void *> ReadSet;


        random::mt19937 rng(current_time_nanoseconds());
        random::uniform_int_distribution<> randomKeys1(100000,125000);
        //cout << randomKeys(rng) << endl;
        int totalCachedMissed=0;
        for (int i = 0; i < 100; i++)
        {
            char* keysToFind = KeysToStore[randomKeys1(rng)];
            auto val = ARTWithTuples.findValueByKey(keysToFind);


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
                BOOST_TEST(tp.getAttribute<4>() == (vectorValues[index].getAttribute<1>()) / 100.0);
            }
        }

        cout<<"Total Cached missed out of 100 random keys ="<<totalCachedMissed<<" by transaction#"<<id<<endl;
    };

    auto findKeys6 = [](ARTTupleContainer &ARTWithTuples, size_t id, std::string &status) {
        std::vector<void *> writeSet;
        std::vector<void *> ReadSet;


        random::mt19937 rng(current_time_nanoseconds());
        random::uniform_int_distribution<> randomKeys1(125000,150000);
        //cout << randomKeys(rng) << endl;
        int totalCachedMissed=0;
        for (int i = 0; i < 100; i++)
        {
            char* keysToFind = KeysToStore[randomKeys1(rng)];
            auto val = ARTWithTuples.findValueByKey(keysToFind);


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
                BOOST_TEST(tp.getAttribute<4>() == (vectorValues[index].getAttribute<1>()) / 100.0);
            }

        }

        cout<<"Total Cached missed out of 100 random keys ="<<totalCachedMissed<<" by transaction#"<<id<<endl;

    };

    auto findKeys7 = [](ARTTupleContainer &ARTWithTuples, size_t id, std::string &status) {
        std::vector<void *> writeSet;
        std::vector<void *> ReadSet;


        random::mt19937 rng(current_time_nanoseconds());
        random::uniform_int_distribution<> randomKeys1(150000,175000);
        //cout << randomKeys(rng) << endl;
        int totalCachedMissed=0;
        for (int i = 0; i < 100; i++)
        {
            char* keysToFind = KeysToStore[randomKeys1(rng)];
            auto val = ARTWithTuples.findValueByKey(keysToFind);


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
                BOOST_TEST(tp.getAttribute<4>() == (vectorValues[index].getAttribute<1>()) / 100.0);
            }
        }

        cout<<"Total Cached missed out of 100 random keys ="<<totalCachedMissed<<" by transaction#"<<id<<endl;
    };

    auto findKeys8 = [](ARTTupleContainer &ARTWithTuples, size_t id, std::string &status) {
        std::vector<void *> writeSet;
        std::vector<void *> ReadSet;


        random::mt19937 rng(current_time_nanoseconds());
        random::uniform_int_distribution<> randomKeys1(175000,200000);
        //cout << randomKeys(rng) << endl;
        int totalCachedMissed=0;
        for (int i = 0; i < 100; i++)
        {
            char* keysToFind = KeysToStore[randomKeys1(rng)];
            auto val = ARTWithTuples.findValueByKey(keysToFind);


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
                BOOST_TEST(tp.getAttribute<4>() == (vectorValues[index].getAttribute<1>()) / 100.0);
            }

        }

        cout<<"Total Cached missed out of 100 random keys ="<<totalCachedMissed<<" by transaction#"<<id<<endl;

    };

    auto start_time2 = std::chrono::high_resolution_clock::now();
    Transaction<TableOperationOnTupleFunc, ARTTupleContainer> *t1 = new Transaction<TableOperationOnTupleFunc, ARTTupleContainer>(
            findKeys1, *ARTable1);
    Transaction<TableOperationOnTupleFunc, ARTTupleContainer> *t2 = new Transaction<TableOperationOnTupleFunc, ARTTupleContainer>(
            findKeys2, *ARTable1);
    Transaction<TableOperationOnTupleFunc, ARTTupleContainer> *t3 = new Transaction<TableOperationOnTupleFunc, ARTTupleContainer>(
            findKeys3, *ARTable1);
    Transaction<TableOperationOnTupleFunc, ARTTupleContainer> *t4 = new Transaction<TableOperationOnTupleFunc, ARTTupleContainer>(
            findKeys4, *ARTable1);
    Transaction<TableOperationOnTupleFunc, ARTTupleContainer> *t5= new Transaction<TableOperationOnTupleFunc, ARTTupleContainer>(
            findKeys5, *ARTable1);
    Transaction<TableOperationOnTupleFunc, ARTTupleContainer> *t6 = new Transaction<TableOperationOnTupleFunc, ARTTupleContainer>(
            findKeys6, *ARTable1);
    Transaction<TableOperationOnTupleFunc, ARTTupleContainer> *t7 = new Transaction<TableOperationOnTupleFunc, ARTTupleContainer>(
            findKeys7, *ARTable1);
    Transaction<TableOperationOnTupleFunc, ARTTupleContainer> *t8 = new Transaction<TableOperationOnTupleFunc, ARTTupleContainer>(
            findKeys8, *ARTable1);
    t1->CollectTransaction();
    t2->CollectTransaction();
    t3->CollectTransaction();
    t4->CollectTransaction();
    t5->CollectTransaction();
    t6->CollectTransaction();
    t7->CollectTransaction();
    t8->CollectTransaction();
    auto end_time2 = std::chrono::high_resolution_clock::now();


    cout<<"Total time by ReadOnly100Ops8Transactions::"<<endl;

    cout << std::chrono::duration_cast<std::chrono::seconds>(end_time2 - start_time2).count() << ":";
    cout << std::chrono::duration_cast<std::chrono::microseconds>(end_time2 - start_time2).count() << ":"<<endl;


}

    BOOST_AUTO_TEST_CASE(ReadOnly10000Ops2Transactions)
    {
        cout << "ReadOnly10000Ops2Transactions" << endl;
        ///Iterator Operation on TupleContainer
        auto findKeys1 = [](ARTTupleContainer &ARTWithTuples, size_t id, std::string &status) {
            std::vector<void *> writeSet;
            std::vector<void *> ReadSet;


            random::mt19937 rng(current_time_nanoseconds());
            random::uniform_int_distribution<> randomKeys1(0,100000);
            //cout << randomKeys(rng) << endl;
            int totalCachedMissed=0;
            for (int i = 0; i < 10000; i++)
            {
                char* keysToFind = KeysToStore[randomKeys1(rng)];
                auto val = ARTWithTuples.findValueByKey(keysToFind);


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
                    BOOST_TEST(tp.getAttribute<4>() == (vectorValues[index].getAttribute<1>()) / 100.0);
                }
            }

            cout<<"Total Cached missed out of 10000 random keys ="<<totalCachedMissed<<" by transaction#"<<id<<endl;
        };

        auto findKeys2 = [](ARTTupleContainer &ARTWithTuples, size_t id, std::string &status) {
            std::vector<void *> writeSet;
            std::vector<void *> ReadSet;


            random::mt19937 rng(current_time_nanoseconds());
            random::uniform_int_distribution<> randomKeys1(100000,200000);
            //cout << randomKeys(rng) << endl;
            int totalCachedMissed=0;
            for (int i = 0; i < 10000; i++)
            {
                char* keysToFind = KeysToStore[randomKeys1(rng)];
                auto val = ARTWithTuples.findValueByKey(keysToFind);


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
                    BOOST_TEST(tp.getAttribute<4>() == (vectorValues[index].getAttribute<1>()) / 100.0);
                }

            }

            cout<<"Total Cached missed out of 10000 random keys ="<<totalCachedMissed<<" by transaction#"<<id<<endl;

        };

        auto start_time2 = std::chrono::high_resolution_clock::now();
        Transaction<TableOperationOnTupleFunc, ARTTupleContainer> *t1 = new Transaction<TableOperationOnTupleFunc, ARTTupleContainer>(
                findKeys1, *ARTable1);
        Transaction<TableOperationOnTupleFunc, ARTTupleContainer> *t2 = new Transaction<TableOperationOnTupleFunc, ARTTupleContainer>(
                findKeys2, *ARTable1);
        t1->CollectTransaction();
        t2->CollectTransaction();
        auto end_time2 = std::chrono::high_resolution_clock::now();


        cout<<"Total time by ReadOnly10000Ops2Transactions::"<<endl;

        cout << std::chrono::duration_cast<std::chrono::seconds>(end_time2 - start_time2).count() << ":";
        cout << std::chrono::duration_cast<std::chrono::microseconds>(end_time2 - start_time2).count() << ":"<<endl;


    }

    BOOST_AUTO_TEST_CASE(ReadOnly10000Ops4Transactions)
    {
        cout << "ReadOnly10000Ops4Transactions" << endl;
        ///Iterator Operation on TupleContainer
        auto findKeys1 = [](ARTTupleContainer &ARTWithTuples, size_t id, std::string &status) {
            std::vector<void *> writeSet;
            std::vector<void *> ReadSet;


            random::mt19937 rng(current_time_nanoseconds());
            random::uniform_int_distribution<> randomKeys1(0,50000);
            //cout << randomKeys(rng) << endl;
            int totalCachedMissed=0;
            for (int i = 0; i < 10000; i++)
            {
                char* keysToFind = KeysToStore[randomKeys1(rng)];
                auto val = ARTWithTuples.findValueByKey(keysToFind);


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
                    BOOST_TEST(tp.getAttribute<4>() == (vectorValues[index].getAttribute<1>()) / 100.0);
                }
            }

            cout<<"Total Cached missed out of 10000 random keys ="<<totalCachedMissed<<" by transaction#"<<id<<endl;
        };

        auto findKeys2 = [](ARTTupleContainer &ARTWithTuples, size_t id, std::string &status) {
            std::vector<void *> writeSet;
            std::vector<void *> ReadSet;


            random::mt19937 rng(current_time_nanoseconds());
            random::uniform_int_distribution<> randomKeys1(50000,100000);
            //cout << randomKeys(rng) << endl;
            int totalCachedMissed=0;
            for (int i = 0; i < 10000; i++)
            {
                char* keysToFind = KeysToStore[randomKeys1(rng)];
                auto val = ARTWithTuples.findValueByKey(keysToFind);


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
                    BOOST_TEST(tp.getAttribute<4>() == (vectorValues[index].getAttribute<1>()) / 100.0);
                }

            }

            cout<<"Total Cached missed out of 10000 random keys ="<<totalCachedMissed<<" by transaction#"<<id<<endl;

        };

        auto findKeys3 = [](ARTTupleContainer &ARTWithTuples, size_t id, std::string &status) {
            std::vector<void *> writeSet;
            std::vector<void *> ReadSet;


            random::mt19937 rng(current_time_nanoseconds());
            random::uniform_int_distribution<> randomKeys1(100000,150000);
            //cout << randomKeys(rng) << endl;
            int totalCachedMissed=0;
            for (int i = 0; i < 10000; i++)
            {
                char* keysToFind = KeysToStore[randomKeys1(rng)];
                auto val = ARTWithTuples.findValueByKey(keysToFind);


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
                    BOOST_TEST(tp.getAttribute<4>() == (vectorValues[index].getAttribute<1>()) / 100.0);
                }
            }

            cout<<"Total Cached missed out of 10000 random keys ="<<totalCachedMissed<<" by transaction#"<<id<<endl;
        };

        auto findKeys4 = [](ARTTupleContainer &ARTWithTuples, size_t id, std::string &status) {
            std::vector<void *> writeSet;
            std::vector<void *> ReadSet;


            random::mt19937 rng(current_time_nanoseconds());
            random::uniform_int_distribution<> randomKeys1(150000,200000);
            //cout << randomKeys(rng) << endl;
            int totalCachedMissed=0;
            for (int i = 0; i < 10000; i++)
            {
                char* keysToFind = KeysToStore[randomKeys1(rng)];
                auto val = ARTWithTuples.findValueByKey(keysToFind);


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
                    BOOST_TEST(tp.getAttribute<4>() == (vectorValues[index].getAttribute<1>()) / 100.0);
                }

            }

            cout<<"Total Cached missed out of 10000 random keys ="<<totalCachedMissed<<" by transaction#"<<id<<endl;

        };

        auto start_time2 = std::chrono::high_resolution_clock::now();
        Transaction<TableOperationOnTupleFunc, ARTTupleContainer> *t1 = new Transaction<TableOperationOnTupleFunc, ARTTupleContainer>(
                findKeys1, *ARTable1);
        Transaction<TableOperationOnTupleFunc, ARTTupleContainer> *t2 = new Transaction<TableOperationOnTupleFunc, ARTTupleContainer>(
                findKeys2, *ARTable1);

        Transaction<TableOperationOnTupleFunc, ARTTupleContainer> *t3 = new Transaction<TableOperationOnTupleFunc, ARTTupleContainer>(
                findKeys3, *ARTable1);
        Transaction<TableOperationOnTupleFunc, ARTTupleContainer> *t4 = new Transaction<TableOperationOnTupleFunc, ARTTupleContainer>(
                findKeys4, *ARTable1);
        t1->CollectTransaction();
        t2->CollectTransaction();
        t3->CollectTransaction();
        t4->CollectTransaction();
        auto end_time2 = std::chrono::high_resolution_clock::now();


        cout<<"Total time by ReadOnly10000Ops4Transactions::"<<endl;

        cout << std::chrono::duration_cast<std::chrono::seconds>(end_time2 - start_time2).count() << ":";
        cout << std::chrono::duration_cast<std::chrono::microseconds>(end_time2 - start_time2).count() << ":"<<endl;


    }

    BOOST_AUTO_TEST_CASE(ReadOnly10000Ops8Transactions)
    {
        cout << "ReadOnly10000Ops4Transactions" << endl;
        ///Iterator Operation on TupleContainer
        auto findKeys1 = [](ARTTupleContainer &ARTWithTuples, size_t id, std::string &status) {
            std::vector<void *> writeSet;
            std::vector<void *> ReadSet;


            random::mt19937 rng(current_time_nanoseconds());
            random::uniform_int_distribution<> randomKeys1(0,25000);
            //cout << randomKeys(rng) << endl;
            int totalCachedMissed=0;
            for (int i = 0; i < 10000; i++)
            {
                char* keysToFind = KeysToStore[randomKeys1(rng)];
                auto val = ARTWithTuples.findValueByKey(keysToFind);


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
                    BOOST_TEST(tp.getAttribute<4>() == (vectorValues[index].getAttribute<1>()) / 100.0);
                }
            }

            cout<<"Total Cached missed out of 100 random keys ="<<totalCachedMissed<<" by transaction#"<<id<<endl;
        };

        auto findKeys2 = [](ARTTupleContainer &ARTWithTuples, size_t id, std::string &status) {
            std::vector<void *> writeSet;
            std::vector<void *> ReadSet;


            random::mt19937 rng(current_time_nanoseconds());
            random::uniform_int_distribution<> randomKeys1(25000,50000);
            //cout << randomKeys(rng) << endl;
            int totalCachedMissed=0;
            for (int i = 0; i < 10000; i++)
            {
                char* keysToFind = KeysToStore[randomKeys1(rng)];
                auto val = ARTWithTuples.findValueByKey(keysToFind);


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
                    BOOST_TEST(tp.getAttribute<4>() == (vectorValues[index].getAttribute<1>()) / 100.0);
                }

            }

            cout<<"Total Cached missed out of 100 random keys ="<<totalCachedMissed<<" by transaction#"<<id<<endl;

        };

        auto findKeys3 = [](ARTTupleContainer &ARTWithTuples, size_t id, std::string &status) {
            std::vector<void *> writeSet;
            std::vector<void *> ReadSet;


            random::mt19937 rng(current_time_nanoseconds());
            random::uniform_int_distribution<> randomKeys1(50000,75000);
            //cout << randomKeys(rng) << endl;
            int totalCachedMissed=0;
            for (int i = 0; i < 10000; i++)
            {
                char* keysToFind = KeysToStore[randomKeys1(rng)];
                auto val = ARTWithTuples.findValueByKey(keysToFind);


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
                    BOOST_TEST(tp.getAttribute<4>() == (vectorValues[index].getAttribute<1>()) / 100.0);
                }
            }

            cout<<"Total Cached missed out of 100 random keys ="<<totalCachedMissed<<" by transaction#"<<id<<endl;
        };

        auto findKeys4 = [](ARTTupleContainer &ARTWithTuples, size_t id, std::string &status) {
            std::vector<void *> writeSet;
            std::vector<void *> ReadSet;


            random::mt19937 rng(current_time_nanoseconds());
            random::uniform_int_distribution<> randomKeys1(75000,100000);
            //cout << randomKeys(rng) << endl;
            int totalCachedMissed=0;
            for (int i = 0; i < 10000; i++)
            {
                char* keysToFind = KeysToStore[randomKeys1(rng)];
                auto val = ARTWithTuples.findValueByKey(keysToFind);


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
                    BOOST_TEST(tp.getAttribute<4>() == (vectorValues[index].getAttribute<1>()) / 100.0);
                }

            }

            cout<<"Total Cached missed out of 10000 random keys ="<<totalCachedMissed<<" by transaction#"<<id<<endl;

        };

        auto findKeys5 = [](ARTTupleContainer &ARTWithTuples, size_t id, std::string &status) {
            std::vector<void *> writeSet;
            std::vector<void *> ReadSet;


            random::mt19937 rng(current_time_nanoseconds());
            random::uniform_int_distribution<> randomKeys1(100000,125000);
            //cout << randomKeys(rng) << endl;
            int totalCachedMissed=0;
            for (int i = 0; i < 10000; i++)
            {
                char* keysToFind = KeysToStore[randomKeys1(rng)];
                auto val = ARTWithTuples.findValueByKey(keysToFind);


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
                    BOOST_TEST(tp.getAttribute<4>() == (vectorValues[index].getAttribute<1>()) / 100.0);
                }
            }

            cout<<"Total Cached missed out of 10000 random keys ="<<totalCachedMissed<<" by transaction#"<<id<<endl;
        };

        auto findKeys6 = [](ARTTupleContainer &ARTWithTuples, size_t id, std::string &status) {
            std::vector<void *> writeSet;
            std::vector<void *> ReadSet;


            random::mt19937 rng(current_time_nanoseconds());
            random::uniform_int_distribution<> randomKeys1(125000,150000);
            //cout << randomKeys(rng) << endl;
            int totalCachedMissed=0;
            for (int i = 0; i < 10000; i++)
            {
                char* keysToFind = KeysToStore[randomKeys1(rng)];
                auto val = ARTWithTuples.findValueByKey(keysToFind);


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
                    BOOST_TEST(tp.getAttribute<4>() == (vectorValues[index].getAttribute<1>()) / 100.0);
                }

            }

            cout<<"Total Cached missed out of 10000 random keys ="<<totalCachedMissed<<" by transaction#"<<id<<endl;

        };

        auto findKeys7 = [](ARTTupleContainer &ARTWithTuples, size_t id, std::string &status) {
            std::vector<void *> writeSet;
            std::vector<void *> ReadSet;


            random::mt19937 rng(current_time_nanoseconds());
            random::uniform_int_distribution<> randomKeys1(150000,175000);
            //cout << randomKeys(rng) << endl;
            int totalCachedMissed=0;
            for (int i = 0; i < 10000; i++)
            {
                char* keysToFind = KeysToStore[randomKeys1(rng)];
                auto val = ARTWithTuples.findValueByKey(keysToFind);


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
                    BOOST_TEST(tp.getAttribute<4>() == (vectorValues[index].getAttribute<1>()) / 100.0);
                }
            }

            cout<<"Total Cached missed out of 10000 random keys ="<<totalCachedMissed<<" by transaction#"<<id<<endl;
        };

        auto findKeys8 = [](ARTTupleContainer &ARTWithTuples, size_t id, std::string &status) {
            std::vector<void *> writeSet;
            std::vector<void *> ReadSet;


            random::mt19937 rng(current_time_nanoseconds());
            random::uniform_int_distribution<> randomKeys1(175000,200000);
            //cout << randomKeys(rng) << endl;
            int totalCachedMissed=0;
            for (int i = 0; i < 10000; i++)
            {
                char* keysToFind = KeysToStore[randomKeys1(rng)];
                auto val = ARTWithTuples.findValueByKey(keysToFind);


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
                    BOOST_TEST(tp.getAttribute<4>() == (vectorValues[index].getAttribute<1>()) / 100.0);
                }

            }

            cout<<"Total Cached missed out of 10000 random keys ="<<totalCachedMissed<<" by transaction#"<<id<<endl;

        };

        auto start_time2 = std::chrono::high_resolution_clock::now();
        Transaction<TableOperationOnTupleFunc, ARTTupleContainer> *t1 = new Transaction<TableOperationOnTupleFunc, ARTTupleContainer>(
                findKeys1, *ARTable1);
        Transaction<TableOperationOnTupleFunc, ARTTupleContainer> *t2 = new Transaction<TableOperationOnTupleFunc, ARTTupleContainer>(
                findKeys2, *ARTable1);
        Transaction<TableOperationOnTupleFunc, ARTTupleContainer> *t3 = new Transaction<TableOperationOnTupleFunc, ARTTupleContainer>(
                findKeys3, *ARTable1);
        Transaction<TableOperationOnTupleFunc, ARTTupleContainer> *t4 = new Transaction<TableOperationOnTupleFunc, ARTTupleContainer>(
                findKeys4, *ARTable1);
        Transaction<TableOperationOnTupleFunc, ARTTupleContainer> *t5= new Transaction<TableOperationOnTupleFunc, ARTTupleContainer>(
                findKeys5, *ARTable1);
        Transaction<TableOperationOnTupleFunc, ARTTupleContainer> *t6 = new Transaction<TableOperationOnTupleFunc, ARTTupleContainer>(
                findKeys6, *ARTable1);
        Transaction<TableOperationOnTupleFunc, ARTTupleContainer> *t7 = new Transaction<TableOperationOnTupleFunc, ARTTupleContainer>(
                findKeys7, *ARTable1);
        Transaction<TableOperationOnTupleFunc, ARTTupleContainer> *t8 = new Transaction<TableOperationOnTupleFunc, ARTTupleContainer>(
                findKeys8, *ARTable1);
        t1->CollectTransaction();
        t2->CollectTransaction();
        t3->CollectTransaction();
        t4->CollectTransaction();
        t5->CollectTransaction();
        t6->CollectTransaction();
        t7->CollectTransaction();
        t8->CollectTransaction();
        auto end_time2 = std::chrono::high_resolution_clock::now();


        cout<<"Total time by ReadOnly10000Ops8Transactions::"<<endl;

        cout << std::chrono::duration_cast<std::chrono::seconds>(end_time2 - start_time2).count() << ":";
        cout << std::chrono::duration_cast<std::chrono::microseconds>(end_time2 - start_time2).count() << ":"<<endl;


    }

BOOST_AUTO_TEST_SUITE_END()

