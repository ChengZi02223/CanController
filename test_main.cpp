// #include "CanDriver.h"
// #include "CO_ObjectDictionary.h"
// #include "CO_SDO.h"
// #include "CO_NMT.h"
// #include "CO_Heartbeat.h"
// #include "CO_PDO.h"
// #include "CO_SYNC.h"
// #include "CO_EMCY.h"
// #include <iostream>
// #include <atomic>
// #include <signal.h>
// #include <thread>
// #include <chrono>

// static std::atomic<bool> running(true);
// void signalHandler(int) { running = false; }

// int main(int argc, char* argv[]) {
//     if (argc != 3) {
//         std::cerr << "Usage: " << argv[0] << " <device> <node_id>\n";
//         std::cerr << "Example: canopen_slave PCAN_USBBUS1 10\n";
//         return 1;
//     }
//     std::string device = argv[1];
//     uint8_t nodeId = static_cast<uint8_t>(std::stoi(argv[2]));
    
//     // 初始化 CAN 驱动
//     CanDriver can;
//     if (!can.init(device, 250000)) {
//         std::cerr << "Failed to init CAN on " << device << std::endl;
//         return 1;
//     }
//     std::cout << "CAN driver initialized on " << device << std::endl;
    
//     // 构建对象字典
//     CO_ObjectDictionary od;
//     od.addEntry(0x1000, 0, ODDataType::UINT32, 4, 1, {0,0,0,0});   // Device type
//     od.addEntry(0x1001, 0, ODDataType::UINT8, 1, 1, {0});          // Error register
//     od.addEntry(0x1018, 1, ODDataType::UINT32, 4, 1, {1,2,3,4});   // Vendor ID
//     od.addEntry(0x2000, 0, ODDataType::UINT16, 2, 3, {0x12,0x34}); // Application var
//     od.addEntry(0x2001, 0, ODDataType::UINT32, 4, 3, {0,0,0,0});
//     od.addEntry(0x2002, 0, ODDataType::UINT16, 2, 3, {0,0});
//     od.dump();
    
//     // 各服务模块
//     CO_NMT nmt(can, nodeId);
//     nmt.setInitialState(NmtState::PRE_OPERATIONAL);  // 启动后立即进入预操作
//     CO_SDO sdo(od, can, nodeId);
//     CO_Heartbeat heartbeat(can, nodeId, nmt);
//     CO_PDO pdo(od, can, nmt, nodeId);
//     CO_SYNC sync(can, nmt, pdo);
//     CO_EMCY emcy(od, can, nodeId);
    
//     // 发送 boot-up 消息
//     can_frame boot;
//     boot.can_id = 0x700 + nodeId;
//     boot.can_dlc = 1;
//     boot.data[0] = 0;
//     can.send(boot);
    
//     // 启动心跳线程
//     heartbeat.start();
    
//     // 设置信号处理
//     signal(SIGINT, signalHandler);
//     signal(SIGTERM, signalHandler);
    
//     std::cout << "CANopen slave node " << (int)nodeId << " is running...\n";
    
//     while (running) {
//         can_frame frame;
//         if (can.receive(frame, 50)) {
//             uint32_t cob = frame.can_id & 0x7FF;
//             if (cob == 0x000) nmt.processCommand(frame);
//             else if (cob == (0x600 + nodeId)) sdo.processRequest(frame);
//             else if (cob == 0x80) sync.processSync(frame);
//             else if ((cob & 0x7F) == nodeId) {
//                 // 检查是否是 RPDO COB-ID: 0x200+nodeId 等，交给PDO处理
//                 pdo.processRPDO(frame);
//             }
//         }
//         // 定期检查事件触发 TPDO
//         pdo.processEventDriven();
//         // 定期检查错误寄存器变化并发送 EMCY
//         emcy.checkAndSend();
//         // 模拟：每秒随机产生一个错误演示（实际应由应用程序触发）
//         static int cnt=0;
//         if (++cnt % 100 == 0) {
//             // 演示：设置一次通用错误，然后清除
//             static bool errSet = false;
//             if (!errSet) {
//                 emcy.setError(0xFF01, 0x10);
//                 errSet = true;
//             } else {
//                 emcy.clearError();
//                 errSet = false;
//             }
//         }
//     }
    
//     heartbeat.stop();
//     can.close();
//     std::cout << "Exited." << std::endl;
//     return 0;
// }