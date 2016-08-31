#include <stdlib.h>
#include <stdio.h>

#include <whereami.h>

#if defined(__GNUC__) && !defined(_WIN32)
__attribute__((constructor))
#endif
static void load()
{
  char* path = NULL;
  int length, dirname_length;

  printf("library loaded\n");

  length = wai_getExecutablePath(NULL, 0, &dirname_length);
  if (length > 0)
  {
    path = (char*)malloc(length + 1);
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
    wai_getModulePath(path, length, &dirname_length);
    path[length] = '\0';

    printf("module path: %s\n", path);
    path[dirname_length] = '\0';
    printf("  dirname: %s\n", path);
    printf("  basename: %s\n", path + dirname_length + 1);
    free(path);
  }
}

#if defined(__GNUC__) && !defined(_WIN32)
__attribute__((constructor))
__attribute__((destructor))
#endif
static void unload()
{
  printf("library unloaded\n");
}

#if defined(_WIN32)

#define WIN32_LEAN_AND_MEAN
#if defined(_MSC_VER)
#pragma warning(push, 3)
#endif
#include <windows.h>

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
  switch (ul_reason_for_call)
  {
    case DLL_PROCESS_ATTACH:
      load();
      break;
    case DLL_THREAD_ATTACH:
      break;
    case DLL_THREAD_DETACH:
      break;
    case DLL_PROCESS_DETACH:
      unload();
      break;
  }
  return TRUE;
}

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

#endif
