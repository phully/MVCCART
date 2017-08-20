//
// Created by Players Inc on 18/08/2017.
//

#ifndef MVCCART_TRANSACTIONTEMPLATES_H
#define MVCCART_TRANSACTIONTEMPLATES_H


#include "transactionManager.h"
#include <iostream>
#include <boost/test/unit_test.hpp>
#include <boost/lexical_cast.hpp>
#include "mvcc/mvcc.hpp"
#include "ART/ARTFULCpp.h"

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
typedef pfabric::Tuple<string,unsigned long, int,string, double> RecordType;
typedef char KeyType[20];
typedef ARTFULCpp<RecordType,KeyType> ARTTupleContainer;
char KeysToStore[235890][20];
std::vector<RecordType> vectorValues;
using snapshot_type = mvcc11::snapshot<RecordType>;
typedef smart_ptr::shared_ptr<snapshot_type const> const_snapshot_ptr;

typedef std::function <void(ARTTupleContainer&,size_t id,std::string& status)> TableOperationOnTupleFunc;
typedef std::function <void(ARTTupleContainer&,size_t id,std::string& status,std::pair<int,int>)> TransactionLambda;

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

int current_time_nanoseconds()
{
    struct timespec tm;
    clock_gettime(CLOCK_REALTIME, &tm);
    return tm.tv_nsec;
}

std::function<RecordType(RecordType&)> updater = [](RecordType& record)
{

    string attr0 = record.getAttribute<0>();
    unsigned long attr1 = record.getAttribute<1>();
    int attr2 = record.getAttribute<2>()+100;
    string attr3 =  fmt::format("String/{}",record.getAttribute<0>());
    double attr4 =  attr1/200.0;

    RecordType tuple = RecordType(attr0,
                                  attr1,
                                  attr2,
                                  attr3,
                                  attr4);
    return tuple;
};


std::function<void(RecordType&)> Evaluater = [](RecordType& tp)
{

    unsigned long index = tp.getAttribute<1>();
    /*BOOST_TEST(tp.getAttribute<0>() == vectorValues[index].getAttribute<0>());
    BOOST_TEST(tp.getAttribute<1>() == vectorValues[index].getAttribute<1>());
    BOOST_TEST(tp.getAttribute<2>() == (int)(vectorValues[index].getAttribute<2>())+100);
    //BOOST_TEST(tp.getAttribute<3>() == fmt::format("String/{}", keysToFind));
    BOOST_TEST(tp.getAttribute<4>() == (vectorValues[index].getAttribute<1>()) / 200.0);*/
};



auto UpdateSmall = [](ARTTupleContainer &ARTWithTuples, size_t id, std::string &status,std::pair<int,int> range)
{
    std::vector<RecordType> WriteSet;
    std::vector<RecordType> ReadSet;

    random::mt19937 rng(current_time_nanoseconds());
    random::uniform_int_distribution<> randomKeys1(range.first,range.second);
    int totalCachedMissed=0;
    for (int i = 0; i < 80; i++)
    {
        char *keysToFind = KeysToStore[randomKeys1(rng)];
        auto result = ARTWithTuples.insertOrUpdateByKey(keysToFind, updater, id, status);

        if (result == nullptr)
        {
            totalCachedMissed++;
        }
        else
        {
            auto tp = result->value;
            if(tp.getAttribute<0>() != "")
                WriteSet.push_back(tp);
        }
    }


    ///Evaluating randomly 20 keys from WriteSet & store in -> ReadSet:

    random::uniform_int_distribution<> randomKeys2(0,80);
    int totalCachedMissed2=0;

    for(int i=0; i < 20; i++)
    {
        //int index = randomKeys2(rng);
        auto tuple = WriteSet[i];
        string str =  tuple.getAttribute<0>();
        char *cstr = new char[str.length() + 1];
        strcpy(cstr, str.c_str());
        auto result= ARTWithTuples.findValueByKey(cstr);
        if(result != NULL )
        {
            auto tp = result->value;
            Evaluater(tp);
            ReadSet.push_back(tp);
        }
        else
        {
            totalCachedMissed2++;
        }
    }
    cout<<"Total updates cached missed out of 80 Updates::"<<totalCachedMissed<<" by transaction#"<<id<<endl;
    cout<<"Total Reads cached missed out of 20  Reads::"<<totalCachedMissed2<<" by transaction#"<<id<<endl;

};

auto UpdateMedium = [](ARTTupleContainer &ARTWithTuples, size_t id, std::string &status,std::pair<int,int> range)
{
    std::vector<RecordType> WriteSet;
    std::vector<RecordType> ReadSet;

    random::mt19937 rng(current_time_nanoseconds());
    random::uniform_int_distribution<> randomKeys1(range.first,range.second);
    int totalCachedMissed=0;
    for (int i = 0; i < 800; i++)
    {
        char *keysToFind = KeysToStore[randomKeys1(rng)];
        auto result = ARTWithTuples.insertOrUpdateByKey(keysToFind, updater, id, status);

        if (result == nullptr)
        {
            totalCachedMissed++;
        }
        else
        {
            auto tp = result->value;
            if(tp.getAttribute<0>() != "")
                WriteSet.push_back(tp);
        }
    }

    ///Evaluating randomly 20 keys from WriteSet & store in -> ReadSet:
    random::uniform_int_distribution<> randomKeys2(0,WriteSet.size());
    int totalCachedMissed2=0;

    for(int i=0; i < 200; i++)
    {
        //int index = randomKeys2(rng);
        auto tuple = WriteSet[i];
        string str =  tuple.getAttribute<0>();
        char *cstr = new char[str.length() + 1];
        strcpy(cstr, str.c_str());
        auto result= ARTWithTuples.findValueByKey(cstr);
        if(result != NULL)
        {

            auto tp = result->value;
            Evaluater(tp);
            ReadSet.push_back(tp);
        }
        else
        {
            totalCachedMissed2++;
        }
    }
    cout<<"Total updates cached missed out of 800 Updates::"<<totalCachedMissed<<" by transaction#"<<id<<endl;
    cout<<"Total updates cached missed out of 200 Reads  ::"<<totalCachedMissed2<<" by transaction#"<<id<<endl;

};


auto UpdateLong = [](ARTTupleContainer &ARTWithTuples, size_t id, std::string &status,std::pair<int,int> range)
{
    std::vector<RecordType> WriteSet;
    std::vector<RecordType> ReadSet;

    random::mt19937 rng(current_time_nanoseconds());
    random::uniform_int_distribution<> randomKeys1(range.first,range.second);
    int totalCachedMissed=0;
    for (int i = 0; i < 80000; i++)
    {
        char *keysToFind = KeysToStore[randomKeys1(rng)];
        auto result = ARTWithTuples.insertOrUpdateByKey(keysToFind, updater, id, status);

        if (result == nullptr || result == NULL)
        {
            totalCachedMissed++;
        }
        else
        {
            auto tp = result->value;
            if(tp.getAttribute<0>() != "")
                WriteSet.push_back(tp);
        }
    }
    cout<<"Total updates cached missed out of 80000::"<<totalCachedMissed<<" by transaction#"<<id<<endl;


    ///Evaluating randomly 20000 keys from WriteSet & store in -> ReadSet:
    random::uniform_int_distribution<> randomKeys2(0,8000);
    int totalCachedMissed2=0;

    for(int i=0; i < 20000; i++)
    {
        //int index = randomKeys2(rng);
        auto tuple = WriteSet[i];
        string str =  tuple.getAttribute<0>();
        char *cstr = new char[str.length() + 1];
        strcpy(cstr, str.c_str());
        auto result= ARTWithTuples.findValueByKey(cstr);
        if(result != NULL)
        {
            auto tp = result->value;
            Evaluater(tp);
            ReadSet.push_back(tp);
        }
        else
        {
            totalCachedMissed2++;
        }
    }
    cout<<"Total updates cached missed out of 20000::"<<totalCachedMissed2<<" by transaction#"<<id<<endl;

};

#endif //MVCCART_TRANSACTIONTEMPLATES_H
