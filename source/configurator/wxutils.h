#pragma once
#ifndef UTILS_H__
#define UTILS_H__

#if OGRE_PLATFORM == OGRE_PLATFORM_LINUX
# ifdef __WXGTK__
#  include <gtk/gtk.h>
#  include <gdk/gdkx.h>
# endif //__WXGTK__
#endif //OGRE_PLATFORM_LINUX

inline wxString conv(const char *s)
{
    return wxString(s, wxConvUTF8);
}

inline wxString conv(const std::string& s)
{
    return wxString(s.c_str(), wxConvUTF8);
}

inline std::string conv(const wxString& s)
{
    return std::string(s.mb_str(wxConvUTF8));
}

inline const char *conv2(const wxString& s)
{
    return std::string(s.mb_str(wxConvUTF8)).c_str();
}

inline const char *conv2(const std::string& s)
{
    return s.c_str();
}

inline size_t getOISHandle(wxWindow *window)
{
    size_t hWnd = 0;
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
    hWnd = (size_t)window->GetHandle();
#elif OGRE_PLATFORM == OGRE_PLATFORM_LINUX
#ifdef __WXGTK__
#if wxCHECK_VERSION(2, 8, 0)
    if (!window->IsShownOnScreen()) {
        wxLogStatus(wxT("getOISHandle(): Window needs to be realized before we can get its XID. Showing the window to avoid crash!"));
        window->Show();
    }
#else
    window->Show();
#endif //wxCHECK_VERSION(2, 8, 0)
#if GTK_CHECK_VERSION (2,14,0)
    // GTK 2.14 includes gtk_widget_get_window()
    hWnd = (size_t)GDK_WINDOW_XID(gtk_widget_get_window(window->GetHandle()));
#else
    // but we can't use that if GTK is too old...
    hWnd = (size_t)GDK_WINDOW_XID(window->GetHandle()->window);
#endif
#else
    // TODO: support other WX configs ?
#error "WX configurations other than GTK not supported yet!"
#endif
#elif OGRE_PLATFORM == OGRE_PLATFORM_APPLE
    // TODO: Apple specific code ?
#error "Apple specific code not written yet!"
#endif
    return hWnd;
}

#endif //UTILS_H__
