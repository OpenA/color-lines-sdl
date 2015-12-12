[Setup]
AppName=COLOR-LINES
AppVerName=COLOR-LINES 0.6
DefaultDirName={pf}\Pinebrush games\COLOR-LINES
DefaultGroupName=Pinebrush games
UninstallDisplayIcon={app}\lines.exe
OutputDir=.
OutputBaseFilename=lines-0.6
AllowNoIcons=true

[Languages]
Name: en; MessagesFile: compiler:Default.isl
Name: ru; MessagesFile: compiler:Languages\Russian.isl

[Files]
Source: lines.exe; DestDir: {app}
Source: icon\*; DestDir: {app}\icon
Source: gfx\*; DestDir: {app}\gfx
Source: sounds\*; DestDir: {app}\sounds
Source: *.dll; DestDir: {app}

[CustomMessages]
CreateDesktopIcon=Create a &desktop icon
LaunchGame=Launch &game
UninstallMsg=Uninstall COLOR-LINES
ru.CreateDesktopIcon=Создать &ярлык на рабочем столе
ru.LaunchGame=Запустить &игру
ru.UninstallMsg=Удалить COLOR-LINES

[Tasks]
Name: desktopicon; Description: {cm:CreateDesktopIcon}

[Run]
Filename: {app}\lines.exe; Description: {cm:LaunchGame}; WorkingDir: {app}; Flags: postinstall

[Icons]
Name: {commondesktop}\COLOR-LINES; Filename: {app}\lines.exe; WorkingDir: {app}; Tasks: desktopicon
Name: {group}\COLOR-LINES; Filename: {app}\lines.exe; WorkingDir: {app}
Name: {group}\{cm:UninstallMsg}; Filename: {uninstallexe}

[UninstallDelete]
Name: {app}; Type: dirifempty
Name: {pf}\Pinebrush games; Type: dirifempty
