#pragma once
#include <queue>
#include <mutex>
#include "Async.h"
#include <memory>
namespace async {
    template<class T>
    class UnBoundedSPSCInner {
    private:
        std::queue<T> _queue;
        std::mutex _mtx;
        Executor* _executor;//consumer's executor.
        bool _writer_closed;
    public:
        using value_type = decltype(_queue)::value_type;
        UnBoundedSPSCInner() :
            _queue(),
            _mtx(),
            _executor(nullptr),
            _writer_closed(false) {}
        template<class ...Args>
        void emplace(Args&& ...args) {
            std::unique_lock lck(_mtx);
            _queue.emplace(std::forward<Args>(args)...);
            if (_executor) {
                _executor->awake();
            }
        }
        void push(T&& element) {
            std::unique_lock lck(_mtx);
            _queue.push(std::move(element));
            if (_executor) {
                _executor->awake();
            }
        }
        void push(const T& element) {
            std::unique_lock lck(_mtx);
            _queue.push(element);
            if (_executor) {
                _executor->awake();
            }
        }
        void closeWriter() {
            std::unique_lock lck(_mtx);7
            _writer_closed = true;
            if (_executor) {
                _executor->awake();
            }
            
        }
        void setConsumerExecutor(Executor* executor) {
            std::unique_lock lck(_mtx);
            _executor = executor;
            if (executor && (!_queue.empty() || _writer_closed)) {
                executor->awake();
            }
        }
        std::optional<T> tryPop(bool& writer_closed) {
            std::optional<T> retval = std::nullopt;
            std::unique_lock lck(_mtx);
            writer_closed = _writer_closed;
            if (!_queue.empty()) {
                retval = std::move(_queue.front());
                _queue.pop();
            }
            return retval;
        }
    };
    /*
        SPSC Queue write.

        Cannot be copied. Only move.
    */
    template<class T>
    class SPSCWriter {
    private:
        std::shared_ptr<UnBoundedSPSCInner<T>> _spsc;
    public:
        SPSCWriter() :_spsc() {};
        SPSCWriter(std::shared_ptr<UnBoundedSPSCInner<T>> spsc) :_spsc(std::move(spsc)) {}
        SPSCWriter(const SPSCWriter&) = delete;//not copiable
        SPSCWriter& operator =(const SPSCWriter&) = delete;//not copiable
        SPSCWriter(SPSCWriter&& src) noexcept :_spsc(std::move(src._spsc)) {};
        SPSCWriter& operator =(SPSCWriter&& src) noexcept {
            this->_spsc = std::move(src._spsc);
            return *this;
        };
        /*
           emplace element at the end.
        */
        template<class ...Args>
        void emplace(Args&& ...args) const {
            _spsc->emplace(std::forward<Args>(args)...);
        }
        /*
           Push element at the end.
           move overload
        */
        void push(T&& element) const {
            _spsc->push(std::move(element));
        }
        /*
            Push element at the end.
            Copy overload
        */
        void push(const T& element)const {
            _spsc->push(element);
        }

        ~SPSCWriter() {
            if (_spsc) {
                //Signal the writer is deleted.
                _spsc->closeWriter();
            }
        }
    };

    /*
        SPSC Queue reader.

        Cannot be copied. Only move.
    */
    template<class T>
    class SPSCReader {
    private:
        std::shared_ptr<UnBoundedSPSCInner<T>> _spsc;
    public:
        SPSCReader() :_spsc(){}
        SPSCReader(std::shared_ptr<UnBoundedSPSCInner<T>> spsc) :_spsc(std::move(spsc)) {}
        SPSCReader(const SPSCReader&) = delete;
        SPSCReader(SPSCReader&& src) noexcept :_spsc(std::move(src._spsc)) {};
        SPSCReader& operator =(const SPSCReader&) = delete;
        SPSCReader& operator =(SPSCReader&& src) noexcept {
            this->_spsc = std::move(src._spsc);
            return *this;
        };
        std::optional<T> tryPop(bool& data) const {
            return _spsc->tryPop(data);
        }
        /*
            Asynchronously get one element from the spsc queue.
            If the result is none,the writer is deleted (no elements will be inserted anymore).
        */
        AwaitableConcept<std::optional<T>> auto popAsync() const{
            struct AWT :Awaitable<AWT> {
                const SPSCReader<T>& spsc;
                std::optional<T> data;
                AWT(const SPSCReader<T>& spsc) :spsc(spsc), data() {}
                bool poll() noexcept {
                    bool writer_closed = false;
                    data = std::move(spsc.tryPop(writer_closed));
                    return data.has_value() || writer_closed;
                }
                void subscribe(Executor* exe) const noexcept {
                    spsc._spsc->setConsumerExecutor(exe);
                }
                std::optional<T>&& await_resume() noexcept {
                    return std::move(data);
                }
            };
            return AWT(*this);
        }
        ~SPSCReader() {
            if (_spsc) {
                _spsc->setConsumerExecutor(nullptr);
            }
        }
    };
    template<class T>
    class UnboundedSPSC {
    private:
        SPSCReader<T> _reader;
        SPSCWriter<T> _writer;
    public:
        UnboundedSPSC() {
            auto shrd = std::make_shared<UnBoundedSPSCInner<T>>();
            _reader = SPSCReader(shrd);
            _writer = SPSCWriter(shrd);
        }
        SPSCReader<T>&& getReader() noexcept {
            return std::move(_reader);
        }
        SPSCWriter<T>&& getWriter()noexcept {
            return std::move(_writer);
        }
    };
    namespace utils{
        template<class T>
        AsyncCoroutine<std::vector<T>> collect(const SPSCReader<T>& reader) {
            std::vector<T> retval;
            std::optional<T> data;
            do {
                data = std::move(co_await reader.popAsync());
                if (data) {
                    retval.push_back(*data);
                }
            } while (data);
            co_return retval;
        }
    }
}