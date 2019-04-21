#ifdef _MSC_VER
#pragma warning(push, 1)
#ifndef _STL_WARNING_LEVEL
#define _STL_WARNING_LEVEL 3 /* Avoid warnings inside nasty MSVC STL code */
#endif
#pragma warning(disable : 4548) /* expression before comma has no effect;      \
                                   expected expression with side - effect */
#pragma warning(disable : 4530) /* C++ exception handler used, but unwind      \
                                   semantics are not enabled. Specify /EHsc */
#pragma warning(disable : 4577) /* 'noexcept' used with no exception handling  \
                                   mode specified; termination on exception    \
                                   is not guaranteed. Specify /EHsc */
#endif                          /* _MSC_VER (warnings) */
