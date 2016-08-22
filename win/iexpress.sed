[Version]
Class=IEXPRESS
SEDVersion=3

[Options]
PackagePurpose=InstallApp
ShowInstallProgramWindow=0
HideExtractAnimation=0
UseLongFileName=1
InsideCompressed=0
CAB_FixedSize=0
CAB_ResvCodeSigning=0
RebootMode=I
InstallPrompt=Install Reminderer?
DisplayLicense=
FinishMessage=Installation complete.
TargetName=reminderer_inst.exe
FriendlyName=Reminderer
AppLaunched=reminderer.inf
PostInstallCmd=<None>
AdminQuietInstCmd=
UserQuietInstCmd=
SourceFiles=SourceFiles

[SourceFiles]
SourceFiles0=C:\gtk3\bin
SourceFiles1=..\src
SourceFiles2=..\data
SourceFiles3=.
SourceFiles4=Release
SourceFiles5=C:\gtk3\share\glib-2.0\schemas

[SourceFiles0]
libatk-1.0-0.dll=
libcairo-2.dll=
libcairo-gobject-2.dll=
libffi-6.dll=
libfontconfig-1.dll=
libfreetype-6.dll=
libgdk-3-0.dll=
libgdk_pixbuf-2.0-0.dll=
libgio-2.0-0.dll=
libglib-2.0-0.dll=
libgmodule-2.0-0.dll=
libgobject-2.0-0.dll=
libgtk-3-0.dll=
libiconv-2.dll=
libintl-8.dll=
liblzma-5.dll=
libpango-1.0-0.dll=
libpangocairo-1.0-0.dll=
libpangoft2-1.0-0.dll=
libpangowin32-1.0-0.dll=
libpixman-1-0.dll=
libpng15-15.dll=
libxml2-2.dll=
pthreadGC2.dll=
zlib1.dll=

[SourceFiles1]
event_edit.ui=
reminderer.ui=
window_about.ui=
window_help.ui=
window_prefs.ui=

[SourceFiles2]
reminderer.png=

[SourceFiles3]
reminderer.inf=

[SourceFiles4]
reminderer.exe=

[SourceFiles5]
gschemas.compiled=
