/*
 *  win-installer.nsi
 *
 *  Copyright (C) 2008 - Jani Rivinoja
 *
 *  This file is part of Outgun.
 *
 *  Outgun is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Outgun is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Outgun; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

!ifndef NAME
  !define NAME 'Outgun'
!endif

!ifndef VERSION
  !define VERSION 'anonymous-build'
!endif

Name ${NAME}

CRCCheck on

# Set UI to use Windows colours
InstallColors /windows
LicenseBkColor /windows

# Installer file name
OutFile "${NAME} ${VERSION}.exe"

# The default installation directory
InstallDir $PROGRAMFILES\Outgun

# Request application privileges for Windows Vista
RequestExecutionLevel user

PageEx license
  LicenseData COPYING
PageExEnd

Page directory
Page components
Page instfiles

# Component sections

Section "Outgun"
  SectionIn RO
  SetOutPath $INSTDIR
  File outgun.exe
  File alleg40.dll
  File nl.dll
  File pthreadGC.dll
  File keyboard.dat
  File README.txt
  File COPYING
  File /r client_stats
  File /r config
  File /r languages
  File /r server_stats
SectionEnd

Section "Documentation"
  SetOutPath $INSTDIR
  File /r doc
SectionEnd

SectionGroup /e "Graphics and sounds"
  Section "Graphic themes"
    SetOutPath $INSTDIR
    File /r graphics
  SectionEnd

  Section "Extra fonts"
    SetOutPath $INSTDIR
    File /r fonts
  SectionEnd

  Section "Sound themes"
    SetOutPath $INSTDIR
    File /r sound
  SectionEnd
SectionGroupEnd

Section "Maps"
  SetOutPath $INSTDIR
  File /r maps
  File /r cmaps
SectionEnd

SectionGroup /e "Server specific files"
  Section "Dedicated server batch file"
    SetOutPath $INSTDIR
    File "Dedicated server.bat"
  SectionEnd

  Section "Random map templates"
    SetOutPath $INSTDIR
    File /r mapgen
  SectionEnd

  Section /o "Server monitor"
    SetOutPath $INSTDIR
    #File srvmonit.exe
  SectionEnd

  Section /o "Relay"
    SetOutPath $INSTDIR
    #File relay.exe
  SectionEnd
SectionGroupEnd
