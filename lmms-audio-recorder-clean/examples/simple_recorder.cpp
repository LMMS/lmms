#include <iostream>
#include <csignal>
#include <thread>
#include <chrono>
#include "RecordingEngine.h"
#include "RecordingTrack.h"

using namespace lmms;

static bool g_running = true;

void signalHandler(int) {
    g_running = false;
}

void printUsage(const char* program) {
    std::cout << "Usage: " << program << " [options]\n"
              << "Options:\n"
              << "  -o <dir>     Output directory (default: current directory)\n"
              << "  -d <device>  Audio device ID (default: auto)\n"
              << "  -b <backend> Audio backend: alsa, pipewire, auto (default: auto)\n"
              << "  -c <channels> Number of channels: 1 or 2 (default: 2)\n"
              << "  -r <rate>    Sample rate: 44100, 48000, etc. (default: 44100)\n"
              << "  -h           Show this help\n"
              << "\nExample:\n"
              << "  " << program << " -o ~/recordings -b alsa -c 2 -r 48000\n"
              << "\nPress Ctrl+C to stop recording.\n";
}

int main(int argc, char* argv[]) {
    std::string outputDir = ".";
    std::string deviceId;
    std::string backend = "auto";
    int channels = 2;
    int sampleRate = 44100;
    
    // Parse arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-o" && i + 1 < argc) {
            outputDir = argv[++i];
        } else if (arg == "-d" && i + 1 < argc) {
            deviceId = argv[++i];
        } else if (arg == "-b" && i + 1 < argc) {
            backend = argv[++i];
        } else if (arg == "-c" && i + 1 < argc) {
            channels = std::stoi(argv[++i]);
        } else if (arg == "-r" && i + 1 < argc) {
            sampleRate = std::stoi(argv[++i]);
        } else if (arg == "-h") {
            printUsage(argv[0]);
            return 0;
        }
    }
    
    // Set up signal handler
    std::signal(SIGINT, signalHandler);
    
    std::cout << "=== LMMS Audio Recorder ===" << std::endl;
    std::cout << "Backend: " << backend << std::endl;
    std::cout << "Channels: " << channels << std::endl;
    std::cout << "Sample Rate: " << sampleRate << " Hz" << std::endl;
    std::cout << "Output Directory: " << outputDir << std::endl;
    std::cout << std::endl;
    
    // Initialize recording engine
    RecordingEngine engine;
    engine.setSampleRate(sampleRate);
    engine.setChannels(channels);
    engine.setOutputDirectory(outputDir);
    
    if (!engine.initialize(backend)) {
        std::cerr << "Failed to initialize audio backend: " << backend << std::endl;
        std::cerr << "Make sure you have the required libraries installed:" << std::endl;
        std::cerr << "  - ALSA: libasound2-dev" << std::endl;
        std::cerr << "  - PipeWire: libpipewire-0.3-dev" << std::endl;
        return 1;
    }
    
    // List available devices
    auto devices = engine.getAvailableDevices();
    std::cout << "Available audio devices:" << std::endl;
    for (const auto& dev : devices) {
        std::cout << "  - " << dev.name << " (ID: " << dev.id << ")" << std::endl;
    }
    std::cout << std::endl;
    
    // Select device
    if (!deviceId.empty()) {
        if (!engine.setDevice(deviceId)) {
            std::cerr << "Failed to open device: " << deviceId << std::endl;
            return 1;
        }
    }
    
    // Create recording track
    auto* track = engine.createTrack("recording");
    
    // Arm recording
    if (!engine.armRecording()) {
        std::cerr << "Failed to arm recording" << std::endl;
        return 1;
    }
    
    // Start recording
    if (!engine.startRecording()) {
        std::cerr << "Failed to start recording" << std::endl;
        return 1;
    }
    
    std::cout << "Recording started! Press Ctrl+C to stop..." << std::endl;
    std::cout << std::endl;
    
    // Monitor recording
    auto startTime = std::chrono::steady_clock::now();
    while (g_running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        auto elapsed = std::chrono::steady_clock::now() - startTime;
        auto seconds = std::chrono::duration_cast<std::chrono::seconds>(elapsed).count();
        
        std::cout << "\rRecording: " << seconds << "s - " 
                  << track->getRecordedSeconds() << "s audio captured"
                  << "       " << std::flush;
    }
    
    std::cout << std::endl << std::endl;
    
    // Stop recording
    engine.stopRecording();
    
    std::cout << "Recording stopped." << std::endl;
    std::cout << "Saved to: " << track->getFilePath() << std::endl;
    std::cout << "Total duration: " << track->getRecordedSeconds() << " seconds" << std::endl;
    
    return 0;
}
