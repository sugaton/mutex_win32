#pragma once
#include <vector>
#include <queue>
#include "mutex.h"
#include <memory>
#include <windows.h>

using namespace win32_mutex;

template <typename T>
class BufferPool
{
private:
    std::vector<std::unique_ptr<std::vector<T>>> pool; // バッファを保持する
    size_t poolSize;                                  // プールのサイズ
    ConditionVariable pool_cond;                      // pool状態を内部通知
    CriticalSection pool_cs;                         // pool状態用lock

public:
    /// @brief コンストラクタ：指定されたサイズでバッファプールを初期化
    /// @param pool_size
    /// @param reserve_size 各バッファの初期確保メモリサイズ
    explicit BufferPool(size_t pool_size, size_t reserve_size)
        : poolSize(pool_size), pool_cond(), pool_cs()
    {
        for (size_t i = 0; i < poolSize; ++i)
        {
            pool.push_back(std::unique_ptr<std::vector<T>>(new std::vector<T>(0)));
            pool.back()->reserve(reserve_size);
        }
    }

    BufferPool(const BufferPool &) = delete;
    BufferPool &operator=(const BufferPool &) = delete;

    /// @brief バッファを取得する.
    ///        取得できない場合取得可能になるまで待機する.
    /// @return std::unique_ptr<std::vector<T>>
    std::unique_ptr<std::vector<T>> acquireBuffer()
    {
        auto lock = lock_guard(pool_cs);
        if (pool.size() == 0)
        {
            pool_cond.wait(pool_cs,
                           [this]()
                           { return this->pool.size() != 0; });
        }
        auto buffer = std::move(pool.back());
        pool.pop_back();
        return buffer;
    }

    // 返却されたBufferを受け取るメソッド
    void pushBuffer(std::unique_ptr<std::vector<T>> &&buffer)
    {
        auto lock = lock_guard(pool_cs);
        pool.emplace_back(std::move(buffer));
        pool_cond.notify_one();
    }
};

/// @brief BufferPoolからバッファーを受取り、開放されるタイミングでBufferPoolに所有権を返す
/// @tparam T 内部保持するデータタイプ
template <typename T>
class Buffer
{
private:
    std::unique_ptr<std::vector<T>> data;
    BufferPool<T>& src_pool;

public:
    /// @brief BufferPoolから一つバッファを取得し、初期化する。
    ///        取得可能バッファがない場合は取得可能になるまで待機する
    /// @param src
    explicit Buffer(BufferPool<T>& src) : data(src.acquireBuffer()),
        src_pool(src)
    {
        if (!data)
        {
            throw std::invalid_argument("Null unique_ptr passed to Buffer");
        }
    }

    ~Buffer()
    {
        // BufferPoolにバッファを返却する
        src_pool.pushBuffer(std::move(data));
    }

    // コピー禁止
    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;

    // ムーブコンストラクタとムーブ代入演算子をデフォルトで有効化
    Buffer(Buffer&&) noexcept = default;
    Buffer& operator=(Buffer&&) noexcept = default;

    /// @brief
    /// @param src_size
    /// @param src
    void copy(size_t src_size, T* src)
    {
        if (src_size > data->capacity())
        {
            data->reserve(src_size);
        }
        data->assign(src, src + src_size);
    }

    T* getData()
    {
        return data->data();
    }

    size_t size()
    {
        return data->size();
    }
    
    std::vector<T>& get()
    {
        return *(data.get());
    }
};