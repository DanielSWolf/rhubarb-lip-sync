#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <whereami.h>

#if defined(_WIN32)

#define WIN32_LEAN_AND_MEAN
#if defined(_MSC_VER)
#pragma warning(push, 3)
#endif
#include <windows.h>

#define RTLD_LAZY 1
#define RTLD_NOW 2
#define RTLD_GLOBAL 4
#define RTLD_LOCAL 8

static void* dlopen(const char* fileName, int mode)
{
  wchar_t buffer[MAX_PATH];

  if (MultiByteToWideChar(CP_UTF8, 0, fileName, -1, buffer, sizeof(buffer) / sizeof(*buffer)))
  {
    wchar_t buffer_[MAX_PATH];

    GetFullPathNameW(buffer, sizeof(buffer_) / sizeof(*buffer_), buffer_, NULL);

    return (void*)LoadLibraryW(buffer_);
  }

  return NULL;
}

static int dlclose(void* handle)
{
  return FreeLibrary((HMODULE)handle) ? 0 : -1;
}

static const char* dlerror(void)
{
  DWORD error;

  error = GetLastError();

  if (error)
  {
    static char message[1024];

    FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, error, MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT), message, sizeof(message), NULL);

    return message;
  }

  return "no error";
}

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

#else

#include <dlfcn.h>

#endif

int main(int argc, char** argv)
{
  char* path = NULL;
  int length, dirname_length;
  int i;

  length = wai_getExecutablePath(NULL, 0, &dirname_length);
  if (length > 0)
  {
    path = (char*)malloc(length + 1);
    if (!path)
      abort();
    wai_getExecutablePath(path, length, &dirname_length);
    path[length] = '\0';

    printf("executable path: %s\n", path);
    path[dirname_length] = '\0';
    printf("  dirname: %s\n", path);
    printf("  basename: %s\n", path + dirname_length + 1);
    free(path);
  }

  length = wai_getModulePath(NULL, 0, &dirname_length);
  if (length > 0)
  {
    path = (char*)malloc(length + 1);
    if (!path)
      abort();
    wai_getModulePath(path, length, &dirname_length);
    path[length] = '\0';

    printf("module path: %s\n", path);
    path[dirname_length] = '\0';
    printf("  dirname: %s\n", path);
    printf("  basename: %s\n", path + dirname_length + 1);
    free(path);
  }

  for (i = 1; i < argc; ++i)
  {
    if (strncmp(argv[i], "--load-library=", 15) == 0)
    {
      char* name = argv[i] + 15;
      void* handle;

      printf("\n");

      handle = dlopen(name, RTLD_NOW);
      if (!handle)
        printf("failed to load library: %s\n", dlerror());

      if (handle)
        dlclose(handle);
    }
  }

  return 0;
}
