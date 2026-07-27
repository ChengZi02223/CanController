
// #include "CanDriver.h"
// #include <iostream>
// #include <thread>
// #include <chrono>
// #include <iomanip>
// #include <windows.h>

// int main()
// {
//     SetConsoleOutputCP(CP_UTF8);
//     // 1. 创建 CAN 驱动实例
//     CanDriver can;

//     // 2. 初始化设备（使用 PCAN_USBBUS1，波特率 250kbps）
//     std::string device = "PCAN_USBBUS1";
//     unsigned int baudrate = 500000; // 500 kbps

//     if (!can.init(device, baudrate)) {
//         std::cerr << "CAN 初始化失败，程序退出。" << std::endl;
//         return 1;
//     }
//     std::cout << "CAN 设备 " << device << " 初始化成功，波特率 " << baudrate << " bps" << std::endl;


//     // can_frame testFrame;
//     // testFrame.can_id = 0x123;
//     // testFrame.can_dlc = 8;
//     // for (int i = 0; i < 8; ++i) testFrame.data[i] = i + 1;

//     // if (can.send(testFrame)) {
//     //     std::cout << "已发送测试帧 ID=0x123, 数据: ";
//     //     for (int i = 0; i < 8; ++i)
//     //         std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)testFrame.data[i] << " ";
//     //     std::cout << std::dec << std::endl;
//     // } else {
//     //     std::cerr << "发送测试帧失败" << std::endl;
//     // }

//     // 3. 构建并发送 CANopen NMT 启动节点命令
//     //    NMT 报文：COB-ID = 0x000，数据[0] = 命令码(0x01=启动)，数据[1] = 节点 ID(64)
//     can_frame nmtFrame;
//     nmtFrame.can_id = 0x000;           // NMT COB-ID
//     nmtFrame.can_dlc = 2;
//     nmtFrame.data[0] = 0x01;           // 启动节点命令
//     nmtFrame.data[1] = 64;             // 目标节点 ID

//     if (can.send(nmtFrame)) {
//         std::cout << "已发送 NMT 启动节点命令 (节点 64)" << std::endl;
//     } else {
//         std::cerr << "发送 NMT 命令失败" << std::endl;
//     }

//     // 4. 接收循环：尝试接收 10 个 CAN 帧，超时 100ms
//     std::cout << "开始接收 CAN 帧..." << std::endl;
//     int frameCount = 0;
//     const int maxFrames = 10;
//     const int timeoutMs = 100;


//     while (1) {
//         can_frame rxFrame;
//         if (can.receive(rxFrame, timeoutMs)) {
//             // 打印接收到的帧信息
//             std::cout << "收到帧 " << frameCount+1 << ": ID=0x" << std::hex << rxFrame.can_id 
//                       << " DLC=" << std::dec << (int)rxFrame.can_dlc << " Data=";
//             for (int i = 0; i < rxFrame.can_dlc; ++i) {
//                 std::cout << std::hex << std::setw(2) << std::setfill('0') 
//                           << (int)rxFrame.data[i] << " ";
//             }
//             std::cout << std::dec << std::endl;
//             frameCount++;
//         } else {
//             // 超时未收到数据，继续等待
//             std::cout << "." << std::flush;
//         }
//     }

//     // 5. 关闭设备
//     can.close();
//     std::cout << "\nCAN 设备已关闭，程序退出。" << std::endl;
//     return 0;
// }