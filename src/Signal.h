#pragma once
#include <atomic>
#include "Async.h"
#include <memory>
#include <SharedPoolAllocator.h>
namespace async {
    class SignalInner {
    private:
        Executor* _executor;
        std::atomic_bool _flg;
        bool _ready;
        
    public:
        SignalInner():_executor(nullptr),_flg(false),_ready(false) {

        }
        void ready() {
            while (_flg.exchange(true, std::memory_order_acquire)) {
                //to do sleep or yield
            }
            _ready = true;
            if (_executor) {
                _executor->awake();
            }
            _flg.store(false, std::memory_order_release);
        }
        bool is_ready() noexcept{
            bool retval;
            while (_flg.exchange(true, std::memory_order_acquire)) {
                //to do sleep or yield
            }
            retval = _ready;
            _flg.store(false, std::memory_order_release);
            return retval;
        }
        void setExecutor(Executor* executor) {
            while (_flg.exchange(true, std::memory_order_acquire)) {
                //to do sleep or yield
            }
            _executor = executor;
            if (executor && _ready) {
                _executor->awake();
            }
            _flg.store(false, std::memory_order_release);
        }
    };
    class SignalWriter {
    private:
        std::shared_ptr<SignalInner> _inner;
    public:
        SignalWriter() :_inner(nullptr) {}
        SignalWriter(std::shared_ptr<SignalInner> inner) :_inner(std::move(inner)) {
        }
        SignalWriter(const SignalWriter&) = delete;
        SignalWriter(SignalWriter&& src) noexcept:_inner(std::move(src._inner)) {
        };

        SignalWriter& operator=(SignalWriter&& src)noexcept {
            if (&src == this) {
                return *this;
            }
            _inner = std::move(src._inner);
            return *this;
        }
        void ready()const {
            _inner->ready();
        }
        ~SignalWriter() {
            if (_inner) {
                _inner->ready();
                
            }
        }
    };

    class SignalReader {
    private:
        std::shared_ptr<SignalInner> _inner;
    public:
        SignalReader():_inner(nullptr){}
        SignalReader(std::shared_ptr<SignalInner> inner) :_inner(std::move(inner)) {
        }
        SignalReader(SignalReader&& src) noexcept :_inner(std::move(src._inner)) {
        };

        SignalReader(const SignalReader&) = delete;

        SignalReader& operator=(SignalReader&& src)  {
            if (&src == this) {
                return *this;
            }
            _inner = std::move(src._inner);
            return *this;
        }
        bool isReady() const noexcept{
            return _inner->is_ready();
        }
        void setExecutor(Executor* executor) const {
            _inner->setExecutor(executor);
        }
        ~SignalReader() {
        }
    };
    
    struct SignalAwaitable :public Awaitable<SignalAwaitable> {
    private:
        const SignalReader& _reader;
    public:
        SignalAwaitable(const SignalReader& reader):_reader(reader){}
        bool poll() const {
            return _reader.isReady();
        }
        void subscribe(Executor* exe) const {
            _reader.setExecutor(exe);
        }
        constexpr void await_resume() noexcept{
            
        }
    };
    SignalAwaitable operator co_await(const SignalReader& reader) {
        return SignalAwaitable(reader);
    }
    class Signal {
    private:
        SignalWriter _writer;
        SignalReader _reader;
    public:
        Signal():_writer(),_reader() {
            auto _inner=std::allocate_shared<SignalInner>(PooledAlloc<SignalInner>());
            _writer = SignalWriter(_inner);
            _reader = SignalReader(_inner);
        }
        SignalWriter&& get_writer() {
            return std::move(_writer);
        }
        SignalReader&& get_reader() {
            return std::move(_reader);
        }
    };
}
