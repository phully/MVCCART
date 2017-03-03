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

#define MAX_VERSION_DEPTH 100
typedef unsigned char RecordType[50];
typedef unsigned char MVRecordType[MAX_VERSION_DEPTH][100];


template <typename Iter>
class ADTIterator {
public:
    typedef typename Iter::value_type::second_type RecordType;

    typedef std::function<bool(const RecordType&)> Predicate;

    explicit ADTIterator() {}
    explicit ADTIterator(Iter j, Iter e, Predicate p) : i(j), end(e), pred(p) {
        // make sure the initial iterator position refers to an entry satisfying
        // the predicate
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
    // typename Iter::value_type::second_type* operator->() { return &i->second; }

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
int iter_cb(void *data, const unsigned char* key, uint32_t key_len, void *val);
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


int iter_cb(void *data, const unsigned char* key, uint32_t key_len, void *val)
{
    uint64_t *out = (uint64_t*)data;
    uintptr_t line = (uintptr_t)val;
    uint64_t mask = (line * (key[0] + key_len));
    out[0]++;
    out[1] ^= mask;
    return 0;
}



int iter_callback(void *data, const unsigned char* key, uint32_t key_len, void *val)
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
        std::cout<<"found K/V ="<<key<<"/"<<*ptr<<"\n";

    return 0;
}


template <typename RecordType, typename KeyType = char[25]>
    class AdaptiveRadixTreeTable
    {

        private: art_tree t;
        private: char buf[512];
        private: int res;
        public: uint64_t ARTSize;


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
        public:  void DestroyAdaptiveRadixTreeTable()   { art_tree_destroy(&t); }


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
         * Iterate over tree
         */
        public:void iterate()
        {
            uint64_t out[] = {0, 0};
            art_iter(&t, iter_callback, &out);
        }
    };

#endif //MVCCART_ADAPTIVERADIXTREETABLE_H
