#include <systemc.h>
#include <vector>
#include <iostream>

// Cache 模块定义
class Cache : public sc_module {
public:
    // Ports
    sc_in<bool> clk;          // 时钟信号
    sc_in<bool> read;         // 读操作信号
    sc_in<bool> write;        // 写操作信号
    sc_in<uint32_t> address;  // 地址信号
    sc_in<uint32_t> w_data;   // 写入数据信号
    sc_out<uint32_t> r_data;  // 读出数据信号
    sc_out<bool> ready;       // 操作完成信号

    SC_CTOR(Cache);

private:
    struct CacheLine {
        bool valid;
        uint32_t tag;
        std::vector<uint8_t> data;
    };

    // 缓存数据结构
    std::vector<std::vector<CacheLine> > caches; // 多级缓存
    std::vector<uint32_t> cache_sizes;          // 每级缓存大小
    std::vector<uint32_t> line_sizes;           // 每级缓存行大小
    std::vector<uint32_t> latencies;            // 每级缓存访问延迟

    uint8_t levels = 2; // 默认两级缓存（可以扩展）

    void process_cache();
    bool search_cache(uint32_t level, uint32_t addr, uint32_t& data);
    void update_cache(uint32_t level, uint32_t addr, uint32_t data);
};

// 构造函数：初始化多级缓存
Cache::Cache(sc_module_name name) : sc_module(name), 
    cache_sizes{1024, 2048}, line_sizes{64, 64}, latencies{1, 3} 
{

    caches.resize(levels);
    for (uint8_t i = 0; i < levels; i++) {
        uint32_t num_lines = cache_sizes[i] / line_sizes[i];
        caches[i].resize(num_lines, {false, 0, std::vector<uint8_t>(line_sizes[i], 0)});
    }

    SC_THREAD(process_cache);
    sensitive << clk.pos();
}

// 处理缓存逻辑
void Cache::process_cache() {
    while (true) {
        wait();
        ready.write(false);

        uint32_t addr = address.read();
        uint32_t data = 0;

        if (read.read()) {
            bool hit = false;

            // 逐级检查缓存
            for (uint8_t level = 0; level < levels; level++) {
                if (search_cache(level, addr, data)) {
                    std::cout << "Cache hit at level " << (int)level + 1 << std::endl;
                    r_data.write(data);
                    hit = true;
                    wait(latencies[level], SC_NS); // 模拟延迟
                    break;
                }
            }

            // 如果所有级别都未命中
            if (!hit) {
                std::cout << "Cache miss! Fetching from memory." << std::endl;
                data = 0xDEADBEEF; // 假设从主存返回的数据
                r_data.write(data);
                for (uint8_t level = 0; level < levels; level++) {
                    update_cache(level, addr, data);
                }
            }
            ready.write(true);
        } else if (write.read()) {
            // 写操作逻辑
            uint32_t data_to_write = w_data.read();

            // 写入所有缓存级别（write-through）
            for (uint8_t level = 0; level < levels; level++) {
                update_cache(level, addr, data_to_write);
            }

            std::cout << "Written data to all cache levels!" << std::endl;
            ready.write(true);
        }
    }
}

// 查找缓存
bool Cache::search_cache(uint32_t level, uint32_t addr, uint32_t& data) {
    uint32_t tag = addr / line_sizes[level];
    uint32_t index = (addr / line_sizes[level]) % caches[level].size();
    uint32_t offset = addr % line_sizes[level];

    CacheLine& line = caches[level][index];
    if (line.valid && line.tag == tag) {
        data = 0;
        for (int i = 0; i < 4; i++) {
            data = (data << 8) | line.data[offset + i];
        }
        return true; // Cache hit
    }
    return false; // Cache miss
}

// 更新缓存
void Cache::update_cache(uint32_t level, uint32_t addr, uint32_t data) {
    uint32_t tag = addr / line_sizes[level];
    uint32_t index = (addr / line_sizes[level]) % caches[level].size();
    uint32_t offset = addr % line_sizes[level];

    CacheLine& line = caches[level][index];
    line.valid = true;
    line.tag = tag;
    for (int i = 3; i >= 0; i--) {
        line.data[offset + i] = data & 0xFF;
        data >>= 8;
    }
}

// 主程序
int sc_main(int argc, char** argv) {
    sc_signal<bool> w_signal, r_signal, ready_signal;
    sc_signal<uint32_t> wdata, addr, rdata;
    sc_clock clk_signal("clk_signal", 10, SC_NS);

    // 实例化缓存模块
    Cache cache("Cache");

    // 信号连接
    cache.clk(clk_signal);
    cache.read(r_signal);
    cache.write(w_signal);
    cache.address(addr);
    cache.w_data(wdata);
    cache.r_data(rdata);
    cache.ready(ready_signal);

    // 测试用例 1: 写数据
    std::cout << "[TEST 1] Writing data 0x12345678 to address 0x00000000" << std::endl;
    wdata.write(0x12345678);
    addr.write(0x00000000);
    w_signal.write(true);
    r_signal.write(false);
    sc_start(10, SC_NS);

    // 测试用例 2: 读数据
    std::cout << "[TEST 2] Reading data from address 0x00000000" << std::endl;
    w_signal.write(false);
    r_signal.write(true);
    sc_start(10, SC_NS);

    std::cout << "Read data: " << std::hex << rdata.read() << std::endl;

    // 测试用例 3: 缓存未命中
    std::cout << "[TEST 3] Reading data from address 0x00001000 (Cache miss)" << std::endl;
    addr.write(0x00001000);
    sc_start(10, SC_NS);
    std::cout << "Read data: " << std::hex << rdata.read() << std::endl;

    // 结束仿真
    std::cout << "Simulation ends" << std::endl;

    return 0;
}
