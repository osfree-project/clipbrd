#
# A Makefile for WinOS/2 Clipboard Viewer
# (c) osFree project,
#

DESCRIPTION = Windows Clipboard Viewer
SOURCES = clipbrd cliputils winutils fileutils scrollutils
LIBS = commdlg

ADD_COPT = -sg 
ADD_RCOPT = -i=$(%WATCOM)$(SEP)h$(SEP)win

!include $(%ROOT)tools/mk/build.mk

