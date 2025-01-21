#include <systemc.h>
#include <vector>
#include <iostream>

// Memory 模块定义
class Memory : public sc_module {
public:
    // Ports
    sc_in<bool> clk;          // 时钟信号
    sc_in<bool> read;         // 读操作信号
    sc_in<bool> write;        // 写操作信号
    sc_in<uint32_t> address;  // 地址信号
    sc_in<uint32_t> w_data;   // 写入数据信号
    sc_out<uint32_t> r_data;  // 读出数据信号
    sc_out<bool> ready;       // 操作完成信号

    SC_CTOR(Memory);

private:
    // 内存数据结构
    std::vector<std::vector<uint8_t> > memory; // 模拟的分页内存
    int page_size = 4 * 1024;                 // 每页大小：4 KiB
    int page_num = 1024 * 1024;               // 页数：2^20

    void process_memory(); // 内存操作逻辑
};

// 构造函数：初始化内存并定义线程
Memory::Memory(sc_module_name name) : sc_module(name) {
    // 初始化分页内存
    memory.resize(page_num, std::vector<uint8_t>(page_size, 0));

    // 定义线程
    SC_THREAD(process_memory);
    sensitive << clk.pos(); // 对时钟上升沿敏感
}

// 内存操作逻辑
void Memory::process_memory() {
    while (true) {
        wait(); // 等待时钟上升沿
        ready.write(false);

        uint32_t addr = address.read();
        int page = addr / page_size;
        int position = addr % page_size;
        uint32_t data = 0;

        if (read.read() && write.read()) {
            std::cerr << "Simultaneous read and write detected!" << std::endl;
        } else if (read.read()) {
            // 读操作
            for (int i = 3; i >= 0; i--) { // 每次读取 4 字节
                data = (data << 8) | memory[page][position + i];
            }
            r_data.write(data);
            std::cout << "Read data: " << std::hex << data << " from address: " << addr << std::endl;
        } else if (write.read()) {
            // 写操作
            data = w_data.read();
            for (int i = 0; i < 4; i++) { // 每次写入 4 字节
                memory[page][position + i] = data & 0xFF;
                data >>= 8;
            }
            std::cout << "Written data: " << std::hex << w_data.read() << " to address: " << addr << std::endl;
        }

        ready.write(true); // 操作完成
    }
}

// 主程序
int sc_main(int argc, char** argv) {
    // 打印仿真启动信息
    std::cout << "Simulation starts" << std::endl;

    // 定义信号
    sc_signal<bool> w_signal, r_signal, ready_signal;
    sc_signal<uint32_t> wdata, addr, rdata;
    sc_clock clk_signal("clk_signal", 10, SC_NS); // 时钟周期 10ns

    // 实例化 Memory 模块
    Memory memory("Memory");

    // 信号连接
    memory.clk(clk_signal);
    memory.write(w_signal);
    memory.read(r_signal);
    memory.w_data(wdata);
    memory.r_data(rdata);
    memory.address(addr);
    memory.ready(ready_signal);

    // 测试用例 1：写入数据
    std::cout << "[TEST 1] Writing data 0x12345678 to address 0x00000000" << std::endl;
    wdata.write(0x12345678);
    addr.write(0x00000000);
    w_signal.write(true);
    r_signal.write(false);
    sc_start(10, SC_NS); // 模拟 10ns

    // 测试用例 2：读取数据
    std::cout << "[TEST 2] Reading data from address 0x00000000" << std::endl;
    w_signal.write(false);
    r_signal.write(true);
    sc_start(10, SC_NS); // 模拟 10ns

    std::cout << "Read data: " << std::hex << rdata.read() << std::endl;

    // 测试用例 3：写入另一组数据
    std::cout << "[TEST 3] Writing data 0x87654321 to address 0x00001000" << std::endl;
    wdata.write(0x87654321);
    addr.write(0x00001000);
    w_signal.write(true);
    r_signal.write(false);
    sc_start(10, SC_NS); // 模拟 10ns

    // 测试用例 4：读取写入的数据
    std::cout << "[TEST 4] Reading data from address 0x00001000" << std::endl;
    w_signal.write(false);
    r_signal.write(true);
    sc_start(10, SC_NS); // 模拟 10ns

    std::cout << "Read data: " << std::hex << rdata.read() << std::endl;

    // 结束仿真
    std::cout << "Simulation ends" << std::endl;

    return 0;
}