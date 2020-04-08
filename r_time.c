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

static char *results[ENDPOINT_COUNT] = {0};

static size_t header_callback(char *buffer, size_t size, size_t nitems, void *userdata)
{
    if (strstr(buffer, "Date:") != NULL || strstr(buffer, "date:") != NULL) {
        printf("found! - %s", buffer);
    }

    return nitems * size;
}

struct endpoint {
    unsigned int idx;
    char *url;
};

static void*
pull_one_url(void *data)
{
    CURL *curl = curl_easy_init();

    struct endpoint *e = (struct endpoint*)data;

    curl_easy_setopt(curl, CURLOPT_URL, e->url);
    curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
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

    char *res[ENDPOINT_COUNT] = {0};
    
    for (int i = 0; i < ENDPOINT_COUNT; i++) {
        struct endpoint *e = malloc(sizeof(struct endpoint));
        e->url = malloc(strlen(endpoints[i]) + 1);
        strcpy(e->url, endpoints[i]);
        e->idx = i;
        printf("url: %s, idx: %d\n", e->url, e->idx);
        int err = pthread_create(&tid[i], NULL, pull_one_url, (void *)e);
        if (err != 0) {
            fprintf(stderr, "Couldn't run thread number %d, errno %d\n", i, err);
        }
        else {
            fprintf(stderr, "Thread %d, gets %s\n", i, endpoints[i]);
        }
    }

    /* now wait for all threads to terminate */ 
    for (int i = 0; i < ENDPOINT_COUNT; i++) {
        pthread_join(tid[i], NULL);
        fprintf(stderr, "Thread %d terminated\n", i);
    }

    return NULL;
}

