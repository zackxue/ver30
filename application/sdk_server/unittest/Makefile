# Set the platform that we are working on, defualt is 'simulator'
GMI_PLATFORM           ?= simulator

# Set top dir of the compile system
GMI_TOPDIR             ?= $(word 1, $(subst application/sdk_server, ,$(shell pwd)))

# Set if it will show the compile command line
GMI_MAKE_VERBOSE       ?= @

# Export these variables
export GMI_PLATFORM
export GMI_TOPDIR
export GMI_MAKE_VERBOSE

# Sub item list that need to be compiled
# To facilitate reading and compiling system can work, please keep module compiling sequence, principle as follows:
# 1 module dependency
# 2 alplabet sequence
GMI_ITEMS                = forgestream psdump logsdk udpcs evtimeout dmtest mdcmem uxevcli uxsvr uslp noacc alarm udpsock

# Include item_rules.mk
include $(GMI_TOPDIR)/build/item_rules.mk



