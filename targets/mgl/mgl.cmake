# MGL target: run the OpenGL CTS against MGL's libmgl.dylib on macOS.

set(DEQP_TARGET_NAME "MGL")

find_library(COCOA_LIBRARY Cocoa)
find_library(QUARTZCORE_LIBRARY QuartzCore)
set(DEQP_PLATFORM_LIBRARIES ${COCOA_LIBRARY} ${QUARTZCORE_LIBRARY})

# Replaces the stock OS X (CGL) platform port.
set(TCUTIL_PLATFORM_SRCS
	mgl/tcuMGLPlatform.cpp
	)

if (DEFINED MGL_LIBRARY_PATH)
	add_definitions(-DMGL_DEFAULT_LIBRARY_PATH="${MGL_LIBRARY_PATH}")
endif ()
