#
# SPDX-License-Identifier: BSD-2-Clause
#
# Copyright (C) 2019 FORTH-ICS/CARV
#		Panagiotis Peristerakis <perister@ics.forth.gr>
#

# Platform XLEN
PLATFORM_RISCV_XLEN = 64

# Blobs to build
FW_TEXT_START=0x80000000

# Dynamic firmware configuration
FW_DYNAMIC=n

# Firmware that jumps at fixed address
FW_JUMP=n
# FW_JUMP_ADDR=0x80200000           # This needs to be 2MB aligned for 64-bit support
# FW_JUMP_FDT_ADDR=0x80800000       # TODO: this is a cancerous workaround; set this correctly, ideally built! 

# Firmware with payload configuration.
FW_PAYLOAD=y
# FW_PAYLOAD_ALIGN=0x1000
FW_PAYLOAD_OFFSET=0x200000        # This needs to be 2MB aligned for 64-bit support
FW_PAYLOAD_FDT_ADDR=0x82200000    # TODO: this is a cancerous workaround; set this correctly, ideally built!