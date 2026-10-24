/* Actual HTTP implementation with a deterministic transport. No app or network. */
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <curl/curl.h>
#include "http.h"
#include "errors.h"
#include "logging.h"
#include <assert.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct {
    curl_write_callback write;
    void *data;
    const char *post_fields;
    struct curl_slist *headers;
    long post, nosignal;
} mock_curl;

static atomic_int init_calls, creates, destroys;
static atomic_bool initialized;
static bool fail_init;
static _Thread_local CURLcode response;
static _Thread_local int write_case;

CURLcode curl_global_init(long flags) {
    assert(flags == CURL_GLOBAL_DEFAULT);
    atomic_fetch_add(&init_calls, 1);
    usleep(10000); /* Force competing first users to wait for initialization. */
    if (fail_init) return CURLE_FAILED_INIT;
    atomic_store(&initialized, true);
    return CURLE_OK;
}
CURL *curl_easy_init(void) {
    assert(atomic_load(&initialized));
    atomic_fetch_add(&creates, 1);
    return (CURL *) calloc(1, sizeof(mock_curl));
}
CURLcode curl_easy_setopt(CURL *curl, CURLoption opt, ...) {
    mock_curl *h = (mock_curl *) curl;
    va_list args;
    va_start(args, opt);
    switch (opt) {
        case CURLOPT_WRITEFUNCTION: h->write = va_arg(args, curl_write_callback); break;
        case CURLOPT_WRITEDATA: h->data = va_arg(args, void *); break;
        case CURLOPT_POSTFIELDS:
            h->post_fields = va_arg(args, const char *); h->post = 1; break;
        case CURLOPT_HTTPHEADER: h->headers = va_arg(args, struct curl_slist *); break;
        case CURLOPT_POST: h->post = va_arg(args, long); break;
        case CURLOPT_NOSIGNAL: h->nosignal = va_arg(args, long); break;
        default: break;
    }
    va_end(args);
    return CURLE_OK;
}
CURLcode curl_easy_perform(CURL *curl) {
    mock_curl *h = (mock_curl *) curl;
    assert(h->nosignal == 1 && h->write && h->data);
    if (h->post) assert(h->headers && h->post_fields);
    if (response != CURLE_OK) return response;
    HTTP_DATA *data = h->data;
    if (write_case == 1) {
        assert(h->write("x", SIZE_MAX, 2, data) == 0);
        assert(data->size == 0);
        return CURLE_WRITE_ERROR;
    }
    if (write_case == 2) {
        data->size = SIZE_MAX - 2;
        assert(h->write("abcd", 1, 4, data) == 0);
        data->size = 0;
        return CURLE_WRITE_ERROR;
    }
    assert(h->write("abc", 1, 3, data) == 3);
    assert(h->write("def", 1, 3, data) == 3);
    return CURLE_OK;
}
CURLcode curl_easy_getinfo(CURL *curl, CURLINFO info, ...) {
    (void) curl;
    assert(info == CURLINFO_RESPONSE_CODE);
    va_list args;
    va_start(args, info);
    *va_arg(args, long *) = response == CURLE_HTTP_RETURNED_ERROR ? 500L : 200L;
    va_end(args);
    return CURLE_OK;
}
void curl_easy_cleanup(CURL *curl) {
    mock_curl *h = (mock_curl *) curl;
    assert(!h->data && !h->headers && !h->post_fields);
    atomic_fetch_add(&destroys, 1);
    free(h);
}
const char *curl_easy_strerror(CURLcode code) { (void) code; return "mock failure"; }
struct curl_slist *curl_slist_append(struct curl_slist *list, const char *str) {
    assert(list == NULL);
    struct curl_slist *item = calloc(1, sizeof(*item));
    item->data = strdup(str);
    return item;
}
void curl_slist_free_all(struct curl_slist *list) {
    free(list->data); free(list);
}
int gs_set_error(int code, const char *fmt, ...) { (void) fmt; return code; }
void commons_log_printf(commons_log_level level, const char *tag, const char *fmt, ...) {
    (void) level; (void) tag; (void) fmt;
}
void commons_log_hexdump(commons_log_level level, const char *tag, const void *data, size_t size) {
    (void) level; (void) tag; (void) data; (void) size;
}

static void exercise(HTTP *http) {
    HTTP_DATA *data = http_data_alloc();
    assert(data && data->memory[0] == '\0' && data->size == 0);
    for (int i = 0; i < 100; i++) {
        assert(http_post_json(http, "http://fixture/", "{}", data) == GS_OK);
        assert(data->size == 6 && strcmp(data->memory, "abcdef") == 0);
        assert(http_request(http, "http://fixture/", data) == GS_OK);
        assert(data->size == 6 && strcmp(data->memory, "abcdef") == 0);
    }
    response = CURLE_HTTP_RETURNED_ERROR;
    assert(http_request(http, "http://fixture/", data) == GS_FAILED);
    assert(http_post_json(http, "http://fixture/", "{}", data) == GS_FAILED);
    response = CURLE_COULDNT_CONNECT;
    assert(http_request(http, "http://fixture/", data) == GS_IO_ERROR);
    response = CURLE_OK;
    for (write_case = 1; write_case <= 2; write_case++) {
        assert(http_request(http, "http://fixture/", data) == GS_IO_ERROR);
    }
    write_case = 0;
    assert(http_request(http, "http://fixture/", data) == GS_OK);
    http_data_free(data);
}
static void *worker(void *shared) {
    HTTP *http = shared ? shared : http_create("/fixture");
    if (fail_init) { assert(http == NULL); return NULL; }
    assert(http);
    exercise(http);
    if (!shared) http_destroy(http);
    return NULL;
}
int main(int argc, char **argv) {
    (void) argv;
    alarm(30);
    fail_init = argc > 1;
    pthread_t threads[8];
    for (int i = 0; i < 8; i++) assert(pthread_create(&threads[i], NULL, worker, NULL) == 0);
    for (int i = 0; i < 8; i++) pthread_join(threads[i], NULL);
    assert(atomic_load(&init_calls) == 1);
    if (fail_init) { assert(atomic_load(&creates) == 0); puts("HTTP init failure passed"); return 0; }
    HTTP *shared = http_create("/fixture");
    for (int i = 0; i < 8; i++) assert(pthread_create(&threads[i], NULL, worker, shared) == 0);
    for (int i = 0; i < 8; i++) pthread_join(threads[i], NULL);
    http_destroy(shared);
    assert(atomic_load(&creates) == atomic_load(&destroys));
    puts("HTTP lifecycle, concurrent initialization/requests, status ABI and buffer bounds passed");
}
