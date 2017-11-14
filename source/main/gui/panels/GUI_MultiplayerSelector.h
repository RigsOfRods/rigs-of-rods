
#pragma once
// TODO: dummy ----- to be reimplemented

namespace RoR{
    namespace GUI {

class MultiplayerSelector
{

public:

    void SetVisible(bool v) {}
    bool IsVisible() {return false;}
    /// Launch refresh from main thread
    void RefreshServerlist() {}
    /// For main thread
    bool IsRefreshThreadRunning() const { return false; }
    /// To be invoked periodically from main thread if refresh is in progress.
    void CheckAndProcessRefreshResult() {}


};

} // namespace GUI

} // namespace RoR
