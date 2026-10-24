/*
 * This file is part of Moonlight Embedded.
 *
 * Copyright (C) 2015 Iwan Timmer
 *
 * Moonlight is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * Moonlight is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Moonlight; if not, see <http://www.gnu.org/licenses/>.
 */

#include "http.h"
#include "errors.h"
#include "set_error.h"
#include "logging.h"

#include <string.h>
#include <curl/curl.h>
#include <pthread.h>
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>

#ifdef __WIN32
#define PATH_SEPARATOR '\\'
#else
#define PATH_SEPARATOR '/'
#endif

struct HTTP_T {
    CURL *curl;
    pthread_mutex_t mutex;
};

static pthread_once_t curl_init_once = PTHREAD_ONCE_INIT;
static CURLcode curl_init_result = CURLE_FAILED_INIT;

static void init_curl(void) {
    curl_init_result = curl_global_init(CURL_GLOBAL_DEFAULT);
}

int http_init(void) {
    if (pthread_once(&curl_init_once, init_curl) != 0 || curl_init_result != CURLE_OK) {
        return gs_set_error(GS_ERROR, "Failed to initialize cURL");
    }
    /* Process lifetime: destroying a client must never tear down the resolver
     * used by other clients, the controller updater or vendor libraries. */
    return GS_OK;
}

static size_t write_fn(void *contents, size_t size, size_t nmemb, void *userp) {
    if (size != 0 && nmemb > SIZE_MAX / size) {
        return 0;
    }
    size_t realsize = size * nmemb;
    HTTP_DATA *mem = (HTTP_DATA *) userp;
    if (realsize == 0) {
        return 0;
    }
    if (mem->size == SIZE_MAX || realsize > SIZE_MAX - mem->size - 1) {
        return 0;
    }

    void *allocated = realloc(mem->memory, mem->size + realsize + 1);
    if (allocated == NULL) {
        return 0;
    }
    mem->memory = allocated;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;
}

HTTP *http_create(const char *keydir) {
    if (http_init() != GS_OK) {
        return NULL;
    }
    CURL *curl = curl_easy_init();
    if (curl == NULL) {
        gs_set_error(GS_ERROR, "Failed to create cURL instance");
        return NULL;
    }

    char certificateFilePath[4096];
    sprintf(certificateFilePath, "%s%c%s", keydir, PATH_SEPARATOR, CERTIFICATE_FILE_NAME);

    char keyFilePath[4096];
    sprintf(&keyFilePath[0], "%s%c%s", keydir, PATH_SEPARATOR, KEY_FILE_NAME);

    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(curl, CURLOPT_SSLENGINE_DEFAULT, 1L);
    curl_easy_setopt(curl, CURLOPT_SSLCERTTYPE, "PEM");
    curl_easy_setopt(curl, CURLOPT_SSLCERT, certificateFilePath);
    curl_easy_setopt(curl, CURLOPT_SSLKEYTYPE, "PEM");
    curl_easy_setopt(curl, CURLOPT_SSLKEY, keyFilePath);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_fn);
    curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_SESSIONID_CACHE, 0L);
    // Fix for https://github.com/mariotaku/moonlight-tv/issues/452
    curl_easy_setopt(curl, CURLOPT_FORBID_REUSE, 1L);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);

    struct HTTP_T *http = malloc(sizeof(struct HTTP_T));
    if (http == NULL) {
        curl_easy_cleanup(curl);
        gs_set_error(GS_OUT_OF_MEMORY, "Failed to allocate HTTP client");
        return NULL;
    }
    http->curl = curl;
    if (pthread_mutex_init(&http->mutex, NULL) != 0) {
        curl_easy_cleanup(curl);
        free(http);
        gs_set_error(GS_ERROR, "Failed to create HTTP mutex");
        return NULL;
    }
    return http;
}

int http_request(HTTP *http, char *url, HTTP_DATA *data) {
    assert(http != NULL);
    assert(data != NULL);
    pthread_mutex_lock(&http->mutex);
    data->memory[0] = 0;
    data->size = 0;
    CURL *curl = http->curl;
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, data);
    curl_easy_setopt(curl, CURLOPT_URL, url);

    commons_log_debug("GameStream", "Request %p %s", data, url);

    int ret = GS_FAILED;
    CURLcode res = curl_easy_perform(curl);

    if (res == CURLE_HTTP_RETURNED_ERROR) {
        long http_status = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_status);
        commons_log_warn("GameStream", "Request %p error HTTP %ld", data, http_status);
        goto finish;
    } else if (res != CURLE_OK) {
        const char *errmsg = curl_easy_strerror(res);
        ret = gs_set_error(GS_IO_ERROR, "cURL error: %s", errmsg);
        commons_log_debug("GameStream", "Request %p error %d: %s", data, ret, errmsg);
        goto finish;
    }
    assert (data->memory != NULL);
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_HTTP_CODE, &http_code);
    commons_log_debug("GameStream", "Request %p response %ld", data, http_code);
    commons_log_hexdump(COMMONS_LOG_LEVEL_VERBOSE, "GameStream", data->memory, data->size);

    ret = GS_OK;
    finish:
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, NULL);
    pthread_mutex_unlock(&http->mutex);
    return ret;
}

int http_post_json(HTTP *http, char *url, const char *payload, HTTP_DATA *data) {
    assert(http != NULL);
    assert(data != NULL);
    pthread_mutex_lock(&http->mutex);
    data->memory[0] = 0;
    data->size = 0;
    CURL *curl = http->curl;
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, data);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long) strlen(payload));
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json; charset=utf-8");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    int ret = GS_FAILED;
    CURLcode res = curl_easy_perform(curl);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, NULL);
    curl_easy_setopt(curl, CURLOPT_POST, 0L);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, NULL);
    curl_slist_free_all(headers);

    if (res == CURLE_HTTP_RETURNED_ERROR) {
        goto finish;
    } else if (res != CURLE_OK) {
        ret = gs_set_error(GS_IO_ERROR, "cURL error: %s", curl_easy_strerror(res));
        goto finish;
    }
    ret = GS_OK;
    finish:
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, NULL);
    pthread_mutex_unlock(&http->mutex);
    return ret;
}

void http_destroy(HTTP *http) {
    assert(http != NULL);
    pthread_mutex_lock(&http->mutex);
    curl_easy_cleanup(http->curl);
    pthread_mutex_unlock(&http->mutex);
    pthread_mutex_destroy(&http->mutex);
    free((void *) http);
}

void http_set_timeout(HTTP *http, int timeout) {
    assert(http != NULL);
    pthread_mutex_lock(&http->mutex);
    curl_easy_setopt(http->curl, CURLOPT_CONNECTTIMEOUT, (long) timeout);
    pthread_mutex_unlock(&http->mutex);
}

/* Whole-transfer ceiling (CURLOPT_TIMEOUT), not just connect: without one, a
 * host that accepts the TCP connection and then hangs stalls the caller
 * forever. 0 restores curl's default (no total timeout). */
void http_set_total_timeout(HTTP *http, int timeout) {
    assert(http != NULL);
    pthread_mutex_lock(&http->mutex);
    curl_easy_setopt(http->curl, CURLOPT_TIMEOUT, (long) timeout);
    pthread_mutex_unlock(&http->mutex);
}

HTTP_DATA *http_data_alloc() {
    HTTP_DATA *data = malloc(sizeof(HTTP_DATA));
    if (data == NULL) {
        return NULL;
    }

    data->memory = malloc(1);
    if (data->memory == NULL) {
        free(data);
        return NULL;
    }
    data->memory[0] = 0;
    data->size = 0;

    return data;
}

void http_data_free(HTTP_DATA *data) {
    if (data == NULL) {
        return;
    }
    if (data->memory != NULL) {
        free(data->memory);
    }

    free(data);
}
