# ****************************************************************************
#    Ledger App Aptos
#    (c) 2020 Ledger SAS.
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.
# ****************************************************************************

ifeq ($(BOLOS_SDK),)
$(error Environment variable BOLOS_SDK is not set)
endif

include $(BOLOS_SDK)/Makefile.defines

########################################
#          Aptos configuration         #
########################################

ifeq ($(TARGET_NAME),TARGET_NANOS)
    DEFINES += MAX_TRANSACTION_PACKETS=6
endif
ifeq ($(TARGET_NAME),TARGET_NANOS2)
    DEFINES += MAX_TRANSACTION_PACKETS=106
endif
ifeq ($(TARGET_NAME),TARGET_NANOX)
    DEFINES += MAX_TRANSACTION_PACKETS=90
endif
ifeq ($(TARGET_NAME),TARGET_STAX)
    # still need to find the right value
    DEFINES += MAX_TRANSACTION_PACKETS=70
endif

########################################
#        Mandatory configuration       #
########################################
# Application name
APPNAME = "Aptos"

# Application version
APPVERSION_M = 0
APPVERSION_N = 6
APPVERSION_P = 9
APPVERSION   = "$(APPVERSION_M).$(APPVERSION_N).$(APPVERSION_P)"

# Application source files
APP_SOURCE_PATH += src

# Application icons
ICON_NANOS = icons/app_aptos_16px.gif
ICON_NANOX = icons/app_aptos_14px.gif
ICON_NANOSP = icons/app_aptos_14px.gif
ICON_STAX = icons/app_aptos_32px.gif

# Application allowed derivation curves
CURVE_APP_LOAD_PARAMS = ed25519

# Application allowed derivation paths
PATH_APP_LOAD_PARAMS = "44'/637'"

# Setting to allow building variant applications
# - <VARIANT_PARAM> is the name of the parameter which should be set
#   to specify the variant that should be build.
# - <VARIANT_VALUES> a list of variant that can be build using this app code.
#   * It must at least contains one value.
#   * Values can be the app ticker or anything else but should be unique.
VARIANT_PARAM = COIN
VARIANT_VALUES = APTOS

# Enabling DEBUG flag will enable PRINTF and disable optimizations
#DEBUG = 1

########################################
#     Application custom permissions   #
########################################
# See SDK `include/appflags.h` for the purpose of each permission
#HAVE_APPLICATION_FLAG_DERIVE_MASTER = 1
#HAVE_APPLICATION_FLAG_GLOBAL_PIN = 1
#HAVE_APPLICATION_FLAG_BOLOS_SETTINGS = 1
#HAVE_APPLICATION_FLAG_LIBRARY = 1

########################################
# Application communication interfaces #
########################################
ENABLE_BLUETOOTH = 1
#ENABLE_NFC = 1

########################################
#         NBGL custom features         #
########################################
ENABLE_NBGL_QRCODE = 1
#ENABLE_NBGL_KEYBOARD = 1
#ENABLE_NBGL_KEYPAD = 1

########################################
#          Features disablers          #
########################################
# These advanced settings allow to disable some feature that are by
# default enabled in the SDK `Makefile.standard_app`.
#DISABLE_STANDARD_APP_FILES = 1
#DISABLE_DEFAULT_IO_SEPROXY_BUFFER_SIZE = 1 # To allow custom size declaration
#DISABLE_STANDARD_APP_DEFINES = 1 # Will set all the following disablers
#DISABLE_STANDARD_SNPRINTF = 1
#DISABLE_STANDARD_USB = 1
#DISABLE_STANDARD_WEBUSB = 1
#DISABLE_STANDARD_BAGL_UX_FLOW = 1

include $(BOLOS_SDK)/Makefile.standard_app
