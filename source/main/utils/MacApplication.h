// SPDX-License-Identifier: GPL-3.0-only
// Standalone macOS application ownership. No Objective-C types escape this header.
#pragma once
#include <memory>

namespace RoR {
class MacAutoreleasePool
{
public:
    MacAutoreleasePool();
    ~MacAutoreleasePool();
    MacAutoreleasePool(const MacAutoreleasePool&) = delete;
    MacAutoreleasePool& operator=(const MacAutoreleasePool&) = delete;
private:
    void* m_pool;
};

class MacApplication
{
public:
    MacApplication() noexcept; // Inert: safe before command line early returns.
    ~MacApplication();
    MacApplication(const MacApplication&) = delete;
    MacApplication& operator=(const MacApplication&) = delete;
    void Activate();
    void AttachWindow(void* window); // Pinned Ogre getCustomAttribute("WINDOW", &window).
    void DetachWindow() noexcept;
    void Pump();
    bool QuitRequested() const noexcept;
    bool ConsumeInputReset() noexcept;
private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};
} // namespace RoR
