#
# A Makefile for osFree Janus Clipboard Viewer
# (c) osFree project
#

DESCRIPTION = osFree Janus Clipboard Viewer
SOURCES = clipbrd cliputils winutils fileutils scrollutils
TARGET_VERSION = 310

ADD_COPT = -sg 
ADD_RCOPT = -i=$(%WATCOM)$(SEP)h$(SEP)win

!include $(%ROOT)tools/mk/build.mk

