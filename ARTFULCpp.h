/*
 + `insert(KeyType k, const RecordType& rec)` inserts a new record with the given key into the table.(done)
 + `deleteByKey(KeyType k)` deletes the record with the given key.(done)
 + `deleteWhere(std::function<bool(const RecordType&)> predicate)` deletes all records satisfying the given predicate.(not needed)
-> `updateByKey(KeyType k,  std::function<void(RecordType&)> updater)` updates the tuple with the given key by applying
   the function `updater` which accepts the tuple as parameter and modified it in a certain way.
-> `updateWhere(std::function<bool(const RecordType&)> predicate, std::function<void(RecordType&)> updater)` updates all tuples from
   the table satisfying the given predicate by applying the function `updater` which modifies the tuple.
-> `getByKey(KeyType k)` returns a pointer to the tuple with the given key in the form of `TuplePtr<RecordType>`. If no tuple
    exists for this key, then an exception is raised.
 + `select(std::function<bool(const RecordType&)> predicate)`
 */

#ifndef MVCCART_ARTFULCPP_H
#define MVCCART_ARTFULCPP_H

#include <iostream>
#include <inttypes.h>

//#include "table/TableInfo.hpp"
//#include "table/TableException.hpp"
//#include "table/BaseTable.hpp"
//#include <boost/signals2.hpp>
//#include "table/TableInfo.hpp"
//#include "fmt/format.h"
#include "core/Tuple.hpp"
//#include <boost/tuple/tuple.hpp>
#include "ArtCPP.hpp"

using namespace pfabric;

#define MAX_VERSION_DEPTH 100
typedef char DefaultKeyType[20];
typedef unsigned char MVRecordType[MAX_VERSION_DEPTH][100];



typedef struct {
    int count;
    int max_count;
    const char **expected;
} prefix_data;

template <typename RecordType, typename KeyType = char[25]>
static int test_prefix_cb(void *data, const unsigned char *k, uint32_t k_len, void *val);
static int test_prefix_cb(void *data, const unsigned char *k, uint32_t k_len, void *val)
{
    prefix_data *p = (prefix_data*)data;
    if(p->count < p->max_count)
    {
        std::cout << "p->count < p->max_count " << std::endl;
    }
    if(memcmp(k, p->expected[p->count], k_len) == 0)
    {
        std::cout <<"Key: %s Expect: %s", k, p->expected[p->count];
    }

    p->count++;
    return 0;
}

int iter_cb(void *data, const unsigned char* key, uint32_t key_len, void *val);
int iter_cb(void *data, const unsigned char* key, uint32_t key_len, void *val)
{
    uint64_t *out = (uint64_t*)data;
    uintptr_t line = (uintptr_t)val;
    uint64_t mask = (line * (key[0] + key_len));
    out[0]++;
    out[1] ^= mask;
    return 0;
}



template <typename RecordType, typename KeyType = char[25]>
class ARTFULCpp
{
    public: ArtCPP<RecordType, KeyType>* ARTIndexTable;
    //auto testTable = std::make_shared<ARTable<MyTuple ,DefaultKeyType>> ();

    typedef std::function<bool( void*)> Pred;
    typedef std::function<bool(RecordType&)> UpdelFunc;
    typedef std::function<bool(RecordType&)> UpPred;



        public: uint64_t getARTSize()
        {
            return ARTIndexTable->art_size();
        }

        /**
         * Constructor for creating an empty table with a given schema.
         */
        public:ARTFULCpp()
        {
            ARTIndexTable = new  ArtCPP<RecordType, KeyType>();
        }

        /**
        * Destructor for table.
        */
        public:void DestroyAdaptiveRadixTreeTable()
        {
            delete ARTIndexTable;
        }

        /**
        * @brief Insert/Update a tuple.
        *
        * Insert the given tuple @rec with the given key into the table, if key already exists it updates it. TODO:: After the insert
        * all observers are notified.
        *
        * @param key the key value of the tuple
        * @param rec the actual tuple
        */
        public:void insertOrUpdateByKey(KeyType key, const RecordType& rec)
        {
            int len = strlen(key);
            //key[len] = '\0';
            //std::cout<<"addres inserted::"<<rec<<"\n";
            //converting void pointer to the Type of TuplePointer
            //boost::intrusive_ptr<InTuplePointer> b = *(boost::intrusive_ptr<InTuplePointer>*)(rec);
            //auto tptrr = *rec->getAttribute<1>();
            //std::cout<<tptrr;
            //void * data = (void*)&rec;
            //void* dt = static_cast<void*>(rec);
            //void* b = reinterpret_cast<void*>(rec);

            //this->NumberOfActiveWriteModifiers++;
            //boost::thread* mythread = new boost::thread((ARTIndexTable->art_insert),(unsigned char*)key, len, (void*)rec);
            //Writerthreads.push_back(mythread);
            //ARTIndexTable->art_insert((unsigned char*)key, len, (void*)rec);
            ARTIndexTable->startInsertModifyThread((unsigned char*)key, len, (void*)rec);
            std::cout<<"Size of ART: "<<ARTIndexTable->art_size()<<std::endl;
        }

        /**
        * @brief Delete a tuple.
        *
        * Delete the tuples associated with the given key from the table
        * and TODO:: inform the observers.
        *
        * @param key the key for which the tuples are deleted from the table
        * @return the number of deleted tuples
        */
        public: RecordType * deleteByKey(KeyType key)
        {
            int len;
            uintptr_t line = 1;
            len = strlen(key);
            key[len] = '\0';

            //Search first, ensure the entries still exit optional
            //RecordType  val = (RecordType)art_search(&t, (unsigned char*)key, len);
            //val = (RecordType *)art_search(&t, (unsigned char*)key, len);

            // Delete, should get line-no back
            ///void * val = ARTIndexTable->art_delete((unsigned char*)key, len);
            ///RecordType *  val2 = (RecordType *)val;
            //return val2;
            return NULL;
        }


        /**
         * TODO::: Lambdas std::function Delete from Tree where clause
         */
        public: RecordType * deleteWhere()
        {
                uint64_t out[] = {0, 0};
            /*void* deletedVal= art_deleteWhere(&t,
                                 [](RecordType R){ return R[0]=='8';},
                                 &out,iter_callbackByPredicate);

            if(deletedVal != NULL)
                std::cout<<"Value deleted "<<(RecordType*)deletedVal;*/
            return NULL;
        }


        /**
        * @brief Return the tuple associated with the given key. TODO:: change it to TuplePtr
        *
        * Return the tuple from the table that is associated with the given
        * key. If the key doesn't exist, an exception is thrown.
        *
        * @param key the key value
        * @return the tuple associated with the given key
        */
        public: RecordType * findValueByKey(KeyType key)
        {
            int len;
            uintptr_t line = 1;
            len = strlen(key);
            key[len] = '\0';

            //Search first, ensure the entries still exit optional
            void*  val = ARTIndexTable->startSearchThread((unsigned char*)key, len);
            ///RecordType* val2= (RecordType *)val;
            //std::cout<<"Size of ART: "<<ARTIndexTable->art_size()<<std::endl;
            ///return val2;
            return NULL;
        }


        /**
         * Iterate over tree
         */
        public:void iterate(art_callback cb)
        {
            uint64_t out[] = {0, 0};
            //this->NumberOfActiveReaders++;
            //boost::thread* mythread = new boost::thread((ARTIndexTable->art_iter),cb,&out);
            //Readerthreads.push_back(mythread);
            ARTIndexTable->startIterating(cb, &out);
        }


        /**
         * Iterate over tree by Predicate
         */
        public:void iterateByPredicate(art_callback iter_callbackByPredicate, Pred predicate)
        {
            uint64_t out[] = {0, 0};
            ///ARTIndexTable->art_iterByPredicate(iter_callbackByPredicate, &out,predicate);
        }



         /**
        * @brief Update all tuples satisfying the given predicate.
        *
        * Update all tuples in the table which satisfy the given predicate.
        * The actual modification is done by the updater function specified as parameter.
        *
        * @param pfunc a predicate func returning true for a tuple to be modified
        * @param ufunc a function performing the modification by returning a modified
        *        tuple
        * @return the number of modified tuples
        */
        unsigned long UpdateKeyWhere(UpPred pfunc, UpdelFunc ufunc) {
            return 0;
        }

        /**
         * @brief Update the tuple specified by the given key.
         *
         * Update the tuple in the table associated with the given key.
         * The actual modification is done by the updater function specified as parameter.
         *
         * @param key the key of the tuple to be modified
         * @param func a function performing the modification by returning a modified
         *        tuple
         * @return the number of modified tuples
         */
        unsigned long DeleteKeyWhere(KeyType key, UpdelFunc ufunc) {

            return 0;
        }
        //ARTFULCpp(const pfabric::TableInfo& tInfo) : BaseTable(tInfo) {}
};
#endif //MVCCART_ADAPTIVERADIXTREETABLE_H