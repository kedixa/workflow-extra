#ifndef WE_REF_PTR_H
#define WE_REF_PTR_H

#include <atomic>
#include <new>
#include <cstddef>

namespace wfextra {

struct RefContructTag {};

template<typename T>
struct RefDataBlock {
    std::atomic<size_t> ref_cnt;
    T data;

    RefDataBlock(size_t r, const T &t) : ref_cnt(r), data(t) { }
    RefDataBlock(size_t r, T &&t) : ref_cnt(r), data(std::move(t)) { }

    template<typename... Args>
    RefDataBlock(RefContructTag, size_t r, Args&&... args)
        : ref_cnt(r), data(std::forward<Args>(args)...)
    { }
};

template<typename T>
class RefPtr {
    using data_block_t = RefDataBlock<T>;
    using data_ptr_t = data_block_t *;

public:
    RefPtr() : data(nullptr) { }

    RefPtr(const T &t) : data(new data_block_t(1, t)) { }
    RefPtr(T &&t) : data(new data_block_t(1, std::move(t))) { }

    RefPtr(const RefPtr &p) : data(p.data) { this->incref(); }
    RefPtr(RefPtr &&p) : data(p.data) { p.data = nullptr; }

    RefPtr &operator=(const RefPtr &p) {
        if (this != &p) {
            this->reset();

            this->data = p.data;
            this->incref();
        }
    }

    RefPtr &operator=(RefPtr &&p) {
        if (this != &p)
            std::swap(this->data, p.data);
    }

    ~RefPtr() { this->reset(); }

    operator bool() const { return this->data != nullptr; }

    const T *operator->() const { return ptr(); }
    const T &operator*() const { return *ptr(); }

    const T *raw_ptr() const { return ptr(); }
    T *raw_ptr() { return ptr(); }

    void reset() {
        if (this->data && --this->data->ref_cnt == 0) {
            delete this->data;
            this->data = nullptr;
        }
    }

    void reset(const T &t) {
        this->reset();
        this->data = new data_block_t{1, t};
    }

    void reset(T &&t) {
        this->reset();
        this->data = new data_block_t{1, std::move(t)};
    }

    template<typename... Args>
    void reset(Args&&... args) {
        this->reset();
        this->data = new data_block_t(RefContructTag{}, 1, std::forward<Args>(args)...);
    }

private:
    T *ptr() const {
        return this->data ? &this->data->data : nullptr;
    }

    void incref() {
        if (this->data)
            ++this->data->ref_cnt;
    }

private:
    data_ptr_t data;
};

} // namespace wfextra

#endif // WE_REF_PTR_H
