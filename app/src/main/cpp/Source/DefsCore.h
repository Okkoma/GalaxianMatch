#pragma once

#ifdef GALAXIANMATCH_EXPORTS
#  if defined(__WIN32__) || defined(__WINRT__)
#    define GALAXIANMATCH_API __declspec(dllexport)
#  elif defined(__GNUC__) && __GNUC__ >= 4
#    define GALAXIANMATCH_API __attribute__ ((visibility("default")))
#  endif
#else
#   define GALAXIANMATCH_API
#endif

