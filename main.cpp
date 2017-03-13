#include <iostream>
#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <check.h>
#include "ARTFULCpp.h"
#include "table/Table.hpp"
#include "core/Tuple.hpp"
#include <boost/tuple/tuple.hpp>

using namespace std;
using namespace pfabric;

void InsertAndIterateyTuples();


///Testing ART , with MyTuple type as Tuple
typedef pfabric::Tuple<unsigned long, int, char *, double> MyTuple;
typedef TuplePtr<MyTuple> InTuplePointer;
typedef MyTuple  TuplesToStore[50];
char ValuesToStore[10][50];
std::vector<MyTuple *> Bucket;

static  int iter_callbackByPredicate(void *data, const unsigned char* key, uint32_t key_len, void *val)
{
    MyTuple * ptr = (MyTuple *)val;
    if(ptr != NULL)
    {
        auto tptr = ptr->getAttribute<2>();
        std::cout<<"###found K/V ="<<key<<"\n"<<tptr<<"\n";
        //std::cout<<"###found K/V ="<<key<<"\n"<<tptr<<"\n";
    }
    return 0;
}

static  int cb(void *data, const unsigned char* key, uint32_t key_len, void *val)
{
    //RecordType * ptr = (RecordType *)val;
    if(val != NULL)
    {
        std::cout<<"###found K/V ="<<key<<"/"<<(char *)val<<"\n";
    }
    return 0;
}


template <typename RecordType, typename KeyType = DefaultKeyType>
void InsertAndIterateyTuples(ARTFULCpp<RecordType, KeyType> testTable);

template <typename RecordType, typename KeyType>
void IterateByKeyValues(ARTFULCpp<RecordType, KeyType> myADTTable);

int main()
{
    std::cout << "Adaptive Radix Tree " << std::endl;


    ///1-  Create template for ARTTable. simple template Type of Index Page Char[50] RecordType Char[20] KeyType
    auto ARTable =  new ARTFULCpp<char[50], char[20]>();
    IterateByKeyValues<char [50] ,char[20]>(*ARTable);


    ///2-  Create template for ARTTable. complex template Type of
    /// Record asTuple: <unsigned long, int, char *, double> MyTuple;
    /// and char[20] as KeyType
    //auto ARTable =  new ARTFULCpp<MyTuple, char[20]>();
    //InsertAndIterateyTuples<MyTuple ,char[20]>(*ARTable);

    //InsertAndIterateyTuples();

    /// 1- Test Insert & Delete by key
    ///   -> Typedef RecordType leave: Char[50] , Key Char[20]
    //AdaptiveRadixTreeTable<char[50] ,char[20]> myADTTable2 = AdaptiveRadixTreeTable<char[50],char[20]>();
    //InsertDeleteByKeyValue<char[50] ,char[20]>(myADTTable2);


    /// 2- Test Iterate by key-values
    ///   -> Typedef RecordType leave: Char[50] , Key Char[20]
    // AdaptiveRadixTreeTable<char* ,char[20]> myADTTable2 = AdaptiveRadixTreeTable<char *,char[20]>();
    // IterateByKeyValues<char * ,char[20]>(myADTTable2);

    /// 3- Test Iterate by key-values
    ///   -> Typedef RecordType leave: Char[50] , Key Char[20]
    //AdaptiveRadixTreeTable<char[50] ,char[20]> myADTTable2 = AdaptiveRadixTreeTable<char[50],char[20]>();
    //IterateByKeyValuesWithPredicate<char[50] ,char[20]>(myADTTable2);


    /// 4- Test Insert & Delete by key
    ///   -> Typedef RecordType leave: Char[50] , Key Char[20]
    //AdaptiveRadixTreeTable<char[50] ,char[20]> myADTTable2 = AdaptiveRadixTreeTable<char[50],char[20]>();
    //InsertSearchByKeyValue<char[50] ,char[20]>(myADTTable2);


    std::cout<<"Completed Successfully!!";
    return 0;
}

template <typename RecordType, typename KeyType>
void InsertAndIterateyTuples(ARTFULCpp<RecordType, KeyType> testTable)
{

   // auto  testTable = std::make_shared<ARTable<InTuplePointer,DefaultKeyType>> ();
    int len, len2;
    char buf[20];
    char bufVal[50];
    FILE *fvals = fopen("/Users/fuadshah/Desktop/CODE9/MVCCART/test_data/words.txt", "r");
    FILE *uuid = fopen("/Users/fuadshah/Desktop/CODE9/MVCCART/test_data/uuid.txt", "r");
    int index = 0;
    int i=1;


    while (fgets(buf, sizeof buf, fvals))
    {
        fgets(bufVal, sizeof bufVal, uuid);

        len  = strlen(buf);
        len2 = strlen(bufVal);
        bufVal[len2]='\0';
        buf[len] = '\0';

        auto tp   =   new MyTuple((unsigned long) i, i + 100, bufVal, i / 100.0);
        Bucket.push_back(tp);
        InTuplePointer tptr (new MyTuple((unsigned long) i, i + 100, bufVal, i / 100.0));

        //cout<<" key / val = "<<buf<<"/"<<bufVal<<endl;
       // auto tptrr = tp.getAttribute<2>();
        cout<<" typed key / val = "<<buf<<"/"<<Bucket[index]->getAttribute<2>()<<endl;
       testTable.insertOrUpdateByKey(buf,Bucket[index]);
        //testTable2->insertOrUpdateByKey(buf,tp);
        i++;
        index++;
        if(i > 10)//235887)
            break;
    }

    testTable.iterate(iter_callbackByPredicate);

}


template <typename RecordType, typename KeyType>
void IterateByKeyValues(ARTFULCpp<RecordType, KeyType> myADTTable)
{
    int len, len2;
    char buf[20];
    char bufVal[50];


   /* using MyVector = std::vector<char *>;
    auto v1 = std::vector<std::shared_ptr<IntVector>> {
            std::make_shared<IntVector>(IntVector{1,2,3}),
            std::make_shared<IntVector>(IntVector{4,5,6}),
            std::make_shared<IntVector>(IntVector{7,8,9})
    };*/

    /// Reading all Values to store against keys
    FILE *fvals = fopen("/Users/fuadshah/Desktop/CODE9/MVCCART/test_data/uuid.txt", "r");
    FILE *f = fopen("/Users/fuadshah/Desktop/CODE9/MVCCART/test_data/words.txt", "r");
    int index = 0;
    int line = 0;
    while (fgets(bufVal, sizeof bufVal, fvals))
    {
        fgets(buf, sizeof buf, f);
        len = strlen(bufVal);
        len2 = strlen(buf);

        buf[len2]='\0';
        bufVal[len] = '\0';
        char * temp = bufVal;
        strcpy(ValuesToStore[index],bufVal);
        cout << "inserting key= " << buf << "  - value = " << ValuesToStore[index]<< endl;
        myADTTable.insertOrUpdateByKey(buf, ValuesToStore[index]);
        //cout<<index<<" tuple added::::::"<<ValuesToStore[index]<<"\n";
        //strcpy(ValuesToStore[index], bufVal);
        index++;
        if (index == 10)
            break;
    }

    /// Reading all Keys to store against values
    /*
    while (fgets(buf, sizeof buf, f))
    {
        len = strlen(buf);
        buf[len] = '\0';
        cout << "inserting key= " << buf << "  - value = " << ValuesToStore[line] << endl;

        myADTTable.insertOrUpdateByKey(buf, ValuesToStore[line]);
        //myADTTable.insertOrUpdateByKey(buf, line);
        cout << buf << endl;
        cout << "Size of ART:- " << myADTTable.ARTSize << endl;
        line++;

        if (line == 10)
            break;
    }
    */
    /// Get iterator for that the tree we put data into
    //  nlines = line - 1;
    //uint64_t out[] = {0, 0};
    //art_iter(&t, iter_cb, &out);

    myADTTable.iterate(cb);
    myADTTable.DestroyAdaptiveRadixTreeTable();
    cout<<"Exited normaly";
}
