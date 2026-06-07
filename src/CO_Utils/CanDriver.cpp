#include "CanDriver.h"
#include <windows.h>
#include <iostream>
#include <cstring>

//========= 手动定义PCAN全部原生类型、结构体、常量（替代PCANBasic.h）=========
typedef unsigned short TPCANHandle;
typedef unsigned short TPCANBaudrate;
typedef unsigned char  TPCANType;
typedef unsigned int   TPCANStatus;

// CAN报文结构体 TPCANMsg
typedef struct _TPCANMsg
{
    DWORD ID;
    BYTE MSGTYPE;
    BYTE LEN;
    BYTE DATA[8];
} TPCANMsg;

// 时间戳结构体 TPCANTimestamp
typedef struct _TPCANTimestamp
{
    DWORD millis;
    WORD  millis_overflow;
    WORD  micros;
} TPCANTimestamp;

// PCAN常量定义
#define PCAN_ERROR_OK         ((TPCANStatus)0x00000000)
#define PCAN_ERROR_QRCVEMPTY  ((TPCANStatus)0x00000010)

// 通道设备
#define PCAN_USBBUS1 ((TPCANHandle)0x51)
#define PCAN_USBBUS2 ((TPCANHandle)0x52)
#define PCAN_PCIBUS1 ((TPCANHandle)0x41)

// 波特率编码
#define PCAN_BAUD_125K  ((TPCANBaudrate)0x0014)
#define PCAN_BAUD_250K  ((TPCANBaudrate)0x0016)
#define PCAN_BAUD_500K  ((TPCANBaudrate)0x0019)
#define PCAN_BAUD_1M    ((TPCANBaudrate)0x001C)

#define PCAN_MESSAGE_STANDARD ((BYTE)0x00)
//====================================================================

// 函数指针原型 __stdcall 调用约定，和DLL导出完全一致
typedef TPCANStatus(__stdcall *CAN_Initialize_t)(TPCANHandle, TPCANBaudrate, TPCANType, DWORD, WORD);
typedef TPCANStatus(__stdcall *CAN_Uninitialize_t)(TPCANHandle);
typedef TPCANStatus(__stdcall *CAN_Write_t)(TPCANHandle, TPCANMsg*);
typedef TPCANStatus(__stdcall *CAN_Read_t)(TPCANHandle, TPCANMsg*, TPCANTimestamp*);
typedef TPCANStatus(__stdcall *CAN_GetErrorText_t)(TPCANStatus, WORD, LPSTR);

// 全局函数指针
static CAN_Initialize_t        pCAN_Initialize = nullptr;
static CAN_Uninitialize_t      pCAN_Uninitialize = nullptr;
static CAN_Write_t             pCAN_Write = nullptr;
static CAN_Read_t              pCAN_Read = nullptr;
static CAN_GetErrorText_t      pCAN_GetErrorText = nullptr;
static HMODULE g_hPcanDll = nullptr;

// 加载DLL
static bool LoadPCANDll()
{
    if(g_hPcanDll) return true;
    g_hPcanDll = LoadLibraryA("PCANBasic.dll");
    if (!g_hPcanDll) {
        std::cerr << "找不到PCANBasic.dll" << std::endl;
        return false;
    }

    pCAN_Initialize   = (CAN_Initialize_t)GetProcAddress(g_hPcanDll, "CAN_Initialize");
    pCAN_Uninitialize = (CAN_Uninitialize_t)GetProcAddress(g_hPcanDll, "CAN_Uninitialize");
    pCAN_Write        = (CAN_Write_t)GetProcAddress(g_hPcanDll, "CAN_Write");
    pCAN_Read         = (CAN_Read_t)GetProcAddress(g_hPcanDll, "CAN_Read");
    pCAN_GetErrorText = (CAN_GetErrorText_t)GetProcAddress(g_hPcanDll, "CAN_GetErrorText");

    if (!pCAN_Initialize || !pCAN_Uninitialize || !pCAN_Write || !pCAN_Read) {
        std::cerr << "获取PCAN函数地址失败" << std::endl;
        FreeLibrary(g_hPcanDll);
        g_hPcanDll = nullptr;
        return false;
    }
    return true;
}

//================= 原有类实现，逻辑不变 =================
CanDriver::CanDriver()
    : handle_(nullptr), isInitialized_(false)
{
}

CanDriver::~CanDriver()
{
    close();
    if(g_hPcanDll)
    {
        FreeLibrary(g_hPcanDll);
        g_hPcanDll = nullptr;
    }
}

bool CanDriver::init(const std::string& device, unsigned int baudrate)
{
    if (!LoadPCANDll()) return false;

    TPCANHandle pcanHandle;
    if (device == "PCAN_USBBUS1") {
        pcanHandle = PCAN_USBBUS1;
    } else if (device == "PCAN_USBBUS2") {
        pcanHandle = PCAN_USBBUS2;
    } else if (device == "PCAN_PCIBUS1") {
        pcanHandle = PCAN_PCIBUS1;
    } else {
        std::cerr << "不支持设备:" << device << std::endl;
        return false;
    }

    TPCANBaudrate pcanBaud;
    switch (baudrate) {
        case 125000: pcanBaud = PCAN_BAUD_125K; break;
        case 250000: pcanBaud = PCAN_BAUD_250K; break;
        case 500000: pcanBaud = PCAN_BAUD_500K; break;
        case 1000000:pcanBaud = PCAN_BAUD_1M; break;
        default: pcanBaud = PCAN_BAUD_250K; break;
    }

    // 后面2个参数IOPort=0、Int=0，兼容默认参数
    TPCANStatus status = pCAN_Initialize(pcanHandle, pcanBaud, 0, 0, 0);
    if (status != PCAN_ERROR_OK) {
        std::cerr << "CAN初始化失败,err:" << status << std::endl;
        return false;
    }

    handle_ = reinterpret_cast<void*>(pcanHandle);
    isInitialized_ = true;
    return true;
}

void CanDriver::close()
{
    if (isInitialized_ && handle_ && pCAN_Uninitialize) {
        TPCANHandle pcanHandle = static_cast<TPCANHandle>(reinterpret_cast<uintptr_t>(handle_));
        pCAN_Uninitialize(pcanHandle);
        handle_ = nullptr;
        isInitialized_ = false;
    }
}

bool CanDriver::send(const can_frame& frame)
{
    if (!isInitialized_ || !handle_ || !pCAN_Write)
        return false;

    TPCANHandle pcanHandle = static_cast<TPCANHandle>(reinterpret_cast<uintptr_t>(handle_));
    TPCANMsg msg;
    msg.ID = frame.can_id & 0x7FF;
    msg.MSGTYPE = PCAN_MESSAGE_STANDARD;
    msg.LEN = frame.can_dlc;
    memcpy(msg.DATA, frame.data, 8);

    TPCANStatus status = pCAN_Write(pcanHandle, &msg);
    if (status != PCAN_ERROR_OK) {
        std::cerr << "发送失败:" << status << std::endl;
        return false;
    }
    return true;
}

bool CanDriver::receive(can_frame& frame, int timeout_ms)
{
    if (!isInitialized_ || !handle_ || !pCAN_Read)
        return false;

    TPCANHandle pcanHandle = static_cast<TPCANHandle>(reinterpret_cast<uintptr_t>(handle_));
    TPCANMsg msg;
    TPCANTimestamp ts;

    TPCANStatus status = pCAN_Read(pcanHandle, &msg, &ts);
    if (status == PCAN_ERROR_QRCVEMPTY) {
        if (timeout_ms > 0) Sleep(timeout_ms);
        return false;
    } else if (status != PCAN_ERROR_OK) {
        std::cerr << "接收异常:" << status << std::endl;
        return false;
    }

    frame.can_id = msg.ID;
    frame.can_dlc = msg.LEN;
    memcpy(frame.data, msg.DATA, 8);
    return true;
}