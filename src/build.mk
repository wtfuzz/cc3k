TARGET_CC3K_PATH = src

# Add tropicssl include to all objects built for this target
INCLUDE_DIRS += include

# C source files included in this build.
CSRC += src/cc3k.c
CSRC += src/cc3k_packet.c
CSRC += src/cc3k_socket.c

# ASM source files included in this build.
ASRC +=

