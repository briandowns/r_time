/*-
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2020 Brian J. Downs
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <ctype.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <curl/curl.h>

#include "r_time.h"

#define ENDPOINT_COUNT 9
#define MIN_RESPONSES  3

#define HTTP_UPPER_HEADER  "Date: " // e.g. Date: Sat, 11 Apr 2020 17:42:26 GMT
#define HTTP_LOWER_HEADER  "date: " // e.g. date: Sat, 11 Apr 2020 17:42:26 GMT
#define HTTP_HEADER_FORMAT "%a, %d %b %Y %H:%M:%S %Z"

const char* const endpoints[ENDPOINT_COUNT] = {
	"https://facebook.com", 
    "https://microsoft.com", 
    "https://amazon.com", 
    "https://google.com",
	"https://youtube.com", 
    "https://twitter.com", 
    "https://reddit.com", 
    "https://netflix.com",
	"https://bing.com"
};

/**
 * endpoint
 */
struct endpoint {
    pthread_mutex_t mu;
    unsigned int idx;
    char *url;
    time_t timestamp;
};

/**
 * endpoint_new creates a new pointer to an endpoint
 * structure. The caller is responsible for freeing
 * the used memory. This can be done by calling the 
 * endpoint_free function.
 */
struct endpoint*
endpoint_new(const unsigned int idx, const char *url)
{
    if (idx < 0 || idx > ENDPOINT_COUNT) {
        return NULL;
    }
    struct endpoint *e = malloc(sizeof(struct endpoint));
    e->idx = idx;
    e->url = malloc(strlen(url)+1);
    strcpy(e->url, url);
    pthread_mutex_init(&e->mu, NULL);
    return e;
}

/**
 * endpoint_free frees the memory used
 * by the given endpoint pointer.
 */
static void
endpoint_free(struct endpoint *e)
{
    if (e == NULL) {
        return;
    }
    if (e->url != NULL) {
        free(e->url);
    }

    pthread_mutex_destroy(&e->mu);
}

/**
 * trim_whitespace
 */
static char*
trim_whitespace(char *str)
{
    while(isspace((unsigned char)*str)) {
        str++;
    }

    if (*str == 0) {
        return str;
    }

    char *end = str + strlen(str) - 1;
    while(end > str && isspace((unsigned char)*end)) {
        end--;
    }

    end[1] = '\0';

    return str;
}

/**
 * slice_str
 */
static void
slice_str(const char *str, char *buffer, size_t start, size_t end)
{
    size_t j = 0;

    for (size_t i = start; i <= end; ++i) {
        buffer[j++] = str[i];
    }

    buffer[j] = 0;
}

/**
 * header_callback
 */
static size_t 
header_callback(char *buffer, size_t size, size_t nitems, void *userdata)
{
    struct endpoint *e = (struct endpoint*)userdata;
    //trim_whitespace(buffer);

    if (strstr(buffer, HTTP_UPPER_HEADER) != 0 || strstr(buffer, HTTP_LOWER_HEADER) != 0) {
        pthread_mutex_lock(&e->mu);

        trim_whitespace(buffer);
        size_t buffer_len = strlen(buffer);
        char date[buffer_len];
        slice_str(buffer, date, 6, strlen(buffer));

        struct tm tm;
        strptime(date, HTTP_HEADER_FORMAT, &tm);
        e->timestamp = mktime(&tm);
        printf("url: %s - timestamp: %ld\n", e->url, e->timestamp);
        pthread_mutex_unlock(&e->mu);
    }

    return nitems * size;
}

/**
 * pull_one_url
 */
static void*
pull_one_url(void *data)
{
    CURL *curl = curl_easy_init();

    struct endpoint *e = (struct endpoint*)data;

    curl_easy_setopt(curl, CURLOPT_URL, e->url);
    curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, e);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);
    curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    return NULL;
}

void
r_time_init() 
{
    curl_global_init(CURL_GLOBAL_ALL);
}

/**
 * endpoint_timestamp_count
 */
static unsigned int
endpoint_timestamp_count(struct endpoint *res[ENDPOINT_COUNT])
{
    unsigned int count = 0;
    for (int i = 0; i < ENDPOINT_COUNT; i++) {
        if (res[i]->timestamp > 0) {
            count++;
        }
    }
    return count;
}

time_t
r_time_now() {
    pthread_t tid[ENDPOINT_COUNT];

    struct endpoint *res[ENDPOINT_COUNT] = {0};
    
    for (int i = 0; i < ENDPOINT_COUNT; i++) {
        struct endpoint *e = endpoint_new(i, endpoints[i]);
        res[i] = e;
        
        if (pthread_create(&tid[i], NULL, pull_one_url, (void *)e) != 0) {
            perror("failed to create thread");
        }
    }

    for (int i = 0; i < ENDPOINT_COUNT; i++) {
        pthread_join(tid[i], NULL);
    }

    
    // make sure we have at leaet 3 responses to process
    if (endpoint_timestamp_count(res) >= MIN_RESPONSES) {
        time_t smallest = res[0]->timestamp;
        for (int i = 0; i < ENDPOINT_COUNT; i++) {
            if (res[i]->timestamp < smallest) {
                smallest = res[i]->timestamp;
            }
            endpoint_free(res[i]);
        } 
        return smallest;
    } else {
        time_t now;
        time(&now);
        return now;
    }
}

