
#include <iostream>
#include <vector>
#include <array>
#include <thread>
#include "bufferPool.h"

#include "mutex.h"
int main()
{
    BufferPool<int> pool(3, 10);
    std::vector<std::thread> workers;
    CriticalSection cs;
    for (int i = 0; i < 4; i++) {
        workers.emplace_back(std::thread(
            [&pool, &cs](){               
                Buffer<int> buf(pool);
                {
                    auto lock = lock_guard(cs);
                    std::cout << "before:" << std::this_thread::get_id() <<  std::endl;
                    for (auto& i : buf.get())
                        std::cout << i << std::endl;
                }
                std::array<int, 5> a = { 1,2,3,4,5 };

                std::this_thread::sleep_for(std::chrono::seconds(1));
                buf.copy(5, a.data());
                {
                    auto lock = lock_guard(cs);
                    std::cout << "after:" << std::this_thread::get_id() <<  std::endl;
                    for (auto& i : buf.get())
                        std::cout << i << std::endl;
                }
            }
        )); 
    }
    for (auto& w : workers) {
        w.join();
    }
}
