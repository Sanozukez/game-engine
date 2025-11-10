#pragma once

#include <chrono>
#include <string>
#include <format>
#include "log.h"

namespace Engine::Core {

/**
 * @brief Simple profiling timer for measuring code execution time
 * 
 * Usage:
 *   {
 *       ScopedTimer timer("MyFunction");
 *       // Code to profile
 *   } // Automatically logs time when scope ends
 */
class ScopedTimer {
public:
    explicit ScopedTimer(const std::string& name)
        : m_name(name)
        , m_start(std::chrono::high_resolution_clock::now())
    {
    }

    ~ScopedTimer() {
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - m_start);
        
        // Log time in appropriate unit
        if (duration.count() < 1000) {
            // Less than 1ms → show in microseconds
            Log::Trace(std::format("[PROFILE] {} took {}μs", m_name, duration.count()));
        } else if (duration.count() < 1000000) {
            // Less than 1s → show in milliseconds
            float ms = duration.count() / 1000.0f;
            Log::Trace(std::format("[PROFILE] {} took {:.2f}ms", m_name, ms));
        } else {
            // 1s or more → show in seconds
            float sec = duration.count() / 1000000.0f;
            Log::Trace(std::format("[PROFILE] {} took {:.2f}s", m_name, sec));
        }
    }

    // Prevent copying
    ScopedTimer(const ScopedTimer&) = delete;
    ScopedTimer& operator=(const ScopedTimer&) = delete;

private:
    std::string m_name;
    std::chrono::time_point<std::chrono::high_resolution_clock> m_start;
};

/**
 * @brief Manual timer for more control over start/stop
 * 
 * Usage:
 *   ManualTimer timer;
 *   timer.start();
 *   // Code to profile
 *   timer.stop();
 *   float ms = timer.elapsedMilliseconds();
 */
class ManualTimer {
public:
    ManualTimer() 
        : m_running(false)
        , m_elapsed(0)
    {
    }

    void start() {
        m_start = std::chrono::high_resolution_clock::now();
        m_running = true;
    }

    void stop() {
        if (m_running) {
            auto end = std::chrono::high_resolution_clock::now();
            m_elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - m_start).count();
            m_running = false;
        }
    }

    void reset() {
        m_elapsed = 0;
        m_running = false;
    }

    // Get elapsed time in different units
    int64_t elapsedMicroseconds() const { return m_elapsed; }
    float elapsedMilliseconds() const { return m_elapsed / 1000.0f; }
    float elapsedSeconds() const { return m_elapsed / 1000000.0f; }

    bool isRunning() const { return m_running; }

private:
    std::chrono::time_point<std::chrono::high_resolution_clock> m_start;
    bool m_running;
    int64_t m_elapsed; // microseconds
};

/**
 * @brief Frame timing tracker for FPS and frame time monitoring
 * 
 * Usage:
 *   FrameTimer frameTimer;
 *   while (running) {
 *       frameTimer.beginFrame();
 *       // Game loop
 *       frameTimer.endFrame();
 *       
 *       if (frameTimer.shouldLogStats()) {
 *           frameTimer.logStats();
 *       }
 *   }
 */
class FrameTimer {
public:
    FrameTimer(float logInterval = 1.0f) 
        : m_logInterval(logInterval)
        , m_frameCount(0)
        , m_accumulatedTime(0.0f)
        , m_minFrameTime(999999.0f)
        , m_maxFrameTime(0.0f)
        , m_lastFrameTime(0.0f)
    {
    }

    void beginFrame() {
        m_frameStart = std::chrono::high_resolution_clock::now();
    }

    void endFrame() {
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - m_frameStart);
        
        m_lastFrameTime = duration.count() / 1000.0f; // Convert to ms
        
        m_accumulatedTime += m_lastFrameTime;
        m_frameCount++;
        
        if (m_lastFrameTime < m_minFrameTime) m_minFrameTime = m_lastFrameTime;
        if (m_lastFrameTime > m_maxFrameTime) m_maxFrameTime = m_lastFrameTime;
    }

    bool shouldLogStats() const {
        return m_accumulatedTime >= (m_logInterval * 1000.0f);
    }

    void logStats() {
        if (m_frameCount == 0) return;

        float avgFrameTime = m_accumulatedTime / m_frameCount;
        float avgFPS = 1000.0f / avgFrameTime;

        Log::Info(std::format(
            "[FRAME] FPS: {:.1f} | Avg: {:.2f}ms | Min: {:.2f}ms | Max: {:.2f}ms | Frames: {}",
            avgFPS, avgFrameTime, m_minFrameTime, m_maxFrameTime, m_frameCount
        ));

        // Reset stats
        m_frameCount = 0;
        m_accumulatedTime = 0.0f;
        m_minFrameTime = 999999.0f;
        m_maxFrameTime = 0.0f;
    }

    // Getters for current frame stats
    float getLastFrameTime() const { return m_lastFrameTime; }
    float getCurrentFPS() const { 
        return m_lastFrameTime > 0.0f ? 1000.0f / m_lastFrameTime : 0.0f; 
    }

private:
    std::chrono::time_point<std::chrono::high_resolution_clock> m_frameStart;
    float m_logInterval; // seconds
    uint32_t m_frameCount;
    float m_accumulatedTime; // milliseconds
    float m_minFrameTime;    // milliseconds
    float m_maxFrameTime;    // milliseconds
    float m_lastFrameTime;   // milliseconds
};

} // namespace Engine::Core

// Convenience macros for quick profiling
#ifdef ENABLE_PROFILING
    #define PROFILE_SCOPE(name) Engine::Core::ScopedTimer _timer##__LINE__(name)
    #define PROFILE_FUNCTION() PROFILE_SCOPE(__FUNCTION__)
#else
    #define PROFILE_SCOPE(name)
    #define PROFILE_FUNCTION()
#endif
