#pragma once
#include <memory>
namespace std {
    template<class ...>
    class move_only_function;

    template<class RET, class ...PARAMS>
    class move_only_function<RET(PARAMS...)> {
    private:
        struct FnConcept {
            virtual RET call(PARAMS&&... params) = 0;
            virtual ~FnConcept() = default;
        };
        template<class T>
        struct FnModel:public FnConcept {
            T _functor;
            FnModel(T&& functor) :_functor(std::move(functor)) {}
            RET call(PARAMS&&... params) override {
                return _functor(std::forward<PARAMS>(params)...);
            }
        };
        std::unique_ptr<FnConcept> _pimpl;
    public:
        template<class T>
        move_only_function(T&& t) :_pimpl(std::make_unique<FnModel<T>>(std::move(t))) {
        }
        RET operator()(PARAMS&&... params) {
            return _pimpl->call(std::forward<PARAMS>(params)...);
        }
    };
    template<class RET, class ...PARAMS>
    class move_only_function<RET(PARAMS...) const> {
    private:
        struct FnConcept {
            virtual RET call(PARAMS&&... params) const = 0;
            virtual ~FnConcept() = default;
        };
        template<class T>
        struct FnModel :public FnConcept {
            T _functor;
            FnModel(T&& functor) :_functor(std::move(functor)) {}
            RET call(PARAMS&&... params) const override {
                return _functor(std::forward<PARAMS>(params)...);
            }
        };
        std::unique_ptr<FnConcept> _pimpl;
    public:
        template<class T>
        move_only_function(T&& t) :_pimpl(std::make_unique<FnModel<T>>(std::move(t))) {
        }
        RET operator()(PARAMS&&... params) const{
            return _pimpl->call(std::forward<PARAMS>(params)...);
        }
    };
    template<>
    class move_only_function<void(void)> {
    private:
        struct FnConcept {
            virtual void call(void) = 0;
            virtual ~FnConcept() = default;
        };
        template<class T>
        struct FnModel :public FnConcept {
            T _functor;
            FnModel(T&& functor) :_functor(std::move(functor)) {}
            void call(void) override {
                _functor();
            }
        };
        std::unique_ptr<FnConcept> _pimpl;
    public:
        template<class T>
        move_only_function(T&& t) :_pimpl(std::make_unique<FnModel<T>>(std::move(t))) {
        }
        void operator()(void) {
            return _pimpl->call();
        }
    };

    template<>
    class move_only_function<void(void) const> {
    private:
        struct FnConcept {
            virtual void call(void) const = 0;
            virtual ~FnConcept() = default;
        };
        template<class T>
        struct FnModel :public FnConcept {
            T _functor;
            FnModel(T&& functor) :_functor(std::move(functor)) {}
            void call(void) const override {
                _functor();
            }
        };
        std::unique_ptr<FnConcept> _pimpl;
    public:
        template<class T>
        move_only_function(T&& t) :_pimpl(std::make_unique<FnModel<T>>(std::move(t))) {}
        void operator()(void) const {
            return _pimpl->call();
        }
    };
}
