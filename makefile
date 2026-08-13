#
# A Makefile for osFree Janus Clipboard Viewer
# (c) osFree project
#

DESC = osFree Janus Clipboard Viewer
TARGET_API = WIN
TARGET_VERSION = 310
SOURCES= clipbrd cliputils winutils fileutils scrollutils
LIBS = commdlg

# defines additional options for C compiler
ADD_COPT = -sg 

!include $(%ROOT)tools/mk/build.mk

