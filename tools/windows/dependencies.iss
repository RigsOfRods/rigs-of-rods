[Tasks]
Name: "install_directx"; Description: "Install DirectX9 runtime"
Name: "install_vsredist"; Description: "Install Visual Studio Redistributable"

[Files]
Source: "@CMAKE_BINARY_DIR@\dxwebsetup.exe"; DestDir: {tmp}; Flags: deleteafterinstall; Tasks: install_directx
Source: "@CMAKE_BINARY_DIR@\vc_redist.x86.exe"; DestDir: {tmp}; Flags: deleteafterinstall; Tasks: install_vsredist
Source: "@CMAKE_BINARY_DIR@\vc_redist.x64.exe"; DestDir: {tmp}; Flags: deleteafterinstall; Tasks: install_vsredist

[Run]
; DirectX
Filename: "{tmp}\dxwebsetup.exe"; Parameters: "/q"; WorkingDir: "{tmp}"; Flags: waituntilterminated skipifdoesntexist runascurrentuser; StatusMsg: "Installing DirectX..."; Tasks: install_directx
; VS redist
Filename: "{tmp}\vc_redist.x64.exe"; Parameters: "/install /passive /norestart"; WorkingDir: "{tmp}"; Flags: waituntilterminated skipifdoesntexist runascurrentuser; StatusMsg: "Installing Visual Studio Redistributable (x64)..."; Tasks: install_vsredist
Filename: "{tmp}\vc_redist.x86.exe"; Parameters: "/install /passive /norestart"; WorkingDir: "{tmp}"; Flags: waituntilterminated skipifdoesntexist runascurrentuser; StatusMsg: "Installing Visual Studio Redistributable (x86)..."; Tasks: install_vsredist