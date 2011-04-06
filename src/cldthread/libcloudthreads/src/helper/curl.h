#pragma once

struct MemoryStruct {
  char *memory;
  size_t size;
  size_t offset;
};

size_t WriteMemoryCallback(void *ptr, size_t size, size_t nmemb, void *data);
size_t ReadMemoryCallback(void *ptr, size_t size, size_t nmemb, void *data);
