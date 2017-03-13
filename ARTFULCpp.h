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

#include "table/TableInfo.hpp"
#include "table/TableException.hpp"
#include "table/BaseTable.hpp"
#include <boost/signals2.hpp>
#include "table/TableInfo.hpp"
//#include "fmt/format.h"
#include "core/Tuple.hpp"
#include <boost/tuple/tuple.hpp>
#include "ArtCPP.h"

using namespace pfabric;

#define MAX_VERSION_DEPTH 100
typedef char DefaultKeyType[20];
typedef unsigned char MVRecordType[MAX_VERSION_DEPTH][100];

typedef bool(*PredicatePtr)(const pfabric::TuplePtr<void> );
typedef bool(*UpdateFuncPtr)(const pfabric::TuplePtr<void>  );
typedef bool(*DeleteFuncPtr)(const pfabric::TuplePtr<void> );

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
class ARTFULCpp : public pfabric::BaseTable
{

        public: ArtCPP<RecordType, KeyType>* ARTIndexTable;
        //auto testTable = std::make_shared<ARTable<MyTuple ,DefaultKeyType>> ();


        typedef std::function<bool(const RecordType*)> Predicate;
        typedef std::function<bool(RecordType&)> UpdelFunc;

        /**
        * iter_callback
        */
        public: static int iter_callback(void *data, const unsigned char* key, uint32_t key_len, void *val)
        {

                RecordType* ptr = (RecordType *)val;
                /*
                  uint64_t *out = (uint64_t*)data;
                  uintptr_t line = (uintptr_t)val;
                  uint64_t mask = (line * (key[0] + key_len));
                  out[0]++;
                  out[1] ^= mask;
                */

                if(ptr != NULL)
                {
                    std::cout<<"found K/V ="<<key<<"/"<<*ptr<<"\n";
                }
                return 0;
            }

        public: static  int iter_callbackByPredicate(void *data, const unsigned char* key, uint32_t key_len, void *val)
        {
                RecordType* ptr = (RecordType *)val;
                if(ptr != NULL)
                {

                   std::cout<<"found K/V ="<<key<<val<<"\n";
                }
                return 0;
        }


        /**
         * Constructor for creating an empty table with a given schema.
         */
        public:ARTFULCpp()
        {
            ARTIndexTable = new ArtCPP<RecordType, KeyType>();
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
        public:void insertOrUpdateByKey(KeyType key, const RecordType* rec)
        {
            int len = strlen(key);
            key[len] = '\0';
            std::cout<<"addres inserted::"<<rec<<"\n";
            //converting void pointer to the Type of TuplePointer
            //boost::intrusive_ptr<InTuplePointer> b = *(boost::intrusive_ptr<InTuplePointer>*)(rec);
            //auto tptrr = *rec->getAttribute<1>();
            //std::cout<<tptrr;
            //void * data = (void*)&rec;
            ARTIndexTable->art_insert((unsigned char*)key, len, (void*)rec);
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
            key[len-1] = '\0';

            // Search first, ensure the entries still exit optional
            //RecordType  val = (RecordType)art_search(&t, (unsigned char*)key, len);
            //val = (RecordType *)art_search(&t, (unsigned char*)key, len);

            // Delete, should get line-no back
            void * val = ARTIndexTable->art_delete((unsigned char*)key, len);
            RecordType *  val2 = (RecordType *)val;
            return val2;
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
            key[len-1] = '\0';

            //Search first, ensure the entries still exit optional
            void*  val = ARTIndexTable->art_search((unsigned char*)key, len);
            RecordType* val2= (RecordType *)val;
            std::cout<<"Size of ART: "<<ARTIndexTable->art_size()<<std::endl;
            return val2;
        }


        /**
         * Iterate over tree
         */
        public:void iterate(art_callback cb)
        {
            uint64_t out[] = {0, 0};
            ARTIndexTable->art_iter(cb, &out);
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
        unsigned long UpdateKeyWhere(PredicatePtr pfunc, UpdateFuncPtr ufunc) {



            // make sure we have exclusive access
            // note that we don't use a guard here!
            /*std::unique_lock<std::mutex> lock(mMtx);

            auto res = mDataTable.find(key);
            if (res != mDataTable.end()) {
                TableParams::ModificationMode mode = TableParams::Update;
                unsigned long num = 1;

                // perform the update
                auto upd = ufunc(res->second);

                // check whether we have to perform an update ...
                if (!upd) {
                    // or a delete
                    num = mDataTable.erase(key);
                    mode = TableParams::Delete;
                }
                lock.unlock();
                // notify the observers
                notifyObservers(res->second, mode, TableParams::Immediate);
                return num;
            }
            else {
                // don't forget to release the lock
                lock.unlock();
            }*/
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
        unsigned long DeleteKeyWhere(KeyType key, DeleteFuncPtr ufunc) {
            // make sure we have exclusive access
           /* std::unique_lock<std::mutex> lock(mMtx);

            auto res = mDataTable.find(key);
            if (res != mDataTable.end()) {
                ufunc(res->second);

                lock.unlock();
                notifyObservers(res->second, TableParams::Update, TableParams::Immediate);
                return 1;
            }
            else {
                lock.unlock();
            }*/
            return 0;
        }
        ARTFULCpp(const pfabric::TableInfo& tInfo) : BaseTable(tInfo) {}
};
#endif //MVCCART_ADAPTIVERADIXTREETABLE_H
