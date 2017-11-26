#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE MVCC_TEST

#include <iostream>
#include <boost/test/unit_test.hpp>
#include <boost/lexical_cast.hpp>
#include "mvcc/mvcc.hpp"
#include "Transactions/UpdateIntensiveTemplatesGC.hpp"
#include "generated/settings.h"

using namespace std;

void RESET_AND_DELETE(ARTTupleContainer& ART )
{
    ART.DestroyAdaptiveRadixTreeTable();
    reset_transaction_ID();

}
auto ARTable1 =  new ARTTupleContainer();

auto GarbageCollection = []()
{
    cout<<"Starting GC"<<endl;
    ARTable1->GC();
};



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
     * 80% Updates and 20% reads. UpdateIntensiveTemplate
     * were used , where 80% updates stored in WriteSet
     * while reading randomly 20% of the write set keys
     * to evaluate if the updates were correct.
    */


    BOOST_AUTO_TEST_CASE(Write200000keys)
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
                Transaction<TableOperationOnTupleFunc,ARTTupleContainer>(WriteKeys1,*ARTable1);
        writerTransaction->CollectTransaction();
        auto end_timeWriter= std::chrono::high_resolution_clock::now();

        cout<<endl<<"Single Thread Writer Time->";
        cout << std::chrono::duration_cast<std::chrono::seconds>(end_timeWriter - start_timeWriter).count() << ":";
        cout << std::chrono::duration_cast<std::chrono::microseconds>(end_timeWriter - start_timeWriter).count() << ":"<<endl;
    }

    BOOST_AUTO_TEST_CASE(UpdateIntensive100Ops1Transactions)
    {
        cout << "UpdateIntensive100Ops1Transactions" << endl;
        myEpochGlobal.registerGCTrigger(GarbageCollection);
        myEpochGlobal.StartEpochThread();
        auto start_time2 = std::chrono::high_resolution_clock::now();

        Transaction<TransactionLambda, ARTTupleContainer> *t1 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateSmall, *ARTable1,std::make_pair(0, 200000));
        t1->CollectTransaction();
        auto end_time2 = std::chrono::high_resolution_clock::now();

        cout<<"Total time by UpdateIntensive100Ops1Transactions::"<<endl;
        cout << std::chrono::duration_cast<std::chrono::seconds>(end_time2 - start_time2).count() << ":";
        cout << std::chrono::duration_cast<std::chrono::microseconds>(end_time2 - start_time2).count() << ":"<<endl;

    }

    BOOST_AUTO_TEST_CASE(Delete100Keys)
    {
        cout << "Deleting keys" << endl;
        myEpochGlobal.registerGCTrigger(GarbageCollection);
        myEpochGlobal.StartEpochThread();

        auto start_time2 = std::chrono::high_resolution_clock::now();

        Transaction<TransactionLambda, ARTTupleContainer> *t1 = new Transaction<TransactionLambda, ARTTupleContainer>(
                deleteRandomKeys, *ARTable1,std::make_pair(0, 1000));
        t1->CollectTransaction();
        auto end_time2 = std::chrono::high_resolution_clock::now();

        cout<<"Total time by Deleting100keysPerSingleTransactions::"<<endl;
        cout << std::chrono::duration_cast<std::chrono::seconds>(end_time2 - start_time2).count() << ":";
        cout << std::chrono::duration_cast<std::chrono::microseconds>(end_time2 - start_time2).count() << ":"<<endl;

    }

    BOOST_AUTO_TEST_CASE(UpdateIntensive100Ops2Transactions)
    {
        cout << "UpdateIntensive100Ops2Transactions" << endl;
        auto start_time2 = std::chrono::high_resolution_clock::now();

        Transaction<TransactionLambda, ARTTupleContainer> *t1 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateSmall, *ARTable1,std::make_pair(0, 100000));
        Transaction<TransactionLambda, ARTTupleContainer> *t2 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateSmall, *ARTable1,std::make_pair(100000, 200000));

        t1->CollectTransaction();
        t2->CollectTransaction();

        auto end_time2 = std::chrono::high_resolution_clock::now();

        cout<<"Total time by UpdateIntensive100Ops2Transactions::"<<endl;
        cout << std::chrono::duration_cast<std::chrono::seconds>(end_time2 - start_time2).count() << ":";
        cout << std::chrono::duration_cast<std::chrono::microseconds>(end_time2 - start_time2).count() << ":"<<endl;

    }


BOOST_AUTO_TEST_SUITE_END()