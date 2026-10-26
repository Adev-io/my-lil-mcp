#pragma once

#include "plugin.hpp"
#include "Meter.h"

#include <httplib.h>

#include <atomic>
#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

struct UITaskQueue {
    struct Task {
        std::function<void()>               fn;
        std::shared_ptr<std::promise<void>> promise;
        std::shared_ptr<std::atomic<int>>   state;   // 0 queued, 1 running, 2 cancelled (timed out before it ran)
        std::string                         label;
    };

    std::mutex       mutex;
    std::queue<Task> tasks;

    std::future<void> post(std::function<void()> fn, const std::string& label, std::shared_ptr<std::atomic<int>> state);
    // Returns false if the UI thread did not execute the task within timeoutSecs.
    bool postSync(std::function<void()> fn, const std::string& label = "", int timeoutSecs = 10);
    void drain();
};

struct RackMcpServer : Module {
    enum ParamIds { PORT_PARAM, ENABLED_PARAM, NUM_PARAMS };
    enum InputIds { LISTEN_L_INPUT, LISTEN_R_INPUT, NUM_INPUTS };
    enum OutputIds { HEARTBEAT_OUTPUT, NUM_OUTPUTS };
    enum LightIds { RUNNING_LIGHT, ACTIVITY_LIGHT, NUM_LIGHTS };

    UITaskQueue taskQueue;
    class RackHttpServer* server = nullptr;
    bool wasEnabled = false;
    int serverPort = 0;   // the port startServer() last used; process() restarts when PORT_PARAM differs
    float heartbeatPhase = 0.f;

    // Activity indicator — written by HTTP thread, read by audio thread
    std::atomic<bool> activityFlag{false};
    float activityTimer = 0.f;

    // LISTEN meter. Audio thread owns `meter`; HTTP asks for a reset through meterReset. HTTP bumps snapGen after the
    // window; the audio thread snapshots whenever snapGen != snapDone and then publishes snapDone = snapGen; HTTP reads
    // `snap` only once snapDone equals the generation it asked for.
    Meter meter;
    std::atomic<int> meterReset{0};
    std::atomic<int> snapGen{0};
    std::atomic<int> snapDone{0};
    Meter::Report snap;
    std::mutex measureMutex;

    void onSampleRateChange(const SampleRateChangeEvent& e) override { meter.setSampleRate(e.sampleRate); }

    std::mutex pendingDeleteMutex;
    std::vector<uint64_t> pendingDeleteIds;

    // User-configurable layout bounds, anchored from the MCP module position.
    // cols_hp/rows set to 0 mean "unbounded".
    std::mutex layoutPrefsMutex;
    int layoutMatrixColsHp = 8;
    int layoutMatrixRows = 4;

    RackMcpServer();
    ~RackMcpServer() override;

    void startServer(int port);
    void stopServer();
    void process(const ProcessArgs& args) override;
    json_t* dataToJson() override;
    void dataFromJson(json_t* rootJ) override;
};

class RackHttpServer {
public:
    httplib::Server svr;
    std::thread serverThread;
    std::atomic<bool> running{false};
    int port = 2600;
    UITaskQueue* taskQueue = nullptr;
    RackMcpServer* parent = nullptr;
    rack::Context* rackApp = nullptr;

    RackHttpServer() = default;
    ~RackHttpServer();

    std::string getLayoutPrefsJson();
    std::string dispatchTool(const std::string& name, const std::string& args);
    void handleMcpPost(const httplib::Request& req, httplib::Response& res);
    void setupRoutes();
    void start();
    void stop();
};

struct RackMcpServerWidget : ModuleWidget {
    explicit RackMcpServerWidget(RackMcpServer* module);
    void step() override;
    void appendContextMenu(Menu* menu) override;
};
