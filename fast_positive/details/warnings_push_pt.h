#ifdef _MSC_VER
#pragma warning(push)
#if _MSC_VER < 1900
#pragma warning(disable : 4350) /* behavior change: 'std::_Wrap_alloc... */
#endif
#if _MSC_VER > 1913
#pragma warning(disable : 5045) /* will insert Spectre mitigation... */
#endif
#pragma warning(                                                               \
    disable : 4505) /* unreferenced local function has been removed */
#pragma warning(disable : 4514) /* 'xyz': unreferenced inline function         \
                                               has been removed */
#pragma warning(disable : 4710) /* 'xyz': function not inlined */
#pragma warning(disable : 4711) /* function 'xyz' selected for                 \
                                   automatic inline expansion */
#pragma warning(disable : 4061) /* enumerator 'abc' in switch of enum 'xyz' is \
                                   not explicitly handled by a case label */
#pragma warning(disable : 4201) /* nonstandard extension used :                \
                                   nameless struct / union */
#pragma warning(disable : 4127) /* conditional expression is constant */
#pragma warning(disable : 4702) /* unreachable code */

#pragma warning(disable : 4661) /* no suitable definition provided for         \
                                   explicit template instantiation request*/

#pragma warning(disable : 4251) /* class 'FOO' needs to have dll-interface     \
                                 * a to be used by clients of class 'BAR'      \
                                 * (FALSE-POSITIVE in the MOST CASES). */
#endif                          /* _MSC_VER (warnings) */
