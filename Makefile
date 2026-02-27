ifeq ($(BOLOS_SDK),)
$(error Environment variable BOLOS_SDK is not set)
endif

include $(BOLOS_SDK)/Makefile.target

########################################
#        Mandatory configuration       #
########################################

APPNAME      = "Aptos"
APPVERSION_M = 0
APPVERSION_N = 1
APPVERSION_P = 0
APPVERSION   = "$(APPVERSION_M).$(APPVERSION_N).$(APPVERSION_P)"

APP_SOURCE_PATH += src

ICON_NANOX = icons/nanox_app_aptos.gif
ICON_NANOSP = icons/nanox_app_aptos.gif
ICON_STAX  = icons/stax_app_aptos.gif
ICON_FLEX  = icons/flex_app_aptos.gif

CURVE_APP_LOAD_PARAMS = ed25519

PATH_APP_LOAD_PARAMS = "44'/637'"

VARIANT_PARAM  = COIN
VARIANT_VALUES = APT

########################################
# Application communication interfaces #
########################################

ENABLE_BLUETOOTH = 1

########################################
#          Features disablers          #
########################################

# App provides its own main(), io_event(), io_exchange_al() and
# command_t / apdu_parser() — prevent the SDK from compiling its
# lib_standard_app copies which would cause duplicate symbols.
DISABLE_STANDARD_APP_FILES = 1

include $(BOLOS_SDK)/Makefile.standard_app
