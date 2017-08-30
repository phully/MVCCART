#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE MVCC_TEST

#include <iostream>
#include <boost/test/unit_test.hpp>
#include <boost/lexical_cast.hpp>
#include "mvcc/mvcc.hpp"
#include "Transactions/UpdateIntensiveTemplates.h"


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
     * 80% Updates and 20% reads. UpdateIntensiveTemplate
     * were used , where 80% updates stored in WriteSet
     * while reading randomly 20% of the write set keys
     *
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
        auto start_time2 = std::chrono::high_resolution_clock::now();

        Transaction<TransactionLambda, ARTTupleContainer> *t1 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateSmall, *ARTable1,std::make_pair(0, 200000));
        t1->CollectTransaction();
        auto end_time2 = std::chrono::high_resolution_clock::now();

        cout<<"Total time by UpdateIntensive100Ops1Transactions::"<<endl;
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

    BOOST_AUTO_TEST_CASE(UpdateIntensive100Ops16Transactions)
    {
        cout << "UpdateIntensive100Ops16Transactions" << endl;
        auto start_time2 = std::chrono::high_resolution_clock::now();

        Transaction<TransactionLambda, ARTTupleContainer> *t1 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateSmall, *ARTable1,std::make_pair(0, 12500));
        Transaction<TransactionLambda, ARTTupleContainer> *t2 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateSmall, *ARTable1,std::make_pair(12500, 25000));
        Transaction<TransactionLambda, ARTTupleContainer> *t3 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateSmall, *ARTable1,std::make_pair(25000, 37500));
        Transaction<TransactionLambda, ARTTupleContainer> *t4 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateSmall, *ARTable1,std::make_pair(37500, 50000));
        Transaction<TransactionLambda, ARTTupleContainer> *t5 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateSmall, *ARTable1,std::make_pair(50000, 62500));
        Transaction<TransactionLambda, ARTTupleContainer> *t6 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateSmall, *ARTable1,std::make_pair(62500, 75000));
        Transaction<TransactionLambda, ARTTupleContainer> *t7 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateSmall, *ARTable1,std::make_pair(75000, 87500));
        Transaction<TransactionLambda, ARTTupleContainer> *t8 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateSmall, *ARTable1,std::make_pair(87500, 100000));

        Transaction<TransactionLambda, ARTTupleContainer> *t9 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateSmall, *ARTable1,std::make_pair(100000, 112500));
        Transaction<TransactionLambda, ARTTupleContainer> *t10 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateSmall, *ARTable1,std::make_pair(112500, 125000));
        Transaction<TransactionLambda, ARTTupleContainer> *t11 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateSmall, *ARTable1,std::make_pair(125000, 137500));
        Transaction<TransactionLambda, ARTTupleContainer> *t12 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateSmall, *ARTable1,std::make_pair(137500, 150000));
        Transaction<TransactionLambda, ARTTupleContainer> *t13 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateSmall, *ARTable1,std::make_pair(150000, 162500));
        Transaction<TransactionLambda, ARTTupleContainer> *t14 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateSmall, *ARTable1,std::make_pair(162500, 175000));
        Transaction<TransactionLambda, ARTTupleContainer> *t15 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateSmall, *ARTable1,std::make_pair(175000, 187500));
        Transaction<TransactionLambda, ARTTupleContainer> *t16 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateSmall, *ARTable1,std::make_pair(187500, 200000));

        t1->CollectTransaction();
        t2->CollectTransaction();
        t3->CollectTransaction();
        t4->CollectTransaction();
        t5->CollectTransaction();
        t6->CollectTransaction();
        t7->CollectTransaction();
        t8->CollectTransaction();

        t9->CollectTransaction();
        t10->CollectTransaction();
        t11->CollectTransaction();
        t12->CollectTransaction();
        t13->CollectTransaction();
        t14->CollectTransaction();
        t15->CollectTransaction();
        t16->CollectTransaction();
        auto end_time2 = std::chrono::high_resolution_clock::now();

        cout<<"Total time by UpdateIntensive100Ops16Transactions::"<<endl;
        cout << std::chrono::duration_cast<std::chrono::seconds>(end_time2 - start_time2).count() << ":";
        cout << std::chrono::duration_cast<std::chrono::microseconds>(end_time2 - start_time2).count() << ":"<<endl;


    }

    BOOST_AUTO_TEST_CASE(UpdateIntensive100Ops32Transactions)
    {
        cout << "UpdateIntensive100Ops32Transactions" << endl;
        auto start_time2 = std::chrono::high_resolution_clock::now();


        Transaction<TransactionLambda, ARTTupleContainer> *t1 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateSmall, *ARTable1,std::make_pair(0, 6250));

        Transaction<TransactionLambda, ARTTupleContainer> *t2 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateSmall, *ARTable1,std::make_pair(6250, 12500));

        Transaction<TransactionLambda, ARTTupleContainer> *t3 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateSmall, *ARTable1,std::make_pair(12500, 18750));

        Transaction<TransactionLambda, ARTTupleContainer> *t4 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateSmall, *ARTable1,std::make_pair(18750, 25000));

        Transaction<TransactionLambda, ARTTupleContainer> *t5 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateSmall, *ARTable1,std::make_pair(25000, 31250));

        Transaction<TransactionLambda, ARTTupleContainer> *t6 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateSmall, *ARTable1,std::make_pair(31250, 37500));

        Transaction<TransactionLambda, ARTTupleContainer> *t7 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateSmall, *ARTable1,std::make_pair(37500, 43750));

        Transaction<TransactionLambda, ARTTupleContainer> *t8 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateSmall, *ARTable1,std::make_pair(43750, 50000));

        Transaction<TransactionLambda, ARTTupleContainer> *t9 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateSmall, *ARTable1,std::make_pair(50000, 56250));

        Transaction<TransactionLambda, ARTTupleContainer> *t10 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateSmall, *ARTable1,std::make_pair(56250, 62500));

        Transaction<TransactionLambda, ARTTupleContainer> *t11 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateSmall, *ARTable1,std::make_pair(62500, 68750));

        Transaction<TransactionLambda, ARTTupleContainer> *t12 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateSmall, *ARTable1,std::make_pair(68750, 75000));

        Transaction<TransactionLambda, ARTTupleContainer> *t13 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateSmall, *ARTable1,std::make_pair(75000, 81250));

        Transaction<TransactionLambda, ARTTupleContainer> *t14 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateSmall, *ARTable1,std::make_pair(81250, 87500));

        Transaction<TransactionLambda, ARTTupleContainer> *t15 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateSmall, *ARTable1,std::make_pair(87500, 93750));

        Transaction<TransactionLambda, ARTTupleContainer> *t16 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateSmall, *ARTable1,std::make_pair(93750, 100000));

        Transaction<TransactionLambda, ARTTupleContainer> *t17 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateSmall, *ARTable1,std::make_pair(100000, 106250));

        Transaction<TransactionLambda, ARTTupleContainer> *t18 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateSmall, *ARTable1,std::make_pair(106250, 112500));

        Transaction<TransactionLambda, ARTTupleContainer> *t19 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateSmall, *ARTable1,std::make_pair(112500, 118750));

        Transaction<TransactionLambda, ARTTupleContainer> *t20 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateSmall, *ARTable1,std::make_pair(118750, 125000));

        Transaction<TransactionLambda, ARTTupleContainer> *t21 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateSmall, *ARTable1,std::make_pair(125000, 131250));

        Transaction<TransactionLambda, ARTTupleContainer> *t22 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateSmall, *ARTable1,std::make_pair(131250, 137500));

        Transaction<TransactionLambda, ARTTupleContainer> *t23 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateSmall, *ARTable1,std::make_pair(137500, 143750));

        Transaction<TransactionLambda, ARTTupleContainer> *t24 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateSmall, *ARTable1,std::make_pair(143750, 150000));

        Transaction<TransactionLambda, ARTTupleContainer> *t25 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateSmall, *ARTable1,std::make_pair(150000, 156250));

        Transaction<TransactionLambda, ARTTupleContainer> *t26 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateSmall, *ARTable1,std::make_pair(156250, 162500));

        Transaction<TransactionLambda, ARTTupleContainer> *t27 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateSmall, *ARTable1,std::make_pair(162500, 168750));

        Transaction<TransactionLambda, ARTTupleContainer> *t28 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateSmall, *ARTable1,std::make_pair(168750, 175000));

        Transaction<TransactionLambda, ARTTupleContainer> *t29 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateSmall, *ARTable1,std::make_pair(175000, 181250));

        Transaction<TransactionLambda, ARTTupleContainer> *t30 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateSmall, *ARTable1,std::make_pair(181250, 187500));

        Transaction<TransactionLambda, ARTTupleContainer> *t31 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateSmall, *ARTable1,std::make_pair(187500, 193750));

        Transaction<TransactionLambda, ARTTupleContainer> *t32 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateSmall, *ARTable1,std::make_pair(193750, 200000));

        t1->CollectTransaction();
        t2->CollectTransaction();
        t3->CollectTransaction();
        t4->CollectTransaction();
        t5->CollectTransaction();
        t6->CollectTransaction();
        t7->CollectTransaction();
        t8->CollectTransaction();

        t9->CollectTransaction();
        t10->CollectTransaction();
        t11->CollectTransaction();
        t12->CollectTransaction();
        t13->CollectTransaction();
        t14->CollectTransaction();
        t15->CollectTransaction();
        t16->CollectTransaction();

        t17->CollectTransaction();
        t18->CollectTransaction();
        t19->CollectTransaction();
        t20->CollectTransaction();
        t21->CollectTransaction();
        t22->CollectTransaction();
        t23->CollectTransaction();
        t24->CollectTransaction();

        t25->CollectTransaction();
        t26->CollectTransaction();
        t27->CollectTransaction();
        t28->CollectTransaction();
        t29->CollectTransaction();
        t30->CollectTransaction();
        t31->CollectTransaction();
        t32->CollectTransaction();
        auto end_time2 = std::chrono::high_resolution_clock::now();

        cout<<"Total time by UpdateIntensive100Ops32Transactions::"<<endl;
        cout << std::chrono::duration_cast<std::chrono::seconds>(end_time2 - start_time2).count() << ":";
        cout << std::chrono::duration_cast<std::chrono::microseconds>(end_time2 - start_time2).count() << ":"<<endl;


    }


    BOOST_AUTO_TEST_CASE(UpdateIntensive10000Ops1Transactions)
    {
        cout << "UpdateIntensive10000Ops1Transactions" << endl;
        auto start_time2 = std::chrono::high_resolution_clock::now();

        Transaction<TransactionLambda, ARTTupleContainer> *t1 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateMedium, *ARTable1,std::make_pair(0, 200000));
        t1->CollectTransaction();

        auto end_time2 = std::chrono::high_resolution_clock::now();

        cout<<"Total time by UpdateIntensive10000Ops1Transactions::"<<endl;
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

    BOOST_AUTO_TEST_CASE(UpdateIntensive1000Ops16Transactions)
    {
        cout << "UpdateIntensive100Ops16Transactions" << endl;
        auto start_time2 = std::chrono::high_resolution_clock::now();

        Transaction<TransactionLambda, ARTTupleContainer> *t1 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateMedium, *ARTable1,std::make_pair(0, 12500));
        Transaction<TransactionLambda, ARTTupleContainer> *t2 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateMedium, *ARTable1,std::make_pair(12500, 25000));
        Transaction<TransactionLambda, ARTTupleContainer> *t3 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateMedium, *ARTable1,std::make_pair(25000, 37500));
        Transaction<TransactionLambda, ARTTupleContainer> *t4 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateMedium, *ARTable1,std::make_pair(37500, 50000));
        Transaction<TransactionLambda, ARTTupleContainer> *t5 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateMedium, *ARTable1,std::make_pair(50000, 62500));
        Transaction<TransactionLambda, ARTTupleContainer> *t6 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateMedium, *ARTable1,std::make_pair(62500, 75000));
        Transaction<TransactionLambda, ARTTupleContainer> *t7 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateMedium, *ARTable1,std::make_pair(75000, 87500));
        Transaction<TransactionLambda, ARTTupleContainer> *t8 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateMedium, *ARTable1,std::make_pair(87500, 100000));

        Transaction<TransactionLambda, ARTTupleContainer> *t9 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateMedium, *ARTable1,std::make_pair(100000, 112500));
        Transaction<TransactionLambda, ARTTupleContainer> *t10 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateMedium, *ARTable1,std::make_pair(112500, 125000));
        Transaction<TransactionLambda, ARTTupleContainer> *t11 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateMedium, *ARTable1,std::make_pair(125000, 137500));
        Transaction<TransactionLambda, ARTTupleContainer> *t12 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateMedium, *ARTable1,std::make_pair(137500, 150000));
        Transaction<TransactionLambda, ARTTupleContainer> *t13 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateMedium, *ARTable1,std::make_pair(150000, 162500));
        Transaction<TransactionLambda, ARTTupleContainer> *t14 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateMedium, *ARTable1,std::make_pair(162500, 175000));
        Transaction<TransactionLambda, ARTTupleContainer> *t15 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateMedium, *ARTable1,std::make_pair(175000, 187500));
        Transaction<TransactionLambda, ARTTupleContainer> *t16 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateMedium, *ARTable1,std::make_pair(187500, 200000));

        t1->CollectTransaction();
        t2->CollectTransaction();
        t3->CollectTransaction();
        t4->CollectTransaction();
        t5->CollectTransaction();
        t6->CollectTransaction();
        t7->CollectTransaction();
        t8->CollectTransaction();

        t9->CollectTransaction();
        t10->CollectTransaction();
        t11->CollectTransaction();
        t12->CollectTransaction();
        t13->CollectTransaction();
        t14->CollectTransaction();
        t15->CollectTransaction();
        t16->CollectTransaction();
        auto end_time2 = std::chrono::high_resolution_clock::now();

        cout<<"Total time by UpdateIntensive100Ops16Transactions::"<<endl;
        cout << std::chrono::duration_cast<std::chrono::seconds>(end_time2 - start_time2).count() << ":";
        cout << std::chrono::duration_cast<std::chrono::microseconds>(end_time2 - start_time2).count() << ":"<<endl;


    }

    BOOST_AUTO_TEST_CASE(UpdateIntensive1000Ops32Transactions)
    {
        cout << "UpdateIntensive1000Ops32Transactions" << endl;
        auto start_time2 = std::chrono::high_resolution_clock::now();


        Transaction<TransactionLambda, ARTTupleContainer> *t1 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateMedium, *ARTable1,std::make_pair(0, 6250));

        Transaction<TransactionLambda, ARTTupleContainer> *t2 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateMedium, *ARTable1,std::make_pair(6250, 12500));

        Transaction<TransactionLambda, ARTTupleContainer> *t3 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateMedium, *ARTable1,std::make_pair(12500, 18750));

        Transaction<TransactionLambda, ARTTupleContainer> *t4 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateMedium, *ARTable1,std::make_pair(18750, 25000));

        Transaction<TransactionLambda, ARTTupleContainer> *t5 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateMedium, *ARTable1,std::make_pair(25000, 31250));

        Transaction<TransactionLambda, ARTTupleContainer> *t6 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateMedium, *ARTable1,std::make_pair(31250, 37500));

        Transaction<TransactionLambda, ARTTupleContainer> *t7 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateMedium, *ARTable1,std::make_pair(37500, 43750));

        Transaction<TransactionLambda, ARTTupleContainer> *t8 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateMedium, *ARTable1,std::make_pair(43750, 50000));

        Transaction<TransactionLambda, ARTTupleContainer> *t9 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateMedium, *ARTable1,std::make_pair(50000, 56250));

        Transaction<TransactionLambda, ARTTupleContainer> *t10 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateMedium, *ARTable1,std::make_pair(56250, 62500));

        Transaction<TransactionLambda, ARTTupleContainer> *t11 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateMedium, *ARTable1,std::make_pair(62500, 68750));

        Transaction<TransactionLambda, ARTTupleContainer> *t12 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateMedium, *ARTable1,std::make_pair(68750, 75000));

        Transaction<TransactionLambda, ARTTupleContainer> *t13 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateMedium, *ARTable1,std::make_pair(75000, 81250));

        Transaction<TransactionLambda, ARTTupleContainer> *t14 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateMedium, *ARTable1,std::make_pair(81250, 87500));

        Transaction<TransactionLambda, ARTTupleContainer> *t15 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateMedium, *ARTable1,std::make_pair(87500, 93750));

        Transaction<TransactionLambda, ARTTupleContainer> *t16 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateMedium, *ARTable1,std::make_pair(93750, 100000));

        Transaction<TransactionLambda, ARTTupleContainer> *t17 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateMedium, *ARTable1,std::make_pair(100000, 106250));

        Transaction<TransactionLambda, ARTTupleContainer> *t18 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateMedium, *ARTable1,std::make_pair(106250, 112500));

        Transaction<TransactionLambda, ARTTupleContainer> *t19 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateMedium, *ARTable1,std::make_pair(112500, 118750));

        Transaction<TransactionLambda, ARTTupleContainer> *t20 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateMedium, *ARTable1,std::make_pair(118750, 125000));

        Transaction<TransactionLambda, ARTTupleContainer> *t21 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateMedium, *ARTable1,std::make_pair(125000, 131250));

        Transaction<TransactionLambda, ARTTupleContainer> *t22 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateMedium, *ARTable1,std::make_pair(131250, 137500));

        Transaction<TransactionLambda, ARTTupleContainer> *t23 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateMedium, *ARTable1,std::make_pair(137500, 143750));

        Transaction<TransactionLambda, ARTTupleContainer> *t24 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateMedium, *ARTable1,std::make_pair(143750, 150000));

        Transaction<TransactionLambda, ARTTupleContainer> *t25 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateMedium, *ARTable1,std::make_pair(150000, 156250));

        Transaction<TransactionLambda, ARTTupleContainer> *t26 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateMedium, *ARTable1,std::make_pair(156250, 162500));

        Transaction<TransactionLambda, ARTTupleContainer> *t27 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateMedium, *ARTable1,std::make_pair(162500, 168750));

        Transaction<TransactionLambda, ARTTupleContainer> *t28 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateMedium, *ARTable1,std::make_pair(168750, 175000));

        Transaction<TransactionLambda, ARTTupleContainer> *t29 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateMedium, *ARTable1,std::make_pair(175000, 181250));

        Transaction<TransactionLambda, ARTTupleContainer> *t30 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateMedium, *ARTable1,std::make_pair(181250, 187500));

        Transaction<TransactionLambda, ARTTupleContainer> *t31 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateMedium, *ARTable1,std::make_pair(187500, 193750));

        Transaction<TransactionLambda, ARTTupleContainer> *t32 = new Transaction<TransactionLambda, ARTTupleContainer>(
                UpdateMedium, *ARTable1,std::make_pair(193750, 200000));

        t1->CollectTransaction();
        t2->CollectTransaction();
        t3->CollectTransaction();
        t4->CollectTransaction();
        t5->CollectTransaction();
        t6->CollectTransaction();
        t7->CollectTransaction();
        t8->CollectTransaction();

        t9->CollectTransaction();
        t10->CollectTransaction();
        t11->CollectTransaction();
        t12->CollectTransaction();
        t13->CollectTransaction();
        t14->CollectTransaction();
        t15->CollectTransaction();
        t16->CollectTransaction();

        t17->CollectTransaction();
        t18->CollectTransaction();
        t19->CollectTransaction();
        t20->CollectTransaction();
        t21->CollectTransaction();
        t22->CollectTransaction();
        t23->CollectTransaction();
        t24->CollectTransaction();

        t25->CollectTransaction();
        t26->CollectTransaction();
        t27->CollectTransaction();
        t28->CollectTransaction();
        t29->CollectTransaction();
        t30->CollectTransaction();
        t31->CollectTransaction();
        t32->CollectTransaction();
        auto end_time2 = std::chrono::high_resolution_clock::now();

        cout<<"Total time by UpdateIntensive1000Ops32Transactions::"<<endl;
        cout << std::chrono::duration_cast<std::chrono::seconds>(end_time2 - start_time2).count() << ":";
        cout << std::chrono::duration_cast<std::chrono::microseconds>(end_time2 - start_time2).count() << ":"<<endl;


    }

BOOST_AUTO_TEST_SUITE_END()