#ifndef MVCC11_SNAPSHOT_HPP
#define MVCC11_SNAPSHOT_HPP

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

#endif // MVCC11_USES_STD_SHARED_PTR

#include <utility>
#include <chrono>
#include <thread>
#define INF 99999

#ifdef MVCC11_DISABLE_NOEXCEPT
#define MVCC11_NOEXCEPT(COND)
#else
#define MVCC11_NOEXCEPT(COND) noexcept(COND)
#endif
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


        void setOlderSnapshotNull();


        smart_ptr::shared_ptr<snapshot> _older_snapshot;
        size_t version;
        size_t end_version;
        value_type value;
    };



    template <class ValueType>
    void snapshot<ValueType>::setOlderSnapshotNull()
    {
        smart_ptr::atomic_store(&_older_snapshot,nullptr);
    }

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
}

#endif // MVCC11_SNAPSHOT_HPP