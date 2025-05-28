#include "ThreadManager.h"
#include <iostream>
#include <chrono>

ThreadManager::ThreadManager() : m_running(false), m_started(false) {
}

ThreadManager::~ThreadManager() {
    stop();
}

bool ThreadManager::initialize() {
    if (m_started.load()) {
        std::cout << "ThreadManager already initialized" << std::endl;
        return true;
    }
    
    std::cout << "ThreadManager initialized successfully" << std::endl;
    return true;
}

void ThreadManager::start() {
    if (m_started.load()) {
        std::cout << "ThreadManager already started" << std::endl;
        return;
    }
    
    if (!m_gameLogicFunction) {
        std::cerr << "ThreadManager: No game logic function set!" << std::endl;
        return;
    }
    
    m_running.store(true);
    
    // Start game logic thread
    m_gameLogicThread = std::thread(&ThreadManager::gameLogicLoop, this);
    
    m_started.store(true);
    std::cout << "ThreadManager started successfully" << std::endl;
}

void ThreadManager::stop() {
    if (!m_started.load()) {
        return;
    }
    
    std::cout << "Stopping ThreadManager..." << std::endl;
    
    // Signal thread to stop
    m_running.store(false);
    
    // Wait for thread to finish
    if (m_gameLogicThread.joinable()) {
        m_gameLogicThread.join();
    }
    
    m_started.store(false);
    std::cout << "ThreadManager stopped successfully" << std::endl;
}

void ThreadManager::setGameLogicFunction(std::function<void(double)> updateFunc) {
    m_gameLogicFunction = updateFunc;
}

void ThreadManager::gameLogicLoop() {
    std::cout << "Game logic thread started" << std::endl;
    
    auto lastTime = std::chrono::high_resolution_clock::now();
    double accumulatedTime = 0.0;
    
    while (m_running.load()) {
        auto currentTime = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration<double>(currentTime - lastTime).count();
        lastTime = currentTime;
        
        // Accumulate time for fixed timestep
        accumulatedTime += elapsed;
        
        // Run game logic at fixed timestep
        while (accumulatedTime >= GAME_LOGIC_TIMESTEP) {
            if (m_gameLogicFunction) {
                m_gameLogicFunction(GAME_LOGIC_TIMESTEP);
            }
            accumulatedTime -= GAME_LOGIC_TIMESTEP;
        }
        
        // Sleep for a short time to prevent 100% CPU usage
        std::this_thread::sleep_for(std::chrono::microseconds(SLEEP_MICROSECONDS));
    }
    
    std::cout << "Game logic thread ended" << std::endl;
}
