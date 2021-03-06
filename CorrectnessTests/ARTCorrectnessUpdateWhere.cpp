#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE MVCC_TEST


#include <iostream>
#include "generated/settings.h"
#include <boost/test/unit_test.hpp>
#include <boost/lexical_cast.hpp>
#include "mvcc/mvcc.hpp"
#include "ART/ArtCPP.hpp"
#include <condition_variable>
#include <future>
#include <random>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include <boost/random/variate_generator.hpp>
#include "fmt-master/fmt/format.h"
#include "Transactions/WriteOnlyTemplates.h"


using namespace std;



auto ARTableWithTuples =  new ARTTupleContainer();

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
        cout<<"keystr::"<<key<<endl;
        cout<<"found key::"<<val->value<<endl;
        /*auto tp = val->value;
        unsigned long index = tp.getAttribute<1>();
        cout<<"found val "<<tp.getAttribute<0>()<<endl;
        BOOST_TEST(tp.getAttribute<0>() == vectorValues[index].getAttribute<0>());
        BOOST_TEST(tp.getAttribute<1>() == vectorValues[index].getAttribute<1>());
        BOOST_TEST(tp.getAttribute<2>() == (vectorValues[index].getAttribute<2>()));
        //BOOST_TEST(tp.getAttribute<3>() == fmt::format("String/{}", keysToFind));
        BOOST_TEST(tp.getAttribute<4>() == (index) / 100.0);*/
        //BOOST_TEST(tp.getAttribute<3>() == fmt::format("String/{}", keysToFind));*/
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
     * Testing correctness on concurrent Writes 100% write intensive, scaled till 8 transactions in concurrent
    */

    BOOST_AUTO_TEST_CASE(WriteOnly1Transactions)
    {
        cout << "WriteOnly100Ops1Transactions" << endl;
        auto start_time2 = std::chrono::high_resolution_clock::now();

        Transaction<TransactionLambda, ARTTupleContainer> *t1 = new Transaction<TransactionLambda, ARTTupleContainer>(
                WriteOnly, *ARTableWithTuples,std::make_pair(0, 200000));
        t1->CollectTransaction();

        auto end_time2 = std::chrono::high_resolution_clock::now();

        cout<<"Total time by WriteOnly100Ops1Transactions::"<<endl;
        cout << std::chrono::duration_cast<std::chrono::seconds>(end_time2 - start_time2).count() << ":";
        cout << std::chrono::duration_cast<std::chrono::microseconds>(end_time2 - start_time2).count() << ":"<<endl;
    }

    BOOST_AUTO_TEST_CASE(test_updateWhere_ARTIndex_MVCC)
    {
        cout << "test_updateWhere_ARTIndex_MVCC" << endl;

        ///Run iterators simple on prefix with call back!!
        ///Get transaction readers
        using valueType = RecordType;
        using snapshot_type = mvcc11::snapshot<RecordType>;
        typedef smart_ptr::shared_ptr<snapshot_type const> const_snapshot_ptr;

       auto filter = [](const_snapshot_ptr recordtype)->bool
       {
           RecordType tuple = RecordType(recordtype->value.getAttribute<0>(),
                                         recordtype->value.getAttribute<1>(),
                                         recordtype->value.getAttribute<2>(),
                                         recordtype->value.getAttribute<3>(),
                                         recordtype->value.getAttribute<4>());

            if(((int)(tuple.getAttribute<1>()) < 5000))
            {
               return true;

            }
            return false;
       };


        auto UpdateWhere= [filter](ARTTupleContainer& ARTable,size_t id)
        {
            int index=0;
            uint64_t out[] = {0, 0};
            ARTable.updateAllByPredicate(updater,id,filter);
            //ARTable.iterate(cb,&out,id);

            index++;
        };

        auto start_time = std::chrono::high_resolution_clock::now();
        Transaction<TableOperationOnTupleFunc,ARTTupleContainer>* t1 = new
                Transaction<TableOperationOnTupleFunc,ARTTupleContainer>(UpdateWhere,*ARTableWithTuples);
        t1->CollectTransaction();
        auto end_time= std::chrono::high_resolution_clock::now();

        cout<<endl<<"Single Thread Writer Time->";
        cout << std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time).count() << ":";
        cout << std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count() << ":";
    }


BOOST_AUTO_TEST_SUITE_END()

