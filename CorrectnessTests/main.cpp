#include "Transactions/transactionManager.h"
#include <iostream>
#include <boost/test/unit_test.hpp>
#include <boost/lexical_cast.hpp>
#include "mvcc/mvcc.hpp"
#include "ART/ArtCPP.hpp"

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
typedef ArtCPP<RecordType,KeyType> ARTTupleContainer;
char KeysToStore[235890][20];
std::vector<RecordType> vectorValues;
using snapshot_type = mvcc11::snapshot<RecordType>;
typedef smart_ptr::shared_ptr<snapshot_type const> const_snapshot_ptr;

typedef std::function <void(ARTTupleContainer&,size_t id)> TableOperationOnTupleFunc;
typedef std::function <void(ARTTupleContainer&,size_t id,std::pair<int,int>)> TransactionLambda;



int main()
{
    std::cout << "Adaptive Radix Tree- " << std::endl;

    auto dummy = [](ARTTupleContainer &ARTable, size_t id)
    {
        cout<<"dummy called"<<endl;
    };

    auto ARTableWithTuples =  new ARTTupleContainer();


    Transaction<TableOperationOnTupleFunc,ARTTupleContainer>* t1 = new
            Transaction<TableOperationOnTupleFunc,ARTTupleContainer>(dummy,*ARTableWithTuples);
    t1->CollectTransaction();

    std::cout<<"Completed Successfully!!";
    return 0;
}
