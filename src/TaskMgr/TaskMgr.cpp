#include "TaskMgr.h"
#include "CanDriver.h"
#include "can_cmd.h"
#include <iostream>
#include <thread>
#include <chrono>

void TaskMgr::StartOpenLoopCycle(const OpenLoopInfo &loop_info)
{
    if(loop_info.control_side == 0) {
        std::cerr << "TaskMgr::StartOpenLoopCycle no control start" << std::endl;
        return;        
    }
    
    if (loop_info.loop_count <= 0 || loop_info.mid_stay_time < 0 || loop_info.work_stay_time < 0) {
        std::cerr << "TaskMgr::StartOpenLoopCycle invalid parameters" << std::endl;
        return;
    }

    if (isRunning_.load()) {
        std::cerr << "TaskMgr::StartOpenLoopCycle already running" << std::endl;
        return;
    }

    stopRequested_.store(false);
    isRunning_.store(true);

    std::thread([this, loop_info]() {
        ExecuteOpenLoopCycle(loop_info);
        isRunning_.store(false);
    }).detach();
}

void TaskMgr::StopOpenLoopCycle()
{
    stopRequested_.store(true);
}

bool TaskMgr::IsOpenLoopCycleRunning() const
{
    return isRunning_.load();
}

void TaskMgr::ExecuteOpenLoopCycle(const OpenLoopInfo &loop_info)
{
    for (int loop = 0; loop < loop_info.loop_count && !stopRequested_.load(); ++loop) {
        std::cout << "TaskMgr: loop " << loop + 1 << " / " << loop_info.loop_count << std::endl;

        // if (stopRequested_.load()) {
        //     break;
        // }

        // if (!StopElectromagnet()) {
        //     std::cerr << "TaskMgr: StopElectromagnet failed" << std::endl;
        // }

        // if (loop_info.mid_stay_time > 0) {
        //     std::this_thread::sleep_for(std::chrono::milliseconds(loop_info.mid_stay_time));
        // }

        // if (stopRequested_.load()) {
        //     break;
        // }

        // if (!StartElectromagnet()) {
        //     std::cerr << "TaskMgr: StartElectromagnet failed" << std::endl;
        // }

        // if (SendWorkPositionCommand()) {
        //     if (loop_info.work_stay_time > 0) {
        //         std::this_thread::sleep_for(std::chrono::milliseconds(loop_info.work_stay_time));
        //     }
        // } else {
        //     std::cerr << "TaskMgr: SendWorkPositionCommand failed" << std::endl;
        // }

        // if (stopRequested_.load()) {
        //     break;
        // }

        // if (!SendReturnToMidCommand()) {
        //     std::cerr << "TaskMgr: SendReturnToMidCommand failed" << std::endl;
        // }
    }

    std::cout << "TaskMgr: open-loop cycle finished" << std::endl;
}

bool TaskMgr::StopElectromagnet()
{
    // TODO: fill in stop command details later.
    std::cout << "TaskMgr: StopElectromagnet placeholder" << std::endl;
    return CanDriver::GetInstance()->ExecCmd(NMT_COB_ID, NMT_CLOSE_READ_CMD, 200);
}

bool TaskMgr::StartElectromagnet()
{
    // TODO: fill in start command details later.
    std::cout << "TaskMgr: StartElectromagnet placeholder" << std::endl;
    return CanDriver::GetInstance()->ExecCmd(NMT_COB_ID, NMT_READ_VALUE_CMD, 200);
}

bool TaskMgr::SendWorkPositionCommand()
{
    // TODO: use specific work-position CAN command when available.
    std::cout << "TaskMgr: SendWorkPositionCommand placeholder" << std::endl;
    return true;
}

bool TaskMgr::SendReturnToMidCommand()
{
    // TODO: use specific return-to-mid CAN command when available.
    std::cout << "TaskMgr: SendReturnToMidCommand placeholder" << std::endl;
    return true;
}


