# Microsoft Developer Studio Project File - Name="flite" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=flite - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "flite.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "flite.mak" CFG="flite - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "flite - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "flite - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "flite - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /I "..\..\include" /D "NDEBUG" /D "WIN32" /D "_MBCS" /D "_LIB" /D "NO_UNION_INITIALIZATION" /D "CST_AUDIO_WINCE" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "flite - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I "..\..\include" /D "_DEBUG" /D "WIN32" /D "_MBCS" /D "_LIB" /D "NO_UNION_INITIALIZATION" /D "CST_AUDIO_NONE" /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "flite - Win32 Release"
# Name "flite - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Group "audio"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\audio\au_none.c
# End Source File
# Begin Source File

SOURCE=..\..\src\audio\audio.c
# End Source File
# Begin Source File

SOURCE=..\..\src\audio\native_audio.h
# End Source File
# End Group
# Begin Group "hrg"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\hrg\cst_ffeature.c
# End Source File
# Begin Source File

SOURCE=..\..\src\hrg\cst_item.c
# End Source File
# Begin Source File

SOURCE=..\..\src\hrg\cst_rel_io.c
# End Source File
# Begin Source File

SOURCE=..\..\src\hrg\cst_relation.c
# End Source File
# Begin Source File

SOURCE=..\..\src\hrg\cst_utterance.c
# End Source File
# End Group
# Begin Group "lexicon"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\lexicon\cst_lexicon.c
# End Source File
# Begin Source File

SOURCE=..\..\src\lexicon\cst_lts.c
# End Source File
# End Group
# Begin Group "regex"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\regex\cst_regex.c
# End Source File
# Begin Source File

SOURCE=..\..\src\regex\regexp.c
# End Source File
# Begin Source File

SOURCE=..\..\src\regex\regsub.c
# End Source File
# End Group
# Begin Group "speech"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\speech\cst_lpcres.c
# End Source File
# Begin Source File

SOURCE=..\..\src\speech\cst_track.c
# End Source File
# Begin Source File

SOURCE=..\..\src\speech\cst_track_io.c
# End Source File
# Begin Source File

SOURCE=..\..\src\speech\cst_wave.c
# End Source File
# Begin Source File

SOURCE=..\..\src\speech\cst_wave_io.c
# End Source File
# Begin Source File

SOURCE=..\..\src\speech\cst_wave_utils.c
# End Source File
# Begin Source File

SOURCE=..\..\src\speech\rateconv.c
# End Source File
# End Group
# Begin Group "stats"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\stats\cst_cart.c
# End Source File
# Begin Source File

SOURCE=..\..\src\stats\cst_viterbi.c
# End Source File
# End Group
# Begin Group "synth"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\synth\cst_phoneset.c
# End Source File
# Begin Source File

SOURCE=..\..\src\synth\cst_synth.c
# End Source File
# Begin Source File

SOURCE=..\..\src\synth\cst_utt_utils.c
# End Source File
# Begin Source File

SOURCE=..\..\src\synth\cst_voice.c
# End Source File
# Begin Source File

SOURCE=..\..\src\synth\flite.c
# End Source File
# End Group
# Begin Group "utils"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\utils\cst_alloc.c
# End Source File
# Begin Source File

SOURCE=..\..\src\utils\cst_args.c
# End Source File
# Begin Source File

SOURCE=..\..\src\utils\cst_endian.c
# End Source File
# Begin Source File

SOURCE=..\..\src\utils\cst_error.c
# End Source File
# Begin Source File

SOURCE=..\..\src\utils\cst_features.c
# End Source File
# Begin Source File

SOURCE=..\..\src\utils\cst_file_stdio.c
# End Source File
# Begin Source File

SOURCE=..\..\src\utils\cst_mmap_win32.c
# End Source File
# Begin Source File

SOURCE=..\..\src\utils\cst_string.c
# End Source File
# Begin Source File

SOURCE=..\..\src\utils\cst_tokenstream.c
# End Source File
# Begin Source File

SOURCE=..\..\src\utils\cst_val.c
# End Source File
# Begin Source File

SOURCE=..\..\src\utils\cst_val_const.c
# End Source File
# Begin Source File

SOURCE=..\..\src\utils\cst_val_user.c
# End Source File
# End Group
# Begin Group "wavesynth"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\wavesynth\cst_clunits.c
# End Source File
# Begin Source File

SOURCE=..\..\src\wavesynth\cst_diphone.c
# End Source File
# Begin Source File

SOURCE=..\..\src\wavesynth\cst_sigpr.c
# End Source File
# Begin Source File

SOURCE=..\..\src\wavesynth\cst_sigprFP.c
# End Source File
# Begin Source File

SOURCE=..\..\src\wavesynth\cst_sts.c
# End Source File
# Begin Source File

SOURCE=..\..\src\wavesynth\cst_units.c
# End Source File
# End Group
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\include\cst_alloc.h
# End Source File
# Begin Source File

SOURCE=..\..\include\cst_args.h
# End Source File
# Begin Source File

SOURCE=..\..\include\cst_audio.h
# End Source File
# Begin Source File

SOURCE=..\..\include\cst_cart.h
# End Source File
# Begin Source File

SOURCE=..\..\include\cst_clunits.h
# End Source File
# Begin Source File

SOURCE=..\..\include\cst_diphone.h
# End Source File
# Begin Source File

SOURCE=..\..\include\cst_endian.h
# End Source File
# Begin Source File

SOURCE=..\..\include\cst_error.h
# End Source File
# Begin Source File

SOURCE=..\..\include\cst_features.h
# End Source File
# Begin Source File

SOURCE=..\..\include\cst_file.h
# End Source File
# Begin Source File

SOURCE=..\..\include\cst_hrg.h
# End Source File
# Begin Source File

SOURCE=..\..\include\cst_item.h
# End Source File
# Begin Source File

SOURCE=..\..\include\cst_lexicon.h
# End Source File
# Begin Source File

SOURCE=..\..\include\cst_lts.h
# End Source File
# Begin Source File

SOURCE=..\..\include\cst_phoneset.h
# End Source File
# Begin Source File

SOURCE=..\..\include\cst_regex.h
# End Source File
# Begin Source File

SOURCE=..\..\include\cst_relation.h
# End Source File
# Begin Source File

SOURCE=..\..\include\cst_sigpr.h
# End Source File
# Begin Source File

SOURCE=..\..\include\cst_socket.h
# End Source File
# Begin Source File

SOURCE=..\..\include\cst_string.h
# End Source File
# Begin Source File

SOURCE=..\..\include\cst_sts.h
# End Source File
# Begin Source File

SOURCE=..\..\include\cst_synth.h
# End Source File
# Begin Source File

SOURCE=..\..\include\cst_tokenstream.h
# End Source File
# Begin Source File

SOURCE=..\..\include\cst_track.h
# End Source File
# Begin Source File

SOURCE=..\..\include\cst_units.h
# End Source File
# Begin Source File

SOURCE=..\..\include\cst_utt_utils.h
# End Source File
# Begin Source File

SOURCE=..\..\include\cst_utterance.h
# End Source File
# Begin Source File

SOURCE=..\..\include\cst_val.h
# End Source File
# Begin Source File

SOURCE=..\..\include\cst_val_const.h
# End Source File
# Begin Source File

SOURCE=..\..\include\cst_val_defs.h
# End Source File
# Begin Source File

SOURCE=..\..\include\cst_viterbi.h
# End Source File
# Begin Source File

SOURCE=..\..\include\cst_voice.h
# End Source File
# Begin Source File

SOURCE=..\..\include\cst_wave.h
# End Source File
# Begin Source File

SOURCE=..\..\include\flite.h
# End Source File
# Begin Source File

SOURCE=..\..\include\flite_version.h
# End Source File
# End Group
# End Target
# End Project
