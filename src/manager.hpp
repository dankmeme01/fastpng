#pragma once

#include <stdint.h>
#include <vector>
#include <asp/thread/Thread.hpp>
#include <asp/sync/Channel.hpp>

template <typename Derived>
class SingletonBase {
public:
    // no copy
    SingletonBase(const SingletonBase&) = delete;
    SingletonBase& operator=(const SingletonBase&) = delete;
    // no move
    SingletonBase(SingletonBase&&) = delete;
    SingletonBase& operator=(SingletonBase&&) = delete;

    static Derived& get() {
        static Derived instance;

        return instance;
    }

protected:
    SingletonBase() {}
    virtual ~SingletonBase() {}
};

class FastpngManager : public SingletonBase<FastpngManager> {
    friend class SingletonBase;
    FastpngManager();

public:
    bool reallocFromCache(unsigned char*& buf, size_t& size, uint32_t checksum);
    void queueForCache(const std::filesystem::path& path, std::vector<uint8_t>&& data);
    std::filesystem::path getCacheDir();

private:
    using ConverterTask = std::pair<std::filesystem::path, std::vector<uint8_t>>;

    asp::Thread<FastpngManager*> converterThread;
    asp::Channel<ConverterTask> converterQueue;

    void threadFunc();
};