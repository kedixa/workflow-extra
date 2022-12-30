#include <type_traits>

#include "workflow/WFResourcePool.h"
#include "workflow/WFTaskFactory.h"

namespace wfextra {
namespace details {

template<typename T>
struct is_small_object {
    constexpr static bool value = std::is_trivially_constructible<T>::value &&
        sizeof(T) <= sizeof(void *);
};

template<>
struct is_small_object<void> {
    constexpr static bool value = true;
};

template<typename T, bool>
struct object_helper;

template<typename T>
struct object_helper<T, false> {
    static void construct(void **p, T &&t) {
        *p = new T(std::forward<T>(t));
    }

    template<typename... ARGS>
    static void construct(void **p, ARGS&&... args) {
        *p = new T(std::forward<ARGS>(args)...);
    }

    static void destruct(void **p) {
        delete (T *)(*p);
    }

    static T *get(void **p) {
        return (T *)(*p);
    }
};

template<typename T>
struct object_helper<T, true> {
    static void construct(void **p, T &&t) {
        new ((T*)(p)) T(std::move(t));
    }

    template<typename... ARGS>
    static void construct(void **p, ARGS&&... args) {
        new ((T*)(p)) T(std::forward<ARGS>(args)...);
    }

    static void destruct(void **p) {
        ((T*)p)->~T();
    }

    static T *get(void **p) {
        return (T *)(p);
    }
};

template<>
struct object_helper<void, true> {
    static void construct(void **p) { }
    static void destruct(void **p) { }
    static void *get(void **p) { return nullptr; }
};

struct default_pool {
    static WFResourcePool *get_pool() {
        static WFResourcePool pool(16);
        return &pool;
    }
};

template<typename T>
using xtor = object_helper<T, details::is_small_object<T>::value>;

template<typename T, typename U>
struct invoker {
    template<typename FUNC>
    void operator()(void **p, FUNC &&func, U *arg) {
        xtor<T>::construct(p, func(*arg));
    }
};

template<typename T>
struct invoker<T, void> {
    template<typename FUNC>
    void operator()(void **p, FUNC &&func, void *) {
        xtor<T>::construct(p, func());
    }
};

template<typename U>
struct invoker<void, U> {
    template<typename FUNC>
    void operator()(void **p, FUNC &&func, U *arg) {
        func(*arg);
        xtor<void>::construct(p);
    }
};

template<>
struct invoker<void, void> {
    template<typename FUNC>
    void operator()(void **p, FUNC &&func, void *) {
        func();
        xtor<void>::construct(p);
    }
};

template<typename FUNC, typename T>
struct result_t {
    using type = typename std::remove_reference<typename std::result_of<FUNC(T)>::type>::type;
};

template<typename FUNC>
struct result_t<FUNC, void> {
    using type = typename std::remove_reference<typename std::result_of<FUNC()>::type>::type;
};

} // namespace details

using details::xtor;

template<typename T, typename U = T>
class GoPipe {
    static_assert(std::is_same<T, typename std::remove_reference<T>::type>::value);
public:
    GoPipe() {
        auto *empty = WFTaskFactory::create_empty_task();
        series = Workflow::create_series_work(empty, nullptr);
    }
    ~GoPipe() {
        if (series) { /* abort */ };
    }

    template<typename FUNC>
    GoPipe<T, typename details::result_t<FUNC, U>::type>
    then(WFResourcePool *pool, FUNC&& func) {
        using func_ret_t = typename details::result_t<FUNC, U>::type;
        using then_ret_t = GoPipe<T, func_ret_t>;

        SeriesWork *s = this->series;
        auto newfunc = [pool, func, s] () mutable {
            void *out;
            void *in = s->get_context();
            U *input = xtor<U>::get(&in);

            details::invoker<func_ret_t, U>{}(&out, func, input);
            s->set_context(out);
            xtor<U>::destruct(&in);
            pool->post(nullptr);
        };

        auto *go = WFTaskFactory::create_go_task("GoPipe", newfunc);
        this->series->push_back(pool->get(go));

        return then_ret_t(this->series);
    }

    template<typename FUNC>
    GoPipe<T, typename details::result_t<FUNC, U>::type>
    then(FUNC&& func) {
        return then(details::default_pool::get_pool(), std::forward<FUNC>(func));
    }

    template<typename FUNC>
    void start(T t, FUNC&& finally) {
        void *value;
        xtor<T>::construct(&value, std::move(t));
        series->set_context(value);

        series->set_callback([finally](const SeriesWork *s) {
            void *value = s->get_context();
            xtor<U>::destruct(&value);
            finally();
        });

        series->start();
        this->series = nullptr;
    }

    void start(T t) {
        start(t, [](){});
    }

private:
    GoPipe(SeriesWork *s) : series(s) {}

    template<typename, typename> friend class GoPipe;

private:
    SeriesWork *series;
};

} // namespace wfextra
