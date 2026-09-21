#include "ManagerPerformanceTest.h"
#include <cassert>
#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <future>

ManagerPerformanceTest::ManagerPerformanceTest() = default;
ManagerPerformanceTest::~ManagerPerformanceTest() = default;

bool ManagerPerformanceTest::innerTest() {
    try {
        logger->EnterLog("ManagerPerformanceTest: Starting performance tests");
        Manager manager;
        
        // Run performance test suites
        if (!testSinkCreationPerformance(manager)) return false;
        if (!testSourceCreationPerformance(manager)) return false;
        if (!testBindingPerformance(manager)) return false;
        if (!testStatusQueryPerformance(manager)) return false;
        if (!testSystemMonitoringPerformance(manager)) return false;
        if (!testConcurrentOperations(manager)) return false;
        
        logger->EnterLog("ManagerPerformanceTest: All performance tests completed successfully");
        return true;
        
    } catch (const std::exception& e) {
        logger->EnterLog("ManagerPerformanceTest FAILED with exception: " + std::string(e.what()));
        return false;
    } catch (...) {
        logger->EnterLog("ManagerPerformanceTest FAILED with unknown exception");
        return false;
    }
}

template<typename Func>
double ManagerPerformanceTest::measureExecutionTime(Func func, int iterations) {
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < iterations; ++i) {
        func();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    return duration.count() / 1000.0; // Convert to milliseconds
}

void ManagerPerformanceTest::logPerformanceResult(const std::string& testName, double timeMs, int operations) {
    double avgTimePerOp = timeMs / operations;
    double opsPerSecond = operations / (timeMs / 1000.0);
    
    logger->EnterLog("PERFORMANCE [" + testName + "]: " + 
                    std::to_string(operations) + " ops in " + 
                    std::to_string(timeMs) + "ms " +
                    "(avg: " + std::to_string(avgTimePerOp) + "ms/op, " +
                    std::to_string(opsPerSecond) + " ops/sec)");
}

bool ManagerPerformanceTest::testSinkCreationPerformance(Manager& manager) {
    logger->EnterLog("ManagerPerformanceTest: Testing sink creation performance");
    
    const int iterations = 100;
    std::vector<int> sinkIds;
    sinkIds.reserve(iterations);
    
    // Test AprilTag sink creation performance
    double apriltagTime = measureExecutionTime([&]() {
        int sinkId = manager.CreateApriltagSink();
        if (sinkId > 0) {
            sinkIds.push_back(sinkId);
        }
    }, iterations);
    
    logPerformanceResult("AprilTag Sink Creation", apriltagTime, iterations);
    
    // Verify all sinks were created
    auto allSinks = manager.GetAllSinks();
    if (allSinks.size() < sinkIds.size()) {
        logger->EnterLog("WARNING: Not all sinks were properly registered");
    }
    
    // Test sink creation with specific IDs
    double specificIdTime = measureExecutionTime([&]() {
        int specificId = 10000 + rand() % 1000;
        manager.CreateApriltagSink(specificId);
    }, 50);
    
    logPerformanceResult("AprilTag Sink Creation (Specific ID)", specificIdTime, 50);
    
    // Test ObjectDetection sink creation (if implemented)
    double objDetTime = measureExecutionTime([&]() {
        manager.CreateObjectDetectionSink();
    }, 50);
    
    logPerformanceResult("Object Detection Sink Creation", objDetTime, 50);
    
    // Cleanup performance test
    double cleanupTime = measureExecutionTime([&]() {
        for (int sinkId : sinkIds) {
            manager.StopSinkById(sinkId);
        }
    }, 1);
    
    logPerformanceResult("Sink Cleanup", cleanupTime, sinkIds.size());
    
    return true;
}

bool ManagerPerformanceTest::testSourceCreationPerformance(Manager& manager) {
    logger->EnterLog("ManagerPerformanceTest: Testing source creation performance");
    
    const int iterations = 50;
    std::vector<int> sourceIds;
    sourceIds.reserve(iterations);
    
    // Test image file source creation performance
    double imageTime = measureExecutionTime([&]() {
        int sourceId = manager.CreateImageFileSource("perf_test_" + std::to_string(rand()) + ".jpg");
        if (sourceId > 0) {
            sourceIds.push_back(sourceId);
        }
    }, iterations);
    
    logPerformanceResult("Image File Source Creation", imageTime, iterations);
    
    // Test video file source creation performance
    double videoTime = measureExecutionTime([&]() {
        int sourceId = manager.CreateVideoFileSource("perf_test_" + std::to_string(rand()) + ".mp4", 30);
        if (sourceId > 0) {
            sourceIds.push_back(sourceId);
        }
    }, iterations);
    
    logPerformanceResult("Video File Source Creation", videoTime, iterations);
    
    // Test camera source creation performance (if cameras available)
    auto cameras = manager.EnumerateAvailableCameras();
    if (!cameras.empty()) {
        double cameraTime = measureExecutionTime([&]() {
            manager.CreateCameraSource(cameras[0]);
        }, 10); // Fewer iterations for camera sources
        
        logPerformanceResult("Camera Source Creation", cameraTime, 10);
    }
    
    // Cleanup
    for (int sourceId : sourceIds) {
        manager.StopSourceById(sourceId);
    }
    
    return true;
}

bool ManagerPerformanceTest::testBindingPerformance(Manager& manager) {
    logger->EnterLog("ManagerPerformanceTest: Testing binding performance");
    
    const int pairs = 50;
    std::vector<int> sources, sinks;
    
    // Create sources and sinks for binding tests
    for (int i = 0; i < pairs; ++i) {
        int sourceId = manager.CreateImageFileSource("bind_test_" + std::to_string(i) + ".jpg");
        int sinkId = manager.CreateApriltagSink();
        
        if (sourceId > 0) sources.push_back(sourceId);
        if (sinkId > 0) sinks.push_back(sinkId);
    }
    
    // Test binding performance
    double bindTime = measureExecutionTime([&]() {
        for (size_t i = 0; i < std::min(sources.size(), sinks.size()); ++i) {
            manager.BindSourceToSink(sources[i], sinks[i]);
        }
    }, 1);
    
    logPerformanceResult("Source to Sink Binding", bindTime, std::min(sources.size(), sinks.size()));
    
    // Test unbinding performance
    double unbindTime = measureExecutionTime([&]() {
        for (int sinkId : sinks) {
            manager.UnbindSourceFromSink(sinkId);
        }
    }, 1);
    
    logPerformanceResult("Source from Sink Unbinding", unbindTime, sinks.size());
    
    // Test rebinding performance (common in dynamic scenarios)
    double rebindTime = measureExecutionTime([&]() {
        for (size_t i = 0; i < std::min(sources.size(), sinks.size()); ++i) {
            // Bind to a different source (circular shift)
            size_t nextSource = (i + 1) % sources.size();
            manager.BindSourceToSink(sources[nextSource], sinks[i]);
        }
    }, 1);
    
    logPerformanceResult("Source Rebinding", rebindTime, std::min(sources.size(), sinks.size()));
    
    return true;
}

bool ManagerPerformanceTest::testStatusQueryPerformance(Manager& manager) {
    logger->EnterLog("ManagerPerformanceTest: Testing status query performance");
    
    // Create some sinks to query
    std::vector<int> sinkIds;
    for (int i = 0; i < 20; ++i) {
        int sinkId = manager.CreateApriltagSink();
        if (sinkId > 0) {
            sinkIds.push_back(sinkId);
        }
    }
    
    const int queryIterations = 1000;
    
    // Test individual sink status queries
    double individualStatusTime = measureExecutionTime([&]() {
        for (int sinkId : sinkIds) {
            manager.GetSinkStatusById(sinkId);
        }
    }, queryIterations / sinkIds.size());
    
    logPerformanceResult("Individual Sink Status Queries", individualStatusTime, queryIterations);
    
    // Test individual sink result queries
    double individualResultTime = measureExecutionTime([&]() {
        for (int sinkId : sinkIds) {
            manager.GetSinkResult(sinkId);
        }
    }, queryIterations / sinkIds.size());
    
    logPerformanceResult("Individual Sink Result Queries", individualResultTime, queryIterations);
    
    // Test bulk status queries
    double bulkStatusTime = measureExecutionTime([&]() {
        manager.GetAllSinkStatus();
    }, 100);
    
    logPerformanceResult("Bulk Sink Status Queries", bulkStatusTime, 100);
    
    // Test bulk result queries
    double bulkResultTime = measureExecutionTime([&]() {
        manager.GetAllSinkResults();
    }, 100);
    
    logPerformanceResult("Bulk Sink Result Queries", bulkResultTime, 100);
    
    // Test GetAllSinks and GetAllSources performance
    double getAllSinksTime = measureExecutionTime([&]() {
        manager.GetAllSinks();
    }, 1000);
    
    logPerformanceResult("GetAllSinks", getAllSinksTime, 1000);
    
    double getAllSourcesTime = measureExecutionTime([&]() {
        manager.GetAllSources();
    }, 1000);
    
    logPerformanceResult("GetAllSources", getAllSourcesTime, 1000);
    
    return true;
}

bool ManagerPerformanceTest::testSystemMonitoringPerformance(Manager& manager) {
    logger->EnterLog("ManagerPerformanceTest: Testing system monitoring performance");
    
    const int monitoringIterations = 500;
    
    // Test memory usage monitoring performance
    double memoryTime = measureExecutionTime([&]() {
        manager.GetMemoryUsageBytes();
    }, monitoringIterations);
    
    logPerformanceResult("Memory Usage Monitoring", memoryTime, monitoringIterations);
    
    // Test CPU usage monitoring performance
    double cpuTime = measureExecutionTime([&]() {
        manager.GetCPUUsage();
    }, monitoringIterations);
    
    logPerformanceResult("CPU Usage Monitoring", cpuTime, monitoringIterations);
    
    // Test CPU temperature monitoring performance
    double tempTime = measureExecutionTime([&]() {
        manager.GetCpuTemperature();
    }, monitoringIterations);
    
    logPerformanceResult("CPU Temperature Monitoring", tempTime, monitoringIterations);
    
    // Test disk usage monitoring performance
    double diskTime = measureExecutionTime([&]() {
        manager.GetDiskUsage();
    }, monitoringIterations);
    
    logPerformanceResult("Disk Usage Monitoring", diskTime, monitoringIterations);
    
    // Test combined system monitoring (typical use case)
    double combinedTime = measureExecutionTime([&]() {
        manager.GetMemoryUsageBytes();
        manager.GetCPUUsage();
        manager.GetCpuTemperature();
        manager.GetDiskUsage();
    }, monitoringIterations / 4);
    
    logPerformanceResult("Combined System Monitoring", combinedTime, monitoringIterations);
    
    return true;
}

bool ManagerPerformanceTest::testConcurrentOperations(Manager& manager) {
    logger->EnterLog("ManagerPerformanceTest: Testing concurrent operations");
    
    const int numThreads = 4;
    const int operationsPerThread = 50;
    
    // Test concurrent sink creation
    auto concurrentSinkCreation = [&](int threadId) {
        std::vector<int> localSinks;
        for (int i = 0; i < operationsPerThread; ++i) {
            int sinkId = manager.CreateApriltagSink();
            if (sinkId > 0) {
                localSinks.push_back(sinkId);
            }
        }
        return localSinks.size();
    };
    
    auto start = std::chrono::high_resolution_clock::now();
    
    std::vector<std::future<int>> futures;
    for (int i = 0; i < numThreads; ++i) {
        futures.push_back(std::async(std::launch::async, concurrentSinkCreation, i));
    }
    
    int totalCreated = 0;
    for (auto& future : futures) {
        totalCreated += future.get();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    double concurrentTime = duration.count() / 1000.0;
    
    logPerformanceResult("Concurrent Sink Creation", concurrentTime, totalCreated);
    
    // Test concurrent status queries
    auto concurrentStatusQueries = [&](int threadId) {
        int queries = 0;
        auto sinks = manager.GetAllSinks();
        for (int i = 0; i < operationsPerThread && !sinks.empty(); ++i) {
            int sinkId = sinks[i % sinks.size()];
            manager.GetSinkStatusById(sinkId);
            queries++;
        }
        return queries;
    };
    
    start = std::chrono::high_resolution_clock::now();
    
    futures.clear();
    for (int i = 0; i < numThreads; ++i) {
        futures.push_back(std::async(std::launch::async, concurrentStatusQueries, i));
    }
    
    int totalQueries = 0;
    for (auto& future : futures) {
        totalQueries += future.get();
    }
    
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    concurrentTime = duration.count() / 1000.0;
    
    logPerformanceResult("Concurrent Status Queries", concurrentTime, totalQueries);
    
    // Test concurrent system monitoring
    auto concurrentSystemMonitoring = [&](int threadId) {
        int monitoringCalls = 0;
        for (int i = 0; i < operationsPerThread / 4; ++i) {
            manager.GetMemoryUsageBytes();
            manager.GetCPUUsage();
            manager.GetCpuTemperature();
            manager.GetDiskUsage();
            monitoringCalls += 4;
        }
        return monitoringCalls;
    };
    
    start = std::chrono::high_resolution_clock::now();
    
    futures.clear();
    for (int i = 0; i < numThreads; ++i) {
        futures.push_back(std::async(std::launch::async, concurrentSystemMonitoring, i));
    }
    
    int totalMonitoringCalls = 0;
    for (auto& future : futures) {
        totalMonitoringCalls += future.get();
    }
    
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    concurrentTime = duration.count() / 1000.0;
    
    logPerformanceResult("Concurrent System Monitoring", concurrentTime, totalMonitoringCalls);
    
    logger->EnterLog("ManagerPerformanceTest: Concurrent operations test completed");
    return true;
}