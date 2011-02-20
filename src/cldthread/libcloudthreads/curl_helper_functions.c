#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <curl/curl.h>

#include "curl_helper_functions.h"

/* MemoryWrite example taken from http://curl.haxx.se/libcurl/c/getinmemory.html */

size_t WriteMemoryCallback(void *ptr, size_t size, size_t nmemb, void *data){

    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)data;

    mem->memory = realloc(mem->memory, mem->size + realsize + 1);

    if (mem->memory == NULL) {
        /* out of memory! */
        perror("not enough memory (realloc returned NULL)\n");
        exit(EXIT_FAILURE);
    }

    memcpy(&(mem->memory[mem->size]), ptr, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;

}


size_t ReadMemoryCallback(void *ptr, size_t size, size_t nmemb, void *data){

    size_t bytecount = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)data;

    /* Make sure that we don't read any more than we're supposed to... */
    if(bytecount > (mem->size-mem->offset)) bytecount = mem->size-mem->offset;

    memcpy(ptr, &(mem->memory[mem->offset]), bytecount);
    mem->offset += bytecount;

    return bytecount;

}
