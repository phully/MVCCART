#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE MVCC_TEST

#include <iostream>
#include <boost/test/unit_test.hpp>
#include <boost/lexical_cast.hpp>
#include "mvcc/mvcc.hpp"
#include "ART/ARTFULCpp.h"
#include "Transactions/UpdateIntensiveTemplates.h"
#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <check.h>

using namespace std;




auto ARTable1 =  new ARTTupleContainer();

void RESET_AND_DELETE(ARTTupleContainer& ART )
{
    ART.DestroyAdaptiveRadixTreeTable();
    reset_transaction_ID();

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
     * Testing Read Intesnsive workloads small
     * and long transactions. Each transaction
     * carries out 100 for small and 10000 for
     * long transactions. While scaling from 1 to 8
     * transactions. 80% Reads and 20% Updates
     * randomly from the the ReadSet vector obtained
     * from Reading values.
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

    BOOST_AUTO_TEST_CASE(UpdateIntensive100Ops4Transactions)
    {
        cout << "UpdateIntensive100Ops4Transactions" << endl;
        auto start_time2 = std::chrono::high_resolution_clock::now();

        Transaction<TransactionLambda, ARTTupleContainer> *t1 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateSmall, *ARTable1,std::make_pair(0, 50000));
        Transaction<TransactionLambda, ARTTupleContainer> *t2 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateSmall, *ARTable1,std::make_pair(50000, 100000));
        Transaction<TransactionLambda, ARTTupleContainer> *t3 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateSmall, *ARTable1,std::make_pair(100000, 150000));
        Transaction<TransactionLambda, ARTTupleContainer> *t4 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateSmall, *ARTable1,std::make_pair(150000, 200000));

        t1->CollectTransaction();
        t2->CollectTransaction();
        t3->CollectTransaction();
        t4->CollectTransaction();

        auto end_time2 = std::chrono::high_resolution_clock::now();

        cout<<"Total time by UpdateIntensive100Ops4Transactions::"<<endl;
        cout << std::chrono::duration_cast<std::chrono::seconds>(end_time2 - start_time2).count() << ":";
        cout << std::chrono::duration_cast<std::chrono::microseconds>(end_time2 - start_time2).count() << ":"<<endl;


    }

    BOOST_AUTO_TEST_CASE(UpdateIntensive100Ops8Transactions)
    {
        cout << "UpdateIntensive100Ops8Transactions" << endl;
        auto start_time2 = std::chrono::high_resolution_clock::now();

        Transaction<TransactionLambda, ARTTupleContainer> *t1 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateSmall, *ARTable1,std::make_pair(0, 25000));
        Transaction<TransactionLambda, ARTTupleContainer> *t2 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateSmall, *ARTable1,std::make_pair(25000, 50000));
        Transaction<TransactionLambda, ARTTupleContainer> *t3 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateSmall, *ARTable1,std::make_pair(50000, 75000));
        Transaction<TransactionLambda, ARTTupleContainer> *t4 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateSmall, *ARTable1,std::make_pair(75000, 100000));
        Transaction<TransactionLambda, ARTTupleContainer> *t5 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateSmall, *ARTable1,std::make_pair(100000, 125000));
        Transaction<TransactionLambda, ARTTupleContainer> *t6 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateSmall, *ARTable1,std::make_pair(125000, 150000));
        Transaction<TransactionLambda, ARTTupleContainer> *t7 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateSmall, *ARTable1,std::make_pair(150000, 175000));
        Transaction<TransactionLambda, ARTTupleContainer> *t8 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateSmall, *ARTable1,std::make_pair(175000, 200000));



        t1->CollectTransaction();
        t2->CollectTransaction();
        t3->CollectTransaction();
        t4->CollectTransaction();
        t5->CollectTransaction();
        t6->CollectTransaction();
        t7->CollectTransaction();
        t8->CollectTransaction();

        auto end_time2 = std::chrono::high_resolution_clock::now();

        cout<<"Total time by UpdateIntensive100Ops8Transactions::"<<endl;
        cout << std::chrono::duration_cast<std::chrono::seconds>(end_time2 - start_time2).count() << ":";
        cout << std::chrono::duration_cast<std::chrono::microseconds>(end_time2 - start_time2).count() << ":"<<endl;


    }

    BOOST_AUTO_TEST_CASE(UpdateIntensive10000Ops2Transactions)
    {
        cout << "UpdateIntensive10000Ops2Transactions" << endl;
        auto start_time2 = std::chrono::high_resolution_clock::now();

        Transaction<TransactionLambda, ARTTupleContainer> *t1 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateMedium, *ARTable1,std::make_pair(0, 100000));
        Transaction<TransactionLambda, ARTTupleContainer> *t2 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateMedium, *ARTable1,std::make_pair(100000, 200000));

        t1->CollectTransaction();
        t2->CollectTransaction();

        auto end_time2 = std::chrono::high_resolution_clock::now();

        cout<<"Total time by UpdateIntensive10000Ops2Transactions::"<<endl;
        cout << std::chrono::duration_cast<std::chrono::seconds>(end_time2 - start_time2).count() << ":";
        cout << std::chrono::duration_cast<std::chrono::microseconds>(end_time2 - start_time2).count() << ":"<<endl;


    }

    BOOST_AUTO_TEST_CASE(UpdateIntensive10000Ops4Transactions)
    {
        cout << "UpdateIntensive10000Ops4Transactions" << endl;
        auto start_time2 = std::chrono::high_resolution_clock::now();

        Transaction<TransactionLambda, ARTTupleContainer> *t1 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateMedium, *ARTable1,std::make_pair(0, 50000));
        Transaction<TransactionLambda, ARTTupleContainer> *t2 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateMedium, *ARTable1,std::make_pair(50000, 100000));
        Transaction<TransactionLambda, ARTTupleContainer> *t3 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateMedium, *ARTable1,std::make_pair(100000, 150000));
        Transaction<TransactionLambda, ARTTupleContainer> *t4 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateMedium, *ARTable1,std::make_pair(150000, 200000));

        t1->CollectTransaction();
        t2->CollectTransaction();
        t3->CollectTransaction();
        t4->CollectTransaction();

        auto end_time2 = std::chrono::high_resolution_clock::now();

        cout<<"Total time by UpdateIntensive10000Ops4Transactions::"<<endl;
        cout << std::chrono::duration_cast<std::chrono::seconds>(end_time2 - start_time2).count() << ":";
        cout << std::chrono::duration_cast<std::chrono::microseconds>(end_time2 - start_time2).count() << ":"<<endl;


    }

    BOOST_AUTO_TEST_CASE(UpdateIntensive10000Ops8Transactions)
    {
        cout << "UpdateIntensive10000Ops8Transactions" << endl;
        auto start_time2 = std::chrono::high_resolution_clock::now();

        Transaction<TransactionLambda, ARTTupleContainer> *t1 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateMedium, *ARTable1,std::make_pair(0, 25000));
        Transaction<TransactionLambda, ARTTupleContainer> *t2 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateMedium, *ARTable1,std::make_pair(25000, 50000));
        Transaction<TransactionLambda, ARTTupleContainer> *t3 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateMedium, *ARTable1,std::make_pair(50000, 75000));
        Transaction<TransactionLambda, ARTTupleContainer> *t4 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateMedium, *ARTable1,std::make_pair(75000, 100000));
        Transaction<TransactionLambda, ARTTupleContainer> *t5 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateMedium, *ARTable1,std::make_pair(100000, 125000));
        Transaction<TransactionLambda, ARTTupleContainer> *t6 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateMedium, *ARTable1,std::make_pair(125000, 150000));
        Transaction<TransactionLambda, ARTTupleContainer> *t7 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateMedium, *ARTable1,std::make_pair(150000, 175000));
        Transaction<TransactionLambda, ARTTupleContainer> *t8 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateMedium, *ARTable1,std::make_pair(175000, 200000));



        t1->CollectTransaction();
        t2->CollectTransaction();
        t3->CollectTransaction();
        t4->CollectTransaction();
        t5->CollectTransaction();
        t6->CollectTransaction();
        t7->CollectTransaction();
        t8->CollectTransaction();

        auto end_time2 = std::chrono::high_resolution_clock::now();

        cout<<"Total time by UpdateIntensive10000Ops8Transactions::"<<endl;
        cout << std::chrono::duration_cast<std::chrono::seconds>(end_time2 - start_time2).count() << ":";
        cout << std::chrono::duration_cast<std::chrono::microseconds>(end_time2 - start_time2).count() << ":"<<endl;


    }

BOOST_AUTO_TEST_SUITE_END()

