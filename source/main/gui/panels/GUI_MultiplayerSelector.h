
#pragma once

#include "Application.h"

#include <vector>

namespace RoR{
namespace GUI {

class MultiplayerSelector
{
public:

    struct MpServerData
    {
        MpServerData(const char* name, const char* terrn, int users, int cap, const char* ip, int port, bool pw, int ping);

        bool        has_password;
        Str<50>     display_passwd;
        Str<100>    server_name;
        Str<100>    terrain_name;
        int         num_users;
        int         max_users;
        Str<20>     display_users;
        Str<100>    ip_addr;
        int         net_port;
        int         net_ping;
        Str<50>     display_addr;
        Str<20>     display_ping;
    };

    MultiplayerSelector();

    inline void  SetVisible(bool v)                    { m_is_visible = v; }
    inline bool  IsVisible()                           { return m_is_visible; }
    void         RefreshServerlist();                  /// Launch refresh from main thread
    bool         IsRefreshThreadRunning() const;       /// Check status from main thread
    void         CheckAndProcessRefreshResult();       /// To be invoked periodically from main thread if refresh is in progress.
    void         Draw();

private:
    enum class Mode { ONLINE, DIRECT, SETUP };

    std::vector<MpServerData>       m_servers;
    int                             m_selected_item;
    Mode                            m_mode;
    bool                            m_is_refreshing;
    char                            m_window_title[100];
    bool                            m_is_visible;
};

} // namespace GUI
} // namespace RoR
