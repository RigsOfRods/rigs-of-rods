#include "symlink.h"
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32

HRESULT createLink2(LPCWSTR lpszPathObj, LPCWSTR lpszWorkingDir, LPCWSTR lpszPathLink, LPCWSTR lpszDesc)
{
    HRESULT hres;
    IShellLink* psl;

    // Get a pointer to the IShellLink interface.
    hres = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (LPVOID*)&psl);
    if (SUCCEEDED(hres))
    {
        IPersistFile* ppf;

        // Set the path to the shortcut target and add the description.
        psl->SetPath(lpszPathObj);
		psl->SetWorkingDirectory(lpszWorkingDir);
        psl->SetDescription(lpszDesc);

        // Query IShellLink for the IPersistFile interface for saving the
        // shortcut in persistent storage.
        hres = psl->QueryInterface(IID_IPersistFile, (LPVOID*)&ppf);

        if (SUCCEEDED(hres))
        { 			
            // Save the link by calling IPersistFile::Save.
            hres = ppf->Save(lpszPathLink, TRUE);
            ppf->Release();
        }
        psl->Release();
    }
    return hres;
}

// helper function to convert std::string to std::wstring to LPWSTR
std::wstring s2ws(const std::string& s)
{
    int len;
    int slength = (int)s.length() + 1;
    len = MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, 0, 0);
    wchar_t* buf = new wchar_t[len];
    MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, buf, len);
    std::wstring r(buf);
    delete[] buf;
    return r;
}

// small wxwidgets wrapper
int createLink(std::string target_str, std::string workingdir_str, std::string filename_str, std::string description_str)
{
	std::wstring target = s2ws(target_str);
	std::wstring filename = s2ws(filename_str);
	std::wstring workingdir = s2ws(workingdir_str);
	std::wstring description = s2ws(description_str);
	return createLink2(target.c_str(), workingdir.c_str(), filename.c_str(), description.c_str());
}

#endif //OGRE_PLATFORM
