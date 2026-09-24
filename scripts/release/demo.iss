#ifndef PackageRoot
  #define PackageRoot "F:\HearthwardDemo\20260924\Windows"
#endif
#ifndef OutputRoot
  #define OutputRoot "F:\HearthwardDemo\Release"
#endif
[Setup]
AppId=Hearthward-Demo
AppName=Hearthward Demo
AppVersion=0.1.0
AppPublisher=XLingyyy
DefaultDirName={localappdata}\Programs\HearthwardDemo
DefaultGroupName=Hearthward Demo
PrivilegesRequired=lowest
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
OutputDir={#OutputRoot}
OutputBaseFilename=Hearthward-Demo-0.1.0-Windows-Setup
Compression=lzma2/fast
SolidCompression=no
DiskSpanning=yes
DiskSliceSize=1500000000
SlicesPerDisk=1
WizardStyle=modern
UninstallDisplayIcon={app}\Hearthward.exe
DisableProgramGroupPage=yes
[Files]
Source: "{#PackageRoot}\*"; DestDir: "{app}"; Excludes: "*.pdb,Manifest_*.txt"; Flags: ignoreversion recursesubdirs createallsubdirs
[Icons]
Name: "{autodesktop}\Hearthward Demo"; Filename: "{app}\Hearthward.exe"; Parameters: "-HearthwardAIBackend=vulkan -HearthwardAIGpuLayers=32"; WorkingDir: "{app}"
Name: "{group}\Hearthward Demo"; Filename: "{app}\Hearthward.exe"; Parameters: "-HearthwardAIBackend=vulkan -HearthwardAIGpuLayers=32"; WorkingDir: "{app}"
Name: "{group}\Hearthward Demo (CPU AI)"; Filename: "{app}\Hearthward.exe"; Parameters: "-HearthwardAIBackend=cpu"; WorkingDir: "{app}"
Name: "{group}\Read Me"; Filename: "{app}\README-DEMO.txt"
[Run]
Filename: "{app}\Engine\Extras\Redist\en-us\vc_redist.x64.exe"; Parameters: "/install /passive /norestart"; Verb: "runas"; Flags: shellexec waituntilterminated; Check: NeedsVCRuntime
Filename: "{app}\Hearthward.exe"; Parameters: "-HearthwardAIBackend=vulkan -HearthwardAIGpuLayers=32"; Description: "Launch Hearthward Demo"; Flags: nowait postinstall skipifsilent

[Code]
function NeedsVCRuntime: Boolean;
var MajorMinor, Build: Cardinal;
begin
  Result := not GetVersionNumbers(ExpandConstant('{sys}\vcruntime140.dll'), MajorMinor, Build);
  if not Result then Result := MajorMinor < $000E002C;
end;
