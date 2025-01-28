#pragma once
#include <queue>
#include <mutex>
#include "Async.h"
template<class T>
class UnBoundedSPSC {
private:
      std::queue<T> _queue;
      std::mutex _mtx;
      Executor* _executor;//consumer's executor.
      bool _writer_closed;
public:
      using value_type = decltype(_queue)::value_type;
      UnBoundedSPSC() :
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
          std::unique_lock lck(_mtx);
          _writer_closed = true;
      }
      void setConsumerExecutor(Executor* executor) {
          std::unique_lock lck(_mtx);
          _executor = executor;
          if (executor&&(!_queue.empty()|| _writer_closed)){
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

template<class T>
class SPSCWriter {
private:
    std::shared_ptr<UnBoundedSPSC<T>> _spsc;
public:
    SPSCWriter(std::shared_ptr<UnBoundedSPSC<T>> spsc) :_spsc(std::move(spsc)) {}
    SPSCWriter(const SPSCWriter&) = delete;
    SPSCWriter& operator =(const SPSCWriter&) = delete;
    SPSCWriter(SPSCWriter&& src) noexcept :_spsc(std::move(src._spsc)) {};
    SPSCWriter& operator =(SPSCWriter&& src) noexcept {
        this->_spsc = std::move(src._spsc);
        return *this;
    };

    template<class ...Args>
    void emplace(Args&& ...args) {
        _spsc->emplace(std::forward<Args>(args)...);
    }

    void push(T&& element) {
        _spsc->push(std::move(element));
    }

    void push(const T& element) {
        _spsc->push(element);
    }
    
    ~SPSCWriter() {
        if (_spsc) {
            _spsc->closeWriter();
        }
    }
};

template<class T>
class SPSCReader {
private:
    std::shared_ptr<UnBoundedSPSC<T>> _spsc;
public:
    SPSCReader(std::shared_ptr<UnBoundedSPSC<T>> spsc):_spsc(std::move(spsc)){}
    SPSCReader(const SPSCReader&) = delete;
    SPSCReader(SPSCReader&& src) noexcept:_spsc(std::move(src._spsc)) {};
    AwaitableConcept<std::optional<T>> auto popAsync() {
        struct AWT:Awaitable<AWT>{
            std::shared_ptr<UnBoundedSPSC<T>> spsc;
            std::optional<T> data;
            AWT(std::shared_ptr<UnBoundedSPSC<T>> spsc) :spsc(spsc), data() {}
            bool poll() noexcept {
               bool writer_closed=false;
               data=std::move(spsc->tryPop(writer_closed));
               return data.has_value()||writer_closed;
            }
            void subscribe(Executor* exe) const noexcept {
                spsc->setConsumerExecutor(exe);
            }
            std::optional<T>&& await_resume() noexcept {
                return std::move(data);
            }
        };
        return AWT(_spsc);
    }
    ~SPSCReader() {
        _spsc->setConsumerExecutor(nullptr);
    }
};