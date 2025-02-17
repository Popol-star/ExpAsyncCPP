#pragma once
#include <coroutine>
#include <optional>
#include <tuple>
#include <variant>
namespace async {
/*
    Interface defining when a awaitable is ready.
*/
struct Pollable {
    //Check if the struct is ready.
    //if return true, the executor should resume.
    virtual bool is_ready() = 0;
};
struct Timer{
    size_t remaining;
    Timer(size_t ms):remaining(ms){
         
    }

};
/*
    Executor Interface.
*/
struct Executor {
public:
    //wake up the executor callback. Must be thread safe.
    virtual void awake() = 0;

    enum class TaskPriority
    {
        Direct,//awake() not needed to invoke the firt poll.
        Indirect,//awake() needed to invoke the firt poll.
    };
    //add a waiting task to the executor.(always called on the executor thread)
    //if pollable is null, the coroutine_handle is always resumed when polled.
    virtual void add_task(std::coroutine_handle<> handle, Pollable* pollable,TaskPriority priority= TaskPriority::Indirect) = 0;
    
    void add_task(std::coroutine_handle<> handle, TaskPriority priority = TaskPriority::Indirect){
        add_task(handle, nullptr, priority);
    }
    /*
    
    */
    virtual void register_timer(Timer* timer)=0;
    //maybe virtual destructor?
    //I dunno, maybe?
};

/*
    The coroutine call executor::awake for final suspend.
*/
struct AsyncFinalWait {
    Executor* executor;
    constexpr bool await_ready() const noexcept {
        return false;
    }
    bool await_suspend(std::coroutine_handle<> handle) noexcept {
        if (executor) {
            executor->awake();
        }
        return true;
    }
    constexpr void await_resume() noexcept {}
};

template <class T>
struct AsyncPromise {
    std::optional<T> _data = std::nullopt;//the data returned by the coroutine
    Executor* executor = nullptr;
    std::coroutine_handle<AsyncPromise<T>> get_return_object() {
        return std::coroutine_handle<AsyncPromise<T>>::from_promise(*this);
    }
    constexpr std::suspend_always initial_suspend() noexcept {
        return { };
    }
    AsyncFinalWait final_suspend() noexcept {
        return { executor };
    }
    template<class ...ARGS>
    void return_value(ARGS&& ...args) {
        _data.emplace(std::forward<ARGS>(args)...);
    }
    void unhandled_exception() const noexcept{}
};


template <>
struct AsyncPromise<void> {
    Executor* executor = nullptr;
    std::coroutine_handle<AsyncPromise<void>> get_return_object() {
        return std::coroutine_handle<AsyncPromise<void>>::from_promise(*this);
    }
    constexpr std::suspend_always initial_suspend() noexcept {
        return { };
    }
    AsyncFinalWait final_suspend() noexcept {
        return {executor};
    }
    constexpr void return_void()const noexcept {}
    constexpr void unhandled_exception()const noexcept {}
};

template <class T>
struct AsyncCoroutine {      
    using promise_type = AsyncPromise<T>;
    using return_type = T;
    AsyncCoroutine() :handle(nullptr){}
    AsyncCoroutine(std::coroutine_handle<AsyncPromise<T>> handle) : handle{ handle } {}
    AsyncCoroutine(const AsyncCoroutine&) = delete;//not copiable
    AsyncCoroutine(AsyncCoroutine&& src) noexcept:handle(src.handle) {
        src.handle = nullptr;
    };
    AsyncCoroutine& operator=(const AsyncCoroutine&) = delete;
    AsyncCoroutine& operator=(AsyncCoroutine&& src) noexcept {
        if (&src == this) {
            return *this;
        }
        if (handle) {
            handle.destroy();
        }
        handle = src.handle;
        src.handle = nullptr;
        return *this;
    };
    void resume()const {
        handle.resume();
    }
    bool done() const noexcept{
        return handle.done();
    }
    operator bool() const noexcept {
        return handle != nullptr;
    }
    //should not be called if not done
    return_type& get_return_value() & {
        return *(handle.promise()._data);
    }
    /*
    * Get the value returned by the coroutine.
    * Should not be called if not done.
    */
    
    return_type&& get_return_value()&& {
        return std::move(*(handle.promise()._data));
    }
    /*
    * Get the value returned by the coroutine.
    * Should not be called if not done.
    * Const overloading.
    */
    const return_type& get_return_value()const & {
        return *(handle.promise()._data);
    }
    /*
    * Get the value returned by the coroutine.
    * Should not be called if not done.
    * Rvalue overloading.
    */
    void set_executor(Executor* executor) {
        handle.promise().executor = executor;
    }
    std::coroutine_handle<AsyncPromise<T>> getHandle() const noexcept {
        return handle;
    }
    ~AsyncCoroutine() {
        if (handle) {
            handle.destroy();
        }
    }
    std::coroutine_handle<AsyncPromise<T>> handle;
};

template <>
struct AsyncCoroutine<void> {
    using promise_type= AsyncPromise<void>;
    using return_type = std::monostate;
    AsyncCoroutine() :handle(nullptr) {}
    AsyncCoroutine(std::coroutine_handle<AsyncPromise<void>> handle) : handle{ handle } {}
    AsyncCoroutine(const AsyncCoroutine&) = delete;
    AsyncCoroutine(AsyncCoroutine<void>&& src) noexcept :handle(src.handle) {
        src.handle = nullptr;
    };
    AsyncCoroutine& operator=(const AsyncCoroutine&) = delete;
    AsyncCoroutine& operator=(AsyncCoroutine&& src) noexcept {
        if (&src == this) {
            return *this;
        }
        if (handle) {
            handle.destroy();
        }
        handle = src.handle;
        src.handle = nullptr;
        return *this;
    };
    void resume() const {
        handle.resume();
    }
    bool done() const noexcept {
        return handle.done();
    }
    operator bool() const noexcept {
        return handle!=nullptr;
    }
    void set_executor(Executor* executor)const {
        handle.promise().executor = executor;
    }
    std::coroutine_handle<AsyncPromise<void>> getHandle() const noexcept {
        return handle;
    }
    ~AsyncCoroutine() {
        if (handle) {
            handle.destroy();
        }
    }
    constexpr return_type get_return_value() const noexcept {
        return {};
    }
    std::coroutine_handle<AsyncPromise<void>> handle;
};
/*
    CRTP class for Awaitable.

*/
template<class T>
struct Awaitable: private Pollable {
    // Inherited via Pollable
    bool is_ready() override
    {
        //return poll()
        return (static_cast<T*>(this))->poll();
    }
    bool await_ready()
    {
        //return poll()
        return (static_cast<T*>(this))->poll();
    }
    template <class RET>
    bool await_suspend(std::coroutine_handle<AsyncPromise<RET>> handle) {
        if (handle && handle.promise().executor) {
            (static_cast<T*>(this))->subscribe(handle.promise().executor);
            handle.promise().executor->add_task(handle, this);
        }
        return true;
    }
};
 
template<class T>
struct AsyncCoroutineAwaitable :public Awaitable<AsyncCoroutineAwaitable<T>> {
    AsyncCoroutine<T>&& _coroutine;
    using result_type = T;
    AsyncCoroutineAwaitable(AsyncCoroutine<T>&& coro) :_coroutine(std::move(coro)) {}
    bool poll() noexcept {
        return _coroutine.done();
    }
    void subscribe(Executor* executor) {
        _coroutine.set_executor(executor);
        executor->add_task(_coroutine.getHandle(), nullptr,Executor::TaskPriority::Direct);
        //executor->awake();
    }
    T&& await_resume() {
        return std::move(*(_coroutine.getHandle().promise()._data));
    }
};

template<>
struct AsyncCoroutineAwaitable<void> :public Awaitable<AsyncCoroutineAwaitable<void>> {
    AsyncCoroutine<void>&& _coroutine;
    using result_type = std::monostate;

    AsyncCoroutineAwaitable(AsyncCoroutine<void>&& coro) :
        _coroutine(std::move(coro)) {}

    //Check if the the coroutine is finished or not.
    bool poll() const noexcept {
        return _coroutine.done();
    }
    //Define how to subsribe executor.
    void subscribe(Executor* executor) {
        _coroutine.set_executor(executor);
        executor->add_task(_coroutine.getHandle(), nullptr,Executor::TaskPriority::Direct);
        //executor->awake();
    }
    //
    constexpr std::monostate await_resume() const noexcept {
        return {};
    }
};
template<class T>
AsyncCoroutineAwaitable<T> operator co_await(AsyncCoroutine<T>&& coro) {
    return AsyncCoroutineAwaitable(std::move(coro));
}
template <typename Awaitable>
auto to_awaitable(Awaitable&& awaitable) {
    // Applique operator co_await si disponible
    if constexpr (requires { operator co_await(std::forward<Awaitable>(awaitable)); }) {
        return operator co_await(std::forward<Awaitable>(awaitable));
    }
    else {
        // Sinon, retourne tel quel
        return std::forward<Awaitable>(awaitable);
    }
}
class TimerAwaitable: public Awaitable<TimerAwaitable>{
    Timer&& timer;
    public:
    TimerAwaitable(Timer&& t):timer(std::move(t)){}
    // Vérifie si tous les awaitables sont prêts
    bool poll() const {
        return timer.remaining==0;
    }

    // Souscrire tous les awaitables à un exécuteur
    void subscribe(Executor* executor) {
        executor->register_timer(&timer);
    }

    // Récupère les résultats de tous les awaitables
    constexpr void  await_resume() noexcept{
    }
};

inline TimerAwaitable operator co_await(Timer&& timer) {
    return TimerAwaitable(std::move(timer));
}
template<class... Awaitables>
struct GenericJoinAwaitable : Awaitable<GenericJoinAwaitable<Awaitables...>> {
    std::tuple<Awaitables...> awaitables;

    explicit GenericJoinAwaitable(Awaitables&&... awaitables)
        : awaitables(std::forward<Awaitables>(awaitables)...) {}

    // Vérifie si tous les awaitables sont prêts
    bool poll() {
        return std::apply([](auto&... a) { return (a.poll() && ...); }, awaitables);
    }

    // Souscrire tous les awaitables à un exécuteur
    void subscribe(Executor* executor) {
        std::apply([executor](auto&... a) { (a.subscribe(executor), ...); }, awaitables);
    }

    // Récupère les résultats de tous les awaitables
    auto await_resume() {
        return std::apply([](auto&... a) { return std::make_tuple(a.await_resume()...); }, awaitables);
    }
};

template<class... Awaitables>
struct TaskJoinAwaitable : Awaitable<GenericJoinAwaitable<Awaitables...>> {
    std::tuple<Awaitables...> awaitables;

    explicit TaskJoinAwaitable(Awaitables&&... awaitables)
        : awaitables(std::forward<Awaitables>(awaitables)...) {}

    // Vérifie si tous les awaitables sont prêts
    bool poll() {
        return std::apply([](auto&... a) { return (a.poll() && ...); }, awaitables);
    }

    // Souscrire tous les awaitables à un exécuteur
    void subscribe(Executor* executor) {
        std::apply([executor](auto&... a) { (a.subscribe(executor), ...); }, awaitables);
    }

    // Récupère les résultats de tous les awaitables
    constexpr void await_resume() const noexcept{}
};
template<class T,class RETURN>
concept AwaitableConcept = requires(T a,Executor* e)
{
    std::is_base_of<Awaitable<T>, T>::value; 
    { a.poll() } -> std::same_as<bool>;
    { a.subscribe(e) } -> std::same_as<void>;
     requires std::common_reference_with<decltype(a.await_resume()), RETURN>;
};

template<class T,class E>
concept ToAwaitable = requires(T a) {
    { operator co_await (a) }->AwaitableConcept<E>;
};

template <typename... AwaitableParams>
[[nodiscard("use co_await with async::join")]] auto join(AwaitableParams&&... params) {
    // Applique `to_awaitable` sur chaque paramètre pour récupérer les awaitables
    return GenericJoinAwaitable(to_awaitable(std::forward<AwaitableParams>(params))...);
}
template <typename... AwaitableParams>
[[nodiscard("use co_await with async::join_task")]] auto join_task(AwaitableParams&&... params) {
    return TaskJoinAwaitable(to_awaitable(std::forward<AwaitableParams>(params))...);
}
}