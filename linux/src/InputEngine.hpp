#pragma once
#include <unordered_set>
#include <vector>
#include <deque>
#include <mutex>
#include <thread>
#include <atomic>
#include <functional>
#include <algorithm>
#include <chrono>
#include <cstring>
#include <cctype>
#include <cmath>
#include <fstream>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <linux/input.h>
#include <X11/Xlib.h>
#include <X11/extensions/XInput2.h>
#ifdef None
#undef None
#endif
#include "KeyMap.hpp"

inline double nowSeconds() {
    static const auto start = std::chrono::steady_clock::now();
    auto cur = std::chrono::steady_clock::now();
    return std::chrono::duration<double>(cur - start).count();
}

/// Dual-layer full-state Linux keyboard & mouse input engine (works on Ubuntu, Debian,
/// Arch, Fedora, Deepin, 统信 UOS, and 银河麒麟 Kylin!).
/// - Layer 1: X11 XInput2 (XI_RawKeyPress/Release/Button/Motion) — zero sudo on X11/UOS!
/// - Layer 2: Kernel evdev (/dev/input/event*) — works on Wayland + X11 + Steam games!
class InputEngine {
public:
    std::unordered_set<PhysicalKey> activeKeys;
    std::vector<PhysicalKey> pressedOrder;

    bool leftMouseDown = false;
    bool rightMouseDown = false;
    bool otherMouseDown = false;
    double lastMouseMoveTime = 0.0;
    double lastMouseClickTime = 0.0;
    double lastKeyActivityTime = 0.0;

    Pt lastCursorPos = { 0.0f, 0.0f };
    Pt mouseVelocity = { 0.0f, 0.0f };
    int totalKeystrokesToday = 0;

    std::atomic<bool> isXI2Active{false};
    std::atomic<bool> isEvdevActive{false};

    std::function<void(PhysicalKey)> onKeyDown;
    std::function<void(PhysicalKey)> onKeyUp;
    std::function<void(int, bool)> onMouseClick;

    ~InputEngine() { stop(); }

    void start() {
        running_ = true;
        xi2Thread_ = std::thread([this]() { runXI2Loop(); });
        evdevThread_ = std::thread([this]() { runEvdevLoop(); });
    }

    void stop() {
        running_ = false;
        if (xi2Thread_.joinable()) xi2Thread_.join();
        if (evdevThread_.joinable()) evdevThread_.join();
    }

    std::string statusText() const {
        if (isEvdevActive && isXI2Active) return "🟢 硬件全状态监听中 (evdev + XInput2)";
        if (isEvdevActive) return "🟢 内核硬件监听中 (evdev Wayland/X11)";
        if (isXI2Active) return "🟢 全局原始监听中 (XInput2 打字/游戏)";
        return "⚠️ Wayland 需加入 input 组以全局监听";
    }

    /// Drains queued events from background input threads and updates mouse velocity on main 60Hz loop.
    void pollAndDrain(float dt, float fallbackCursorX, float fallbackCursorY, bool hasFallbackCursor) {
        std::vector<RawEvt> batch;
        float relDx = 0.0f, relDy = 0.0f;
        bool hasXCursor = false;
        float xCursorX = 0.0f, xCursorY = 0.0f;
        {
            std::lock_guard<std::mutex> lk(mu_);
            batch.swap(queue_);
            relDx = rawRelX_; rawRelX_ = 0.0f;
            relDy = rawRelY_; rawRelY_ = 0.0f;
            hasXCursor = hasXCursor_;
            xCursorX = xCursorX_;
            xCursorY = xCursorY_;
        }

        for (const auto& e : batch) {
            if (e.isResetKeys) {
                if (!activeKeys.empty() && (nowSeconds() - lastKeyActivityTime > 0.30)) {
                    auto prev = activeKeys;
                    evdevKeys_.clear();
                    xi2Keys_.clear();
                    activeKeys.clear();
                    pressedOrder.clear();
                    for (PhysicalKey k : prev) {
                        if (onKeyUp) onKeyUp(k);
                    }
                }
            } else if (e.isMouseBtn) {
                updateMouseButton(e.mouseButton, e.isDown);
            } else {
                setKeyState(e.key, e.isDown, e.fromEvdev);
            }
        }

        Pt cur = lastCursorPos;
        if (hasXCursor) {
            cur = { xCursorX, xCursorY };
        } else if (hasFallbackCursor) {
            cur = { fallbackCursorX, fallbackCursorY };
        }

        float dx = cur.x - lastCursorPos.x;
        float dy = cur.y - lastCursorPos.y;
        if (!cursorInitialized_) {
            dx = 0.0f; dy = 0.0f;
            cursorInitialized_ = true;
        }
        lastCursorPos = cur;

        if (std::fabs(dx) < 0.5f && std::fabs(dy) < 0.5f && (std::fabs(relDx) > 0.5f || std::fabs(relDy) > 0.5f)) {
            dx = relDx;
            dy = relDy;
        }

        float dist = std::hypot(dx, dy);
        if (dist > 0.4f && dt > 0.001f) {
            lastMouseMoveTime = nowSeconds();
            mouseVelocity.x = dx / dt;
            mouseVelocity.y = dy / dt;
        } else {
            mouseVelocity.x *= 0.82f;
            mouseVelocity.y *= 0.82f;
        }

        double now = nowSeconds();
        if (now - lastTabletScanTime_ > 2.5) {
            refreshTabletPresence(now);
        }
        while (!recentKeyTimes_.empty() && now - recentKeyTimes_.front() > 10.0) {
            recentKeyTimes_.pop_front();
        }
    }

    bool isTabletConnected() const {
        return tabletHardwareFound_;
    }

    void refreshTabletPresence(double now) {
        lastTabletScanTime_ = now;
        bool found = false;
        if (DIR* dir = opendir("/sys/class/input")) {
            struct dirent* ent = nullptr;
            while ((ent = readdir(dir)) != nullptr) {
                if (std::strncmp(ent->d_name, "event", 5) != 0) continue;
                std::string namePath = std::string("/sys/class/input/") + ent->d_name + "/device/name";
                std::ifstream nf(namePath);
                std::string name;
                if (!nf || !std::getline(nf, name)) continue;
                for (char& ch : name) ch = (char)std::tolower((unsigned char)ch);
                if (name.find("touchpad") != std::string::npos || name.find("trackpad") != std::string::npos) continue;
                if (name.find("wacom") != std::string::npos ||
                    name.find("huion") != std::string::npos ||
                    name.find("xp-pen") != std::string::npos ||
                    name.find("gaomon") != std::string::npos ||
                    name.find("ugee") != std::string::npos ||
                    name.find("veikk") != std::string::npos ||
                    name.find("xencelabs") != std::string::npos ||
                    name.find("hanvon") != std::string::npos ||
                    name.find("parblo") != std::string::npos ||
                    name.find("stylus") != std::string::npos ||
                    name.find("pen") != std::string::npos ||
                    name.find("tablet") != std::string::npos) {
                    found = true;
                    break;
                }
            }
            closedir(dir);
        }
        tabletHardwareFound_ = found;
    }

    int currentKPM() const {
        return (int)recentKeyTimes_.size() * 6;
    }

    double lastAnyInputTime() const {
        return std::max(lastKeyActivityTime, std::max(lastMouseMoveTime, lastMouseClickTime));
    }

private:
    struct RawEvt {
        bool isMouseBtn = false;
        bool fromEvdev = false;
        bool isDown = false;
        bool isResetKeys = false;
        int mouseButton = 0;
        PhysicalKey key = PhysicalKey::None;
    };

    std::atomic<bool> running_{false};
    std::thread xi2Thread_;
    std::thread evdevThread_;

    std::mutex mu_;
    std::vector<RawEvt> queue_;
    float rawRelX_ = 0.0f;
    float rawRelY_ = 0.0f;
    bool hasXCursor_ = false;
    float xCursorX_ = 0.0f;
    float xCursorY_ = 0.0f;
    bool cursorInitialized_ = false;
    bool tabletHardwareFound_ = false;
    double lastTabletScanTime_ = -10.0;

    std::unordered_set<PhysicalKey> evdevKeys_;
    std::unordered_set<PhysicalKey> xi2Keys_;
    std::deque<double> recentKeyTimes_;

    void pushKey(PhysicalKey k, bool isDown, bool fromEvdev) {
        if (k == PhysicalKey::None) return;
        std::lock_guard<std::mutex> lk(mu_);
        queue_.push_back({ false, fromEvdev, isDown, false, 0, k });
    }

    void pushMouseBtn(int btn, bool isDown, bool fromEvdev) {
        std::lock_guard<std::mutex> lk(mu_);
        queue_.push_back({ true, fromEvdev, isDown, false, btn, PhysicalKey::None });
    }

    void pushResetKeys() {
        std::lock_guard<std::mutex> lk(mu_);
        queue_.push_back({ false, false, false, true, 0, PhysicalKey::None });
    }

    void setKeyState(PhysicalKey key, bool isDown, bool fromEvdev) {
        bool wasActive = activeKeys.count(key) > 0;
        if (fromEvdev) {
            if (isDown) evdevKeys_.insert(key);
            else { evdevKeys_.erase(key); xi2Keys_.erase(key); }
        } else {
            if (isDown) xi2Keys_.insert(key);
            else { xi2Keys_.erase(key); evdevKeys_.erase(key); }
        }

        activeKeys = evdevKeys_;
        activeKeys.insert(xi2Keys_.begin(), xi2Keys_.end());
        bool isNowActive = activeKeys.count(key) > 0;

        if (!wasActive && isNowActive) {
            pressedOrder.erase(std::remove(pressedOrder.begin(), pressedOrder.end(), key), pressedOrder.end());
            pressedOrder.push_back(key);
            double now = nowSeconds();
            lastKeyActivityTime = now;
            recentKeyTimes_.push_back(now);
            totalKeystrokesToday++;
            if (onKeyDown) onKeyDown(key);
        } else if (wasActive && !isNowActive) {
            pressedOrder.erase(std::remove(pressedOrder.begin(), pressedOrder.end(), key), pressedOrder.end());
            if (onKeyUp) onKeyUp(key);
        }
    }

    void updateMouseButton(int button, bool isDown) {
        bool prev = false;
        if (button == 0) { prev = leftMouseDown; leftMouseDown = isDown; }
        else if (button == 1) { prev = rightMouseDown; rightMouseDown = isDown; }
        else { prev = otherMouseDown; otherMouseDown = isDown; }

        if (prev != isDown) {
            double now = nowSeconds();
            lastMouseMoveTime = now;
            if (isDown) lastMouseClickTime = now;
            if (onMouseClick) onMouseClick(button, isDown);
        }
    }

    // MARK: - Layer 1: X11 XInput2 (Zero-permission global raw input on X11 / Deepin / UOS!)

    void runXI2Loop() {
        Display* dpy = XOpenDisplay(nullptr);
        if (!dpy) return;

        int xiOpcode = 0, eventBase = 0, errorBase = 0;
        if (!XQueryExtension(dpy, "XInputExtension", &xiOpcode, &eventBase, &errorBase)) {
            XCloseDisplay(dpy);
            return;
        }

        // Announce XI 2.2: with 2.0 semantics the server stops sending raw events to us while
        // another client (e.g. a fullscreen game) holds a keyboard/pointer grab.
        int major = 2, minor = 2;
        if (XIQueryVersion(dpy, &major, &minor) != Success) {
            XCloseDisplay(dpy);
            return;
        }

        Window root = DefaultRootWindow(dpy);
        XIEventMask evmask;
        unsigned char mask[XIMaskLen(XI_LASTEVENT)] = {0};
        evmask.deviceid = XIAllMasterDevices;
        evmask.mask_len = sizeof(mask);
        evmask.mask = mask;
        XISetMask(mask, XI_RawKeyPress);
        XISetMask(mask, XI_RawKeyRelease);
        XISetMask(mask, XI_RawButtonPress);
        XISetMask(mask, XI_RawButtonRelease);
        XISetMask(mask, XI_RawMotion);

        XISelectEvents(dpy, root, &evmask, 1);
        XFlush(dpy);
        isXI2Active = true;

        int xfd = ConnectionNumber(dpy);
        double lastPointerQuery = -1.0;
        bool pointerQueryPending = true;
        while (running_) {
            // Only round-trip to the X server for the cursor position when the mouse actually
            // moved (max 60Hz), plus a slow 2Hz refresh. The old unconditional 60Hz XQueryPointer
            // loop added constant load on weak UOS/Kylin machines even with the mouse idle.
            struct pollfd pfd = { xfd, POLLIN, 0 };
            int ret = poll(&pfd, 1, pointerQueryPending ? 16 : 100);

            double nowT = nowSeconds();
            if ((pointerQueryPending && nowT - lastPointerQuery >= 0.016) || nowT - lastPointerQuery > 0.5) {
                Window rRet, cRet;
                int rx = 0, ry = 0, wx = 0, wy = 0;
                unsigned int maskRet = 0;
                if (XQueryPointer(dpy, root, &rRet, &cRet, &rx, &ry, &wx, &wy, &maskRet)) {
                    std::lock_guard<std::mutex> lk(mu_);
                    hasXCursor_ = true;
                    xCursorX_ = (float)rx;
                    xCursorY_ = (float)ry;
                }
                if (nowT - lastPointerQuery > 0.35) {
                    char keys[32] = {};
                    XQueryKeymap(dpy, keys);
                    bool anyHeld = false;
                    for (int i = 0; i < 32; ++i) {
                        if (keys[i] != 0) { anyHeld = true; break; }
                    }
                    if (!anyHeld) pushResetKeys();
                }
                lastPointerQuery = nowT;
                pointerQueryPending = false;
            }

            if (ret <= 0 && XPending(dpy) == 0) continue;
            while (XPending(dpy) > 0) {
                XEvent ev;
                XNextEvent(dpy, &ev);
                if (ev.xcookie.type == GenericEvent && ev.xcookie.extension == xiOpcode) {
                    if (XGetEventData(dpy, &ev.xcookie)) {
                        const auto* raw = static_cast<const XIRawEvent*>(ev.xcookie.data);
                        if (ev.xcookie.evtype == XI_RawKeyPress || ev.xcookie.evtype == XI_RawKeyRelease) {
                            int kc = raw->detail;
                            if (kc >= 8) {
                                PhysicalKey pk = mapLinuxEvdevKey((uint16_t)(kc - 8));
                                pushKey(pk, ev.xcookie.evtype == XI_RawKeyPress, false);
                            }
                        } else if (ev.xcookie.evtype == XI_RawButtonPress || ev.xcookie.evtype == XI_RawButtonRelease) {
                            bool down = (ev.xcookie.evtype == XI_RawButtonPress);
                            if (raw->detail == 1) pushMouseBtn(0, down, false);
                            else if (raw->detail == 3) pushMouseBtn(1, down, false);
                            else if (raw->detail == 2) pushMouseBtn(2, down, false);
                        } else if (ev.xcookie.evtype == XI_RawMotion) {
                            pointerQueryPending = true;
                            double* vals = raw->raw_values;
                            float dx = 0.0f, dy = 0.0f;
                            int idx = 0;
                            if (XIMaskIsSet(raw->valuators.mask, 0)) dx = (float)vals[idx++];
                            if (XIMaskIsSet(raw->valuators.mask, 1)) dy = (float)vals[idx++];
                            if (dx != 0.0f || dy != 0.0f) {
                                std::lock_guard<std::mutex> lk(mu_);
                                rawRelX_ += dx;
                                rawRelY_ += dy;
                            }
                        }
                        XFreeEventData(dpy, &ev.xcookie);
                    }
                }
            }
        }

        isXI2Active = false;
        XCloseDisplay(dpy);
    }

    // MARK: - Layer 2: Kernel evdev (/dev/input/event* — Works on Wayland + X11 + Games!)

    static bool hasEvKeyBit(int fd) {
        unsigned long evBits[(EV_MAX / (8 * sizeof(unsigned long))) + 1] = {0};
        if (ioctl(fd, EVIOCGBIT(0, sizeof(evBits)), evBits) < 0) return false;
        return (evBits[EV_KEY / (8 * sizeof(unsigned long))] & (1UL << (EV_KEY % (8 * sizeof(unsigned long))))) != 0;
    }

    void runEvdevLoop() {
        std::vector<int> fds;
        DIR* dir = opendir("/dev/input");
        if (dir) {
            struct dirent* ent = nullptr;
            while ((ent = readdir(dir)) != nullptr) {
                if (std::strncmp(ent->d_name, "event", 5) != 0) continue;
                std::string path = std::string("/dev/input/") + ent->d_name;
                int fd = open(path.c_str(), O_RDONLY | O_NONBLOCK);
                if (fd >= 0) {
                    if (hasEvKeyBit(fd)) fds.push_back(fd);
                    else close(fd);
                }
            }
            closedir(dir);
        }

        if (fds.empty()) return;
        isEvdevActive = true;

        std::vector<struct pollfd> pfds(fds.size());
        for (size_t i = 0; i < fds.size(); ++i) pfds[i] = { fds[i], POLLIN, 0 };

        while (running_) {
            int ret = poll(pfds.data(), pfds.size(), 25);
            if (ret <= 0) continue;

            for (size_t i = 0; i < pfds.size(); ++i) {
                if (!(pfds[i].revents & POLLIN)) continue;
                struct input_event ev;
                while (read(pfds[i].fd, &ev, sizeof(ev)) == (ssize_t)sizeof(ev)) {
                    if (ev.type == EV_KEY) {
                        if (ev.value == 2) continue; // ignore kernel auto-repeat
                        bool isDown = (ev.value != 0);
                        if (ev.code == BTN_LEFT) pushMouseBtn(0, isDown, true);
                        else if (ev.code == BTN_RIGHT) pushMouseBtn(1, isDown, true);
                        else if (ev.code == BTN_MIDDLE) pushMouseBtn(2, isDown, true);
                        else {
                            PhysicalKey pk = mapLinuxEvdevKey(ev.code);
                            pushKey(pk, isDown, true);
                        }
                    } else if (ev.type == EV_REL) {
                        std::lock_guard<std::mutex> lk(mu_);
                        if (ev.code == REL_X) rawRelX_ += (float)ev.value;
                        else if (ev.code == REL_Y) rawRelY_ += (float)ev.value;
                    }
                }
            }
        }

        isEvdevActive = false;
        for (int fd : fds) close(fd);
    }
};
