#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include <array>

#include "StreamDockCPPSDK/StreamDockSDK/HSDAction.h"
#include "StreamDockCPPSDK/StreamDockSDK/NlohmannJSONUtils.h"
#include "SimData/SimData.hpp"
#include "SimData/SimVar.hpp"
#include "SimManager/SimManager.hpp"
#include "ui/UIManager.hpp"
#include "Utils.hpp"

namespace BaseActionEvents
{
    using Actions = std::array<uint32_t, 2>;

    inline constexpr Actions GENERIC = {0, 0};
    inline constexpr Actions PMDG_CLICK = {MOUSE_FLAG_LEFTSINGLE, MOUSE_FLAG_LEFTRELEASE};
    inline constexpr Actions PMDG_SCROLL_UP = {MOUSE_FLAG_WHEEL_UP, 0};
    inline constexpr Actions PMDG_SCROLL_DOWN = {MOUSE_FLAG_WHEEL_DOWN, 0};
}

// Variable procession structure
struct VarBinding {
    SimVarDefinition* def;                  // Pointer to variable def
    std::string  next;                      // New variable name
    DEFINITIONS  group;                     // Variable group
    SubscriptionId* subId;                  // subscription ID pointer
};

// Event procession structure
struct EventBinding {
    SimEventDefinition* def;                // Pointer to event def
    std::string  next;                      // New event name
    EVENT_TYPES type;                       // Event type
    std::array<uint32_t, 2> actions{};      // Event actions
};

class BaseAction : public HSDAction, public IUIUpdatable {
public:
    BaseAction(HSDConnectionManager* hsd_connection,
               const std::string& action,
               const std::string& context);
    virtual ~BaseAction();

protected:
    std::vector<VarBinding> varBindings_;
    std::vector<EventBinding> eventBindings_;

    virtual void OnVariableUpdated(const std::string& name, double value);

    // Public interface for derived classes
    void ApplyBindings();
    void UnregisterAll();
    void CleanUp();
    nlohmann::json BuildCommonPayloadJson() const;
    virtual void SendToPI(const nlohmann::json& payload) override;
    bool UpdatePmdgTypeFromSettings(const nlohmann::json& settings);

    // Coalesced refresh mechanism: variable-update callers should request a redraw
    // here instead of calling UpdateImage() directly. Requests are throttled to at
    // most one redraw per Config::MinUpdateIntervalMs() (default ~200ms/5Hz), and
    // the latest request wins (intermediate ones are dropped). The draw is also
    // skipped entirely when the visible content has not changed since the last
    // redraw (see DisplayKey()).
    void RequestImageUpdate();

    // Derived classes may override this to produce a compact signature of the
    // currently *visible* content. The throttle loop refuses to redraw when the
    // returned signature matches the previous one. Returning an empty string
    // disables content-skip (always redraw on a throttled tick).
    virtual std::string DisplayKey() const { return {}; }

    // PMDG
    bool isPmdg = false;
    PMDGAircraft pmdgPlaneType = PMDG_NONE;

    // Stop the throttle helper, joining its worker thread. Derived destructors
    // must call this so the worker never touches a partially-destroyed object.
    void StopRefreshHelper();

private:
    // Determine if action is for PMDG by action name
    static bool IsPmdgAction(const std::string& action) {
        return action.find(".pmdg.") != std::string::npos;
    }

    std::function<void(const std::string&, double)> varCallback_;

    // Throttle helper state
    void RefreshWorker();

    std::mutex refreshMutex_;
    std::condition_variable refreshCv_;
    std::thread refreshThread_;
    std::atomic<bool> refreshThreadRunning_{false};
    std::atomic<bool> refreshPending_{false};
    std::chrono::steady_clock::time_point lastRefreshTime_{};
    std::string lastDisplayKey_;
    bool hasLastDisplayKey_ = false;

    // Helper methods
    void FillEvent(SimEventDefinition& e, const std::string& name,
                          EVENT_TYPES type, const std::array<uint32_t, 2>& actions);

    void DiffVar(VarBinding& v, std::vector<SimVarDefinition>& toAdd, std::vector<SimVarDefinition>& toRemove);
    void SubscribeVar(VarBinding& v, const std::function<void(const std::string&, double)>& callback);
    void DiffEvent(EventBinding& e, std::vector<SimEventDefinition>& toAdd, std::vector<SimEventDefinition>& toRemove);
    void ApplyChanges(std::vector<SimVarDefinition>& varsToAdd,
                      std::vector<SimVarDefinition>& varsToRemove,
                      std::vector<SimEventDefinition>& eventsToAdd,
                      std::vector<SimEventDefinition>& eventsToRemove);
};
