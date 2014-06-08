#include "wxStrel.h"

IMPLEMENT_ABSTRACT_CLASS(wxStrel, wxPanel)

BEGIN_EVENT_TABLE(wxStrel, wxPanel)
    EVT_LEFT_UP(wxStrel::clickEvent)
END_EVENT_TABLE()
