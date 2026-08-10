#ifndef _TASK_MGR_H_
#define _TASK_MGR_H_

#include <QThread>

struct OpenLoopInfo {
    int control_side;       // 控制编号：0 | 1 | 2 | 3
    int duty_cycle_1;    // 1侧占空比
    int duty_cycle_2;    // 2侧占空比
    int loop_count;         // 循环次数
    int mid_stay_time;      // 中位停留时间
    int work_stay_time;     // 工作位停留时间
};


class TaskMgr {
public:
    static TaskMgr* GetInstance() {
        static TaskMgr *instance = nullptr;
        if(instance == nullptr) {
            instance = new TaskMgr();
        }
        return instance;
    }

    bool IsOpenLoopCycleRunning() const;
    // 开环循环动作
    void StartOpenLoopCycle(const OpenLoopInfo &loop_info);
    void StopOpenLoopCycle();

private:
    TaskMgr() = default;
    ~TaskMgr() = default;
    TaskMgr(const TaskMgr&) = delete;
    TaskMgr& operator=(const TaskMgr&) = delete;

    void ExecuteOpenLoopCycle(const OpenLoopInfo &loop_info);
    bool StopElectromagnet();
    bool StartElectromagnet();
    bool SendWorkPositionCommand();
    bool SendReturnToMidCommand();

    std::atomic<bool> stopRequested_{false};
    std::atomic<bool> isRunning_{false};
};



#endif //_TASK_MGR_H_