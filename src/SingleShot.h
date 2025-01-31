/*
    This header define a performant generic single data transfer.
    Should only be writed once.
*/
#pragma once
#include <optional>
#include <atomic>
#include <thread>
#include <memory>
#include "Async.h"
namespace async {
    template <class T>
    class SingleShotInner {
        Executor* _callback;
        std::optional<T> _data;
        bool _writer_deleted;
        std::atomic_flag _lock;
        void lock() {
            size_t nb_try = 0;
            while (_lock.test_and_set(std::memory_order_acquire)) {
                if (nb_try >= 3) {
                    std::this_thread::yield();
                }
            }
        }
    public:
        SingleShotInner() :_callback(nullptr),
            _data(std::nullopt),
            _lock(),
            _writer_deleted(false)
        {};
        SingleShotInner(const SingleShotInner&) = delete;
        SingleShotInner(SingleShotInner&&) = delete;
        bool trySetCallBack(Executor* waker) {
            bool retval = !_lock.test_and_set(std::memory_order_acquire);
            if (retval) {
                _callback = waker;
                _lock.clear(std::memory_order_release);
            }
            return retval;
        }
        void setCallBack(Executor* waker) {
            lock();
            _callback = waker;
            if ((_data||_writer_deleted) && _callback) {
                _callback->awake();
            }
            _lock.clear(std::memory_order_release);
        }
        void resetCallBack() {
            setCallBack(nullptr);
        }
        template <class ...Args>
        void setValue(Args&& ...values) {
            lock();
            _data.emplace(std::forward<Args>(values)...);
            if (_callback) {
                _callback->awake();
            }
            _callback = nullptr;
            _lock.clear(std::memory_order_release);
        }
        void closeWriter() {
            lock();
            _writer_deleted = true;
            if (_callback) {
                _callback->awake();
            }
            _lock.clear(std::memory_order_release);
        }
        std::optional<T> tryExtractValue(bool& writer_closed) {
            std::optional<T> retval = std::nullopt;
            if (!_lock.test_and_set(std::memory_order_acquire)) {
                writer_closed = _writer_deleted;
                retval = std::move(_data);
                _lock.clear(std::memory_order_release);
            }
            return retval;
        }
        std::optional<T> extractValue(bool& writer_closed) {
            lock();
            writer_closed = _writer_deleted;
            std::optional<T> retval = std::move(_data);
            _lock.clear(std::memory_order_release);
            return retval;
        }
        std::optional<T> extractValue(Executor* waker) {
            lock();
            std::optional<T> retval = std::move(_data);
            if (!retval) {
                _callback = waker;
            }
            _lock.clear(std::memory_order_release);
            return retval;
        }
    };
    template <class T>
    class SingleShotWriter {
        std::shared_ptr<SingleShotInner<T>> _ss;
    public:
        SingleShotWriter() :_ss() {}
        SingleShotWriter(std::shared_ptr<SingleShotInner<T>> ss) :_ss(std::move(ss)) {}
        SingleShotWriter(const SingleShotWriter<T>&) = delete;
        SingleShotWriter(SingleShotWriter<T>&& src)noexcept :_ss(std::move(src._ss)) {}
        SingleShotWriter& operator=(const SingleShotWriter&) = delete;
        SingleShotWriter& operator=(SingleShotWriter&& src) noexcept {
            _ss = std::move(src._ss);
            return *this;
        }
        template <class ...Args>
        void set_value(Args&& ...value)const {
            _ss->setValue(std::forward<Args>(value)...);
        }
        ~SingleShotWriter(){
            if (_ss) {
                _ss->closeWriter();
            }
        }
    };
    template <class T>
    class SingleShotReader {
        std::shared_ptr<SingleShotInner<T>> _ss;
    public:
        SingleShotReader() :_ss() {}
        SingleShotReader(std::shared_ptr<SingleShotInner<T>> ss) :_ss(std::move(ss)) {}
        SingleShotReader(const SingleShotReader<T>&) = delete;
        SingleShotReader(SingleShotReader<T>&& src)noexcept :_ss(std::move(src._ss)) {}
        SingleShotReader& operator=(const SingleShotReader&) = delete;
        SingleShotReader& operator=(SingleShotReader&& src) noexcept {
            _ss = std::move(src._ss);
            return *this;
        }

        bool trySetCallBack(Executor* callback) const {
            return _ss->trySetCallBack(callback);
        }

        void setCallBack(Executor* callback) const {
            _ss->setCallBack(callback);
        }

        std::optional<T> tryExtractValue(bool& writer_deleted) const {
            return _ss->tryExtractValue(writer_deleted);
        }

        std::optional<T> extractValue(Executor* waker) const {
            return _ss->extractValue(waker);
        }
        std::optional<T> extractValue(bool& writer_deleted) const {
            return _ss->extractValue(writer_deleted);
        }
        ~SingleShotReader() {
            if (_ss) {
                _ss->resetCallBack();
            }
            
        }
    };

    template<class T>
    struct SingleShotReaderAwaitable :public Awaitable<SingleShotReaderAwaitable<T>> {
        const SingleShotReader<T>& _reader;
        mutable std::optional<T> _result;
        using result_type = T;
        SingleShotReaderAwaitable(const SingleShotReader<T>& reader) :_reader(std::move(reader)) {}
        bool poll() const {
            bool writer_deleted = true;
            _result = std::move(_reader.extractValue(writer_deleted));
            return _result.has_value()|| writer_deleted;
        }
        void subscribe(Executor* exe) const{
            _reader.setCallBack(exe);
        }
        std::optional<T>& await_resume() & {
            return _result;
        }
        std::optional<T>&& await_resume()&& {
            return std::move(_result);
        }
    };

    template<class T>
    SingleShotReaderAwaitable<T> operator co_await(const SingleShotReader<T>& reader) {
        return SingleShotReaderAwaitable<T>(reader);
    };



    template<class T>
    class SingleShot {
    private:
        SingleShotReader<T> _reader;
        SingleShotWriter<T> _writer;
    public:
        SingleShot():_reader(),_writer(){
            auto shr = std::make_shared<SingleShotInner<T>>();
            _reader = SingleShotReader(shr);
            _writer = SingleShotWriter(shr);
        }
        SingleShotReader<T>&& getReader() {
            return std::move(_reader);
        }
        SingleShotWriter<T>&& getWriter() {
            return std::move(_writer);
        }
    };
}
