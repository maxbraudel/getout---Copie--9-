#ifndef THREAD_MANAGER_H
#define THREAD_MANAGER_H

#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <functional>
#include "enumDefinitions.h"

/**
 * Thread Manager - handles game logic threading with clean interface
 * Separates threading concerns from game logic
 */
class ThreadManager {
public:
    ThreadManager();
    ~ThreadManager();
    
    bool initialize();
    void start();
    void stop();
    
    // Set the game logic update function
    void setGameLogicFunction(std::function<void(double)> updateFunc);
    
    bool isRunning() const { return m_running.load(); }
    bool isStarted() const { return m_started.load(); }
    
private:
    std::atomic<bool> m_running;
    std::atomic<bool> m_started;
    
    std::thread m_gameLogicThread;
    std::function<void(double)> m_gameLogicFunction;
    
    void gameLogicLoop();
    
    static constexpr double GAME_LOGIC_TIMESTEP = 1.0 / 60.0; // 60 FPS
    static constexpr int SLEEP_MICROSECONDS = 500; // Prevent 100% CPU usage
};

#endif // THREAD_MANAGER_H
