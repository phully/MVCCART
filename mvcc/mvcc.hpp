#ifndef MVCC11_MVCC_HPP
#define MVCC11_MVCC_HPP

#ifndef MVCC11_CONTENSION_BACKOFF_SLEEP_MS
#define MVCC11_CONTENSION_BACKOFF_SLEEP_MS 50
#endif // MVCC11_CONTENSION_BACKOFF_SLEEP_MS

// Optionally uses std::shared_ptr instead of boost::shared_ptr
#ifdef MVCC11_USES_STD_SHARED_PTR

#include <memory>

namespace mvcc11 {
namespace smart_ptr {

using std::shared_ptr;
using std::make_shared;
using std::atomic_load;
using std::atomic_store;
using std::atomic_compare_exchange_strong

} // namespace smart_ptr
} // namespace mvcc11

#else // MVCC11_USES_STD_SHARED_PTR

#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/smart_ptr/make_shared.hpp>
#include "Transactions/transactionManager.h"

namespace mvcc11 {
    namespace smart_ptr {

        using boost::shared_ptr;
        using boost::make_shared;
        using boost::atomic_load;
        using boost::atomic_store;

        template <class T>
        bool atomic_compare_exchange_strong(shared_ptr<T> * p, shared_ptr<T> * v, shared_ptr<T> w)
        {
            return boost::atomic_compare_exchange(p, v, w);
        }

    } // namespace smart_ptr
} // namespace mvcc11

#endif // MVCC11_USES_STD_SHARED_PTR

#include <utility>
#include <chrono>
#include <thread>

#ifdef MVCC11_DISABLE_NOEXCEPT
#define MVCC11_NOEXCEPT(COND)
#else
#define MVCC11_NOEXCEPT(COND) noexcept(COND)
#endif

namespace mvcc11 {

    template <class ValueType>
    struct snapshot
    {
        using value_type = ValueType;

        snapshot(size_t ver) MVCC11_NOEXCEPT(true);

        template <class U>
        snapshot(size_t ver, U&& arg)
        MVCC11_NOEXCEPT( MVCC11_NOEXCEPT(value_type{std::forward<U>(arg)}) );

        template <class U>
        snapshot(size_t ver, size_t end_ver, U&& arg)
        MVCC11_NOEXCEPT( MVCC11_NOEXCEPT(value_type{std::forward<U>(arg)}) );

        smart_ptr::shared_ptr<snapshot> _older_snapshot;
        size_t version;
        size_t end_version;
        value_type value;
    };


    template <class ValueType>
    snapshot<ValueType>::snapshot(size_t ver) MVCC11_NOEXCEPT(true)
            : version{ver}
            , end_version{INF}
            , value{}
    {}

    template <class ValueType>
    template <class U>
    snapshot<ValueType>::snapshot(size_t ver, U&& arg)
    MVCC11_NOEXCEPT( MVCC11_NOEXCEPT(value_type{std::forward<U>(arg)}) )
            : version{ver}
            , end_version{INF}
            , value{std::forward<U>(arg)}
    {}

    template <class ValueType>
    template <class U>
    snapshot<ValueType>::snapshot(size_t ver,size_t end_ver, U&& arg)
    MVCC11_NOEXCEPT( MVCC11_NOEXCEPT(value_type{std::forward<U>(arg)}) )
            : version{ver}
            , end_version{INF}
            , value{std::forward<U>(arg)}
    {}




    template <class ValueType>
    class mvcc
    {

        public:
        using value_type = ValueType;
        using snapshot_type = snapshot<value_type>;
        using mutable_snapshot_ptr = smart_ptr::shared_ptr<snapshot_type>;
        using const_snapshot_ptr = smart_ptr::shared_ptr<snapshot_type const>;

        mvcc() MVCC11_NOEXCEPT(true);
        mvcc(size_t txn_id, value_type const &value);
        mvcc(size_t txn_id,value_type &&value);
        mvcc(size_t txn_id,mvcc const &other) MVCC11_NOEXCEPT(true);
        mvcc(size_t txn_id,mvcc &&other) MVCC11_NOEXCEPT(true);

        ~mvcc() = default;

        mvcc& operator=(mvcc const &other) MVCC11_NOEXCEPT(true);
        mvcc& operator=(mvcc &&other) MVCC11_NOEXCEPT(true);

        const_snapshot_ptr current() MVCC11_NOEXCEPT(true);
        const_snapshot_ptr operator*() MVCC11_NOEXCEPT(true);
        const_snapshot_ptr operator->() MVCC11_NOEXCEPT(true);

        const_snapshot_ptr overwrite(value_type const &value);
        const_snapshot_ptr overwrite(value_type &&value);


        const_snapshot_ptr overwriteMV(size_t txn_id,value_type const &value);
        const_snapshot_ptr overwriteMV(size_t txn_id,value_type &&value);

        const_snapshot_ptr deleteMV(size_t txn_id);

        template <class Updater>
        const_snapshot_ptr update(size_t txn_id,Updater updater);

        template <class Updater>
        const_snapshot_ptr try_update(size_t txn_id,Updater updater);

        template <class Updater, class Clock, class Duration>
        const_snapshot_ptr try_update_until(size_t txn_id,Updater updater,std::chrono::time_point<Clock, Duration> const &timeout_time);

        template <class Updater, class Rep, class Period>
        const_snapshot_ptr try_update_for(size_t txn_id,Updater updater, std::chrono::duration<Rep, Period> const &timeout_duration);

    private:
        template <class U>
        const_snapshot_ptr overwrite_impl(U &&value);

        template <class U>
        const_snapshot_ptr overwrite_implMV(size_t txn_id,U &&value);

        template <class U>
        const_snapshot_ptr delete_implMV(size_t txn_id);

        template <class Updater>
        const_snapshot_ptr try_update_impl(size_t txn_id,Updater &updater);

        template <class Updater, class Clock, class Duration>
        const_snapshot_ptr try_update_until_impl(
                size_t txn_id,
                Updater &updater,
                std::chrono::time_point<Clock, Duration> const &timeout_time);


        mutable_snapshot_ptr mutable_current_;
    };


    template <class ValueType>
    mvcc<ValueType>::mvcc() MVCC11_NOEXCEPT(true)
            : mutable_current_{smart_ptr::make_shared<snapshot_type>(0)}
    {mutable_current_->_older_snapshot=nullptr;}
    template <class ValueType>
    mvcc<ValueType>::mvcc(size_t txn_id,value_type const &value)
            : mutable_current_{smart_ptr::make_shared<snapshot_type>(txn_id,INF, value)}
    {mutable_current_->_older_snapshot=nullptr;}
    template <class ValueType>
    mvcc<ValueType>::mvcc(size_t txn_id,value_type &&value)
            : mutable_current_{smart_ptr::make_shared<snapshot_type>(txn_id,INF, std::move(value))}
    {mutable_current_->_older_snapshot=nullptr;}
    template <class ValueType>
    mvcc<ValueType>::mvcc(size_t txn_id,mvcc const &other) MVCC11_NOEXCEPT(true)
            : mutable_current_{smart_ptr::atomic_load(other)}
    {}
    template <class ValueType>
    mvcc<ValueType>::mvcc(size_t txn_id,mvcc &&other) MVCC11_NOEXCEPT(true)
            : mutable_current_{smart_ptr::atomic_load(other)}
    {}

    template <class ValueType>
    auto mvcc<ValueType>::operator=(mvcc const &other) MVCC11_NOEXCEPT(true) -> mvcc &
    {
        smart_ptr::atomic_store(&this->mutable_current_,
                                smart_ptr::atomic_load(&other.mutable_current_));

        return *this;
    }
    template <class ValueType>
    auto mvcc<ValueType>::operator=(mvcc &&other) MVCC11_NOEXCEPT(true) -> mvcc &
    {
        smart_ptr::atomic_store(&this->mutable_current_,
                                smart_ptr::atomic_load(&other.mutable_current_));
        return *this;
    }

    template <class ValueType>
    auto mvcc<ValueType>::current() MVCC11_NOEXCEPT(true) -> const_snapshot_ptr
    {
        return smart_ptr::atomic_load(&mutable_current_);
    }
    template <class ValueType>
    auto mvcc<ValueType>::operator*() MVCC11_NOEXCEPT(true) -> const_snapshot_ptr
    {
        return this->current();
    }
    template <class ValueType>
    auto mvcc<ValueType>::operator->() MVCC11_NOEXCEPT(true) -> const_snapshot_ptr
    {
        return this->current();
    }

    template <class ValueType>
    auto mvcc<ValueType>::overwrite(value_type const &value) -> const_snapshot_ptr
    {
        return this->overwrite_impl(value);
    }

    template <class ValueType>
    auto mvcc<ValueType>::overwrite(value_type &&value) -> const_snapshot_ptr
    {
        return this->overwrite_impl(std::move(value));
    }

    template <class ValueType>
    template <class U>
    auto mvcc<ValueType>::overwrite_impl(U &&value) -> const_snapshot_ptr
    {
        auto desired =
                smart_ptr::make_shared<snapshot_type>(
                        0,
                        std::forward<U>(value));

        while(true)
        {
            auto expected = smart_ptr::atomic_load(&mutable_current_);
            desired->version = expected->version + 1;
            //desired->_older_snapshot = expected;

            auto const overwritten =
                    smart_ptr::atomic_compare_exchange_strong(
                            &mutable_current_,
                            &expected,
                            desired);

            if(overwritten)
            {
                //std::cout<<"Overwrite"<<std::endl;
                return desired;
            }
            std::cout<<"missed";
        }
    }

    template <class ValueType>
    auto mvcc<ValueType>::overwriteMV(size_t txn_id,value_type const &value) -> const_snapshot_ptr
    {
        return this->overwrite_implMV(txn_id,value);
    }

    template <class ValueType>
    auto mvcc<ValueType>::overwriteMV(size_t txn_id,value_type &&value) -> const_snapshot_ptr
    {
        return this->overwrite_implMV(txn_id,std::move(value));
    }

    template <class ValueType>
    template <class U>
    auto mvcc<ValueType>::overwrite_implMV(size_t txn_id,U &&value) -> const_snapshot_ptr
    {

        ///1- Create New-Snapshot initially
        auto desired = smart_ptr::make_shared<snapshot_type>(txn_id, std::forward<U>(value));

        //while (true)
        {

            ///2- Fetch/Read  expected Version speculatively
            auto expected = smart_ptr::atomic_load(&mutable_current_);
            auto const const_expected_version = expected->version;
            auto const const_expected_end_version = expected->end_version;
            auto const &const_expected_value = expected->value;
            //desired->end_version = INF;

            ///3- if record was created and its active currently and not by the current transaction
            if ( const_expected_end_version != INF && const_expected_version != txn_id)
            {

                if (std::find(active_transactionIds.begin(), active_transactionIds.end(), const_expected_version) != active_transactionIds.end() )
                {
                    //std::cout << "Aborted on value " << expected->value<< std::endl;
                    //continue;
                    return nullptr;

                }
            }


            expected->end_version = txn_id;
            auto const overwritten = smart_ptr::atomic_compare_exchange_strong(&mutable_current_, &expected, desired);
            if (overwritten)
            {
                // std::cout << "overwritten =" << txn_id << desired->value<<std::endl;
                ///set status commited
                smart_ptr::atomic_store(&desired->_older_snapshot,expected);
                return desired;
            }
        }
        return nullptr;
    }

    template <class ValueType>
    auto mvcc<ValueType>::deleteMV(size_t txn_id) -> const_snapshot_ptr
    {
        return this->delete_implMV(txn_id);
    }

    template <class ValueType>
    template <class U>
    auto mvcc<ValueType>::delete_implMV(size_t txn_id) -> const_snapshot_ptr
    {

        ///1- Create New-Snapshot initially
        //auto desired = smart_ptr::make_shared<snapshot_type>(txn_id, std::forward<U>(value));

        while (true)
        {

            ///2- Fetch/Read  expected Version speculatively
            auto expected = smart_ptr::atomic_load(&mutable_current_);
            auto const const_expected_version = expected->version;
            auto const const_expected_end_version = expected->end_version;
            auto const &const_expected_value = expected->value;


            ///3- if record was created and its active currently and not by the current transaction
            if (std::find(active_transactionIds.begin(), active_transactionIds.end(), const_expected_version) != active_transactionIds.end()  && const_expected_end_version != INF)
            {
                if (const_expected_version != txn_id)
                {
                    std::cout << "Aborted" << std::endl;
                    continue;
                }
            }

            ///4- return expected old deleted with end version set to txn-id
            expected->end_version = txn_id;
            return expected;

        }
        return nullptr;
    }


    template <class ValueType>
    template <class Updater>
    auto mvcc<ValueType>::update(size_t txn_id,Updater updater) -> const_snapshot_ptr
    {
        while(true)
        {
            auto updated = this->try_update_impl(txn_id,updater);
            if(updated != nullptr)
                return updated;
            std::this_thread::sleep_for(std::chrono::milliseconds(MVCC11_CONTENSION_BACKOFF_SLEEP_MS));
        }
    }

    template <class ValueType>
    template <class Updater>
    auto mvcc<ValueType>::try_update(size_t txn_id,Updater updater) -> const_snapshot_ptr
    {
        return this->try_update_impl(txn_id,updater);
    }

    template <class ValueType>
    template <class Updater, class Clock, class Duration>
    auto mvcc<ValueType>::try_update_until(
            size_t txn_id,
            Updater updater,
            std::chrono::time_point<Clock, Duration> const &timeout_time)
    -> const_snapshot_ptr
    {
        return this->try_update_until_impl(txn_id,updater, timeout_time);
    }

    template <class ValueType>
    template <class Updater, class Rep, class Period>
    auto mvcc<ValueType>::try_update_for(
            size_t txn_id,
            Updater updater,
            std::chrono::duration<Rep, Period> const &timeout_duration)
    -> const_snapshot_ptr
    {
        auto timeout_time = std::chrono::high_resolution_clock::now() + timeout_duration;
        return this->try_update_until_impl(txn_id,updater, timeout_time);
    }


    template <class ValueType>
    template <class Updater>
    auto mvcc<ValueType>::try_update_impl(size_t txn_id,Updater &updater) -> const_snapshot_ptr
    {
            //auto desired = smart_ptr::make_shared<snapshot_type>(txn_id, std::forward<U>(value));

            ///1- Fetch/Read  expected Version speculatively
            auto expected = smart_ptr::atomic_load(&mutable_current_);
            auto const const_expected_version = expected->version;
            auto const const_expected_end_version = expected->end_version;
            auto const &const_expected_value = expected->value;

            ///2- Create New-Snapshot initially
            auto desired = smart_ptr::make_shared<snapshot_type>( txn_id, updater(expected->value));

            ///3- if record was created and its active currently and not by the current transaction
            if ( const_expected_end_version != INF && const_expected_version != txn_id)
            {
                if (std::find(active_transactionIds.begin(), active_transactionIds.end(), const_expected_version) != active_transactionIds.end() )
                {
                    std::cout << "Aborted on value " << expected->value<<" by transaction##"<< txn_id<<std::endl;
                    //continue;
                    return nullptr;
                }
            }

            expected->end_version = txn_id;
            auto const updated = smart_ptr::atomic_compare_exchange_strong(&mutable_current_, &expected, desired);
            if (updated)
            {
                // std::cout << "overwritten =" << txn_id << desired->value<<std::endl;
                ///set status commited
                smart_ptr::atomic_store(&desired->_older_snapshot,expected);
                return desired;
            }

        return nullptr;
    }


    template <class ValueType>
    template <class Updater, class Clock, class Duration>
    auto mvcc<ValueType>::try_update_until_impl(
            size_t txn_id,
            Updater &updater,
            std::chrono::time_point<Clock, Duration> const &timeout_time)
    -> const_snapshot_ptr
    {
        while(true)
        {
            auto updated = this->try_update_impl(txn_id,updater);

            if(updated != nullptr)
                return updated;

            if(std::chrono::high_resolution_clock::now() > timeout_time)
                return nullptr;

            std::this_thread::sleep_for(std::chrono::milliseconds(MVCC11_CONTENSION_BACKOFF_SLEEP_MS));
        }
    }

} // namespace mvcc11

#endif // MVCC11_MVCC_HPP
