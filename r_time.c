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

#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <curl/curl.h>

#include "r_time.h"

#define ENDPOINT_COUNT 9

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
    char *timestamp;
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
    strcpy(e->url, url);
    pthread_mutex_init(&e->mu, NULL);
    return e;
}

/**
 * endpoint_free frees the memory used
 * by the given endpoint pointer.
 */
void
endpoint_free(struct endpoint *e)
{
    if (e == NULL) {
        return;
    }
    pthread_mutex_destroy(&e->mu);
}

/**
 * header_callback
 */
static size_t 
header_callback(char *buffer, size_t size, size_t nitems, void *userdata)
{
    struct endpoint *e = (struct endpoint*)userdata;

    if (strstr(buffer, "Date:") != NULL || strstr(buffer, "date:") != NULL) {
        printf("found! - %s", buffer);
        pthread_mutex_lock(&e->mu);
        e->timestamp = malloc(strlen(buffer)+1);
        strcpy(e->timestamp, buffer);
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
    printf("here\n");
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

time_t*
r_time_now() {
    pthread_t tid[ENDPOINT_COUNT];

    struct endpoint *res[ENDPOINT_COUNT] = {0};
    
    for (int i = 0; i < ENDPOINT_COUNT; i++) {
        struct endpoint *e = endpoint_new(i, endpoints[i]);
        //res[i] = e;
        //printf("url: %s, idx: %d\n", e->url, e->idx);
        
        int err = pthread_create(&tid[i], NULL, pull_one_url, (void *)e);
        if (err != 0) {
            fprintf(stderr, "Couldn't run thread number %d, errno %d\n", i, err);
        }
    }

    /* now wait for all threads to terminate */ 
    for (int i = 0; i < ENDPOINT_COUNT; i++) {
        pthread_join(tid[i], NULL);
        fprintf(stderr, "Thread %d terminated\n", i);
    }

    for (int i = 0; i < ENDPOINT_COUNT; i++) { 
        printf("index: %d, URL: %s, result: %s\n", res[i]->idx, res[i]->url, res[i]->timestamp);
    } 

    return NULL;
}

