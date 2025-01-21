#ifndef CACHE_HPP
#define CACHE_HPP

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

    // 连接到主存的信号
    sc_out<bool> mem_read;    // 读内存信号
    sc_out<bool> mem_write;   // 写内存信号
    sc_out<uint32_t> mem_address; // 主存地址信号
    sc_in<bool> mem_ready;    // 主存完成信号
    sc_in<uint32_t> mem_r_data;   // 主存返回数据信号

    SC_HAS_PROCESS(Cache);

    Cache(sc_module_name name, uint32_t cache_size, uint32_t line_size);

private:
    struct CacheLine {
        bool valid;
        uint32_t tag;
        std::vector<uint8_t> data;
    };

    uint32_t cache_size;
    uint32_t line_size;
    std::vector<CacheLine> cache;

    void process_cache();
    bool search_cache(uint32_t addr, uint32_t& data);
    void update_cache(uint32_t addr, uint32_t data);
};

#endif