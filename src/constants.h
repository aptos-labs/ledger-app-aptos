#pragma once

/**
 * Instruction class of the Aptos application.
 */
#define CLA 0x5B

/**
 * Length of APPNAME variable in the Makefile.
 */
#define APPNAME_LEN (sizeof(APPNAME) - 1)

/**
 * Maximum length of MAJOR_VERSION || MINOR_VERSION || PATCH_VERSION.
 */
#define APPVERSION_LEN 3

/**
 * Maximum length of application name.
 */
#define MAX_APPNAME_LEN 64

/**
 * Maximum transaction packets.
 */
#ifndef MAX_TRANSACTION_PACKETS
#define MAX_TRANSACTION_PACKETS 2
#endif

/**
 * Maximum transaction length (bytes).
 */
#define MAX_TRANSACTION_LEN (MAX_TRANSACTION_PACKETS * 255)

/**
 * Signature length (bytes).
 */
#define SIGNATURE_LEN 64

/**
 * Aptos decimal precision.
 */
#define APT_DECIMAL_PRECISION 8
