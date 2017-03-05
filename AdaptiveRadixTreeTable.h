//
// Created by Players Inc on 27/02/2017.
//

#ifndef MVCCART_ADAPTIVERADIXTREETABLE_H
#define MVCCART_ADAPTIVERADIXTREETABLE_H

#include <iostream>
#include <inttypes.h>
#include "art.h"

#include "table/TableInfo.hpp"
#include "table/TableException.hpp"
#include "table/BaseTable.hpp"
#include <boost/signals2.hpp>
#include "table/TableInfo.hpp"
#define MAX_VERSION_DEPTH 100
typedef unsigned char MVRecordType[MAX_VERSION_DEPTH][100];


template <typename Iter>
class ADTIterator {
public:
    typedef typename Iter::value_type::second_type RecordType;

    typedef std::function<bool(const RecordType&)> Predicate;

    explicit ADTIterator() {}
    explicit ADTIterator(Iter j, Iter e, Predicate p) : i(j), end(e), pred(p) {
        // make sure the initial iterator position refers to an entry satisfying the predicate

        //Set the starting position of the tuple found (i = initial position)

        while (i != end && ! pred(i->second)) ++i;
    }

    ADTIterator& operator++() {
        i++;
        while (i != end && ! pred(i->second)) ++i;
        return *this;
    }

    ADTIterator operator++(int) { auto tmp = *this; ++(*this); return tmp; }
    bool isValid() const { return i != end; }

    /*TuplePtr<RecordType> operator*() {
        return TuplePtr<RecordType> (new RecordType(i->second));
    }*/
     typename Iter::value_type::second_type* operator->() { return &i->second; }

protected:
    Iter i, end;
    Predicate pred;
};

template <typename Iter>
inline ADTIterator<Iter> makeADTIterator(Iter j, Iter e, typename ADTIterator<Iter>::Predicate p)
                                            { return ADTIterator<Iter>(j, e, p); }


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

 bool myfunction(RecordType R)
{
    if(R[0]=='b')
        return true;
    else
        false;
}

template <typename RecordType, typename KeyType = char[25]>
class AdaptiveRadixTreeTable : public pfabric::BaseTable
{
        private:   art_tree t;
        private: char buf[512];
        private: int res;
        public: uint64_t ARTSize;




        typedef std::function<bool(RecordType)> Predicate;


        ///< typedef for a updater function which returns a modification of the parameter tuple
        typedef std::function<void(RecordType&)> UpdaterFunc;

        ///< typedefs for a function performing updates + deletes. Similar to UpdaterFunc
        ///< it allows to update the tuple, but also to delete it (indictated by the
        ///< setting the bool component of @c UpdateResult to false)
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
                    if([](RecordType R){ return R[0]=='b';}(*ptr))
                        std::cout<<"found K/V ="<<key<<"/"<<*ptr<<"\n";
                }
                return 0;
            }


        /**
         * Constructor for creating an empty table with a given schema.
         */
        public:   AdaptiveRadixTreeTable(){
                      int res = art_tree_init(&t);
                      this->ARTSize = art_size(&t);
                  }

        /**
        * Destructor for table.
        */
        public:  void DestroyAdaptiveRadixTreeTable(){ art_tree_destroy(&t); }

        /**
         * insert into tree Specifying keys and RecordTypes;
         */
        public:void insertOrUpdateByKey(KeyType key, const RecordType& rec)
        {
            int len = strlen(key);
            key[len-1] = '\0';
            art_insert(&t, (unsigned char*)key, len, (void*)rec);
            this->ARTSize = art_size(&t);
            //std::cout<<"Size of ART: "<<art_size(&t)<<std::endl;
        }

        /**
         * Delete from tree by Keys
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
            void * val = art_delete(&t, (unsigned char*)key, len);
            RecordType *  val2 = (RecordType *)val;
            this->ARTSize = art_size(&t);
            return val2;
        }


        /**
         * Delete from Tree where clause
         */
        public: RecordType * deleteWhere()
        {
                uint64_t out[] = {0, 0};
                 art_deleteWhere(&t,
                                 [](RecordType R){ return R[0]=='b';},
                                 &out,iter_callbackByPredicate);
            return NULL;
        }


        /**
         * GetByKey
         */
        public: RecordType * findValueByKey(KeyType key)
        {
            int len;
            uintptr_t line = 1;
            len = strlen(key);
            key[len-1] = '\0';

            //Search first, ensure the entries still exit optional
            void*  val = art_search(&t, (unsigned char*)key, len);
            RecordType* val2= (RecordType *)val;
            this->ARTSize = art_size(&t);
            return val2;
        }




        /**
         * Iterate over tree
         */
        public:void iterate()
        {
            uint64_t out[] = {0, 0};
            art_iter(&t, iter_callbackByPredicate, &out);
        }



        /**
         * UpdateOrDeleteByKeywhere
         */
         //update or delete by keys where where predicate satisfies.

        /**
         * UpdateByKeyWhere
         */

        /**
         * UpdateWhere
         */



        AdaptiveRadixTreeTable(const pfabric::TableInfo& tInfo) : BaseTable(tInfo) {}



};
#endif //MVCCART_ADAPTIVERADIXTREETABLE_H
