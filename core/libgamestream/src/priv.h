#pragma once

#include "http.h"
#include "xml.h"
#include <mbedtls/pk.h>
#include <mbedtls/x509_crt.h>

#define UNIQUE_FILE_NAME "uniqueid.dat"
#define UNIQUE_ID_DEFAULT "0123456789ABCDEF"

#define UNIQUEID_BYTES 8
#define UNIQUEID_CHARS (UNIQUEID_BYTES * 2)

struct GS_CLIENT_T {
    char unique_id[UNIQUEID_CHARS + 1];
    mbedtls_pk_context pk;
    mbedtls_x509_crt cert;
    char cert_hex[8192];
    HTTP *http;
    /* Warning from the last successful launch/resume; empty code when none. */
    char launch_warning_code[GS_LAUNCH_WARNING_CODE_MAX + 1];
    char launch_warning_message[GS_LAUNCH_WARNING_MESSAGE_MAX + 1];
};
