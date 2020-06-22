/*!
\copyright  Copyright (c) 2008 - 2018 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\brief      Header file for logging macro's

    Two styles of logging are supported.

    Both styles can be accessed from the pydbg tool using the 
    log commands. The two forms of logging will be displayed
    in the order they were logged. Although the two styles of logging 
    can be combined, the recommendation is to use the DEBUG_LOG()
    functions.

    It is recommended to use the following macros for logging so that the
    amount of information displayed can be changed at run-time:
        DEBUG_LOG_ERROR(...)
        DEBUG_LOG_WARN(...)
        DEBUG_LOG_INFO(...)
        DEBUG_LOG_DEBUG(...)
        DEBUG_LOG_VERBOSE(...)
        DEBUG_LOG_V_VERBOSE(...)
    By default the amount of information logged at run time is controlled by
    the global variable \c debug_log_level__global. It is initialised at boot
    to \c DEFAULT_LOG_LEVEL which is normally \c DEBUG_LOG_LEVEL_INFO so any
    INFO, WARNING and ERROR messages will be displayed. That can be changed
    within this file.

    If required, the level can
    be controlled per module by defining the macro ac DEBUG_LOG_MODULE_NAME
    just before the include of <logging.h> and adding a line
    "DEBUG_LOG_DEFINE_LEVEL_VAR" in the C file. E.g. for main.c this would
    look something like:

        #include <boot.h>
        #include <os.h>
        #define DEBUG_LOG_MODULE_NAME main
        #include <logging.h>
        #include <app/message/system_message.h>

        DEBUG_LOG_DEFINE_LEVEL_VAR

        appTaskData globalApp;

    That would create a global variable \c debug_log_level_main which can
    be set at run-time to control the level of messages output by the
    statements in this file. Some modules may include the \c logging.h via
    a module private header. In that case they would have the define of the
    module name in the private header just before the include of \c logging.h
    and have the \c DEBUG_LOG_DEFINE_LEVEL_VAR statement in one of the C files
    of the module.

    All of these macros are enabled by default, but logging can be disabled 
    by use of the define DISABLE_LOG. Defining or undefining this 
    at the top of a particular source file would allow logging to be completely
    disabled at compile-time on a module basis.

    The per-module level logging can be disabled by defining
    \c DISABLE_PER_MODULE_LOG_LEVELS which will save the data memory
    consumed by the global variables for each module.

    The log levels can be disabled globally by defining
    \c DISABLE_DEBUG_LOG_LEVELS which will cause all the debug statements
    of \c DEFAULT_LOG_LEVEL (normally \c DEBUG_LOG_LEVEL_INFO) and more
    important to always produce output and the less important levels to be
    ignored at build time. This will save some code size from the
    comparison with the log level on each debug statement and from removing
    the less important levels.

    \note The DEBUG_LOG() macros write condensed information
    to a logging area and <b>can only be decoded if the original 
    application image file (.elf) is available</b>.

    This macro is quicker since the format string isn't parsed by the
    firmware and by virtue of being condensed there
    is less chance of losing information if monitoring information
    in real time.

    The DEBUG_PRINT() macros write the complete string to a different 
    logging area, a character buffer, and can be decoded even if the 
    application image file is not available.

    The use of DEBUG_PRINT() is not recommended apart from, for instance,
    printing the contents of a message or buffer. Due to memory
    constraints the available printf buffer is relatively small
    and information can be lost. When paramaters are used the DEBUG_PRINT()
    functions can also use a lot of memory on the software stack which can cause
    a Panic() due to a stack overflow.
*/


#ifndef LOGGING_H
#define LOGGING_H

#include <hydra_log.h>

#if defined(DESKTOP_BUILD) || !defined(INSTALL_HYDRA_LOG)
#include <stdio.h>
#endif

#ifndef LOGGING_EXCLUDE_SEQUENCE
#define EXTRA_LOGGING_STRING        "%04X: "
#define EXTRA_LOGGING_NUM_PARAMS    1
#define EXTRA_LOGGING_PARAMS        , globalDebugLineCount++ 
#else
#define EXTRA_LOGGING_STRING
#define EXTRA_LOGGING_NUM_PARAMS
#define EXTRA_LOGGING_PARAMS
#endif /*  LOGGING_EXCLUDE_SEQUENCE */


/*! \cond internals 

    This is some cunning macro magic that is able to count the
    number of arguments supplied to DEBUG_LOG

    If the arguments were "Hello", 123, 456 then VA_NARGS returns 2

                Hello 123 456 16  15  14  13  12  11  10   9   8   7   6   5   4   3  2  1 0 _bonus
    VA_NARGS_IMPL(_a, _b, _c, _d, _e, _f, _g, _h, _i, _j, _k, _l, _m, _n, _o, _p, _q, N, ...) N


 */
#define VA_NARGS_IMPL(_a, _b, _c, _d, _e, _f, _g, _h, _i, _j, _k, _l, _m, _n, _o, _p, _q, N, ...) N
#define VA_NARGS(...) VA_NARGS_IMPL(__VA_ARGS__, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, _bonus_as_no_ellipsis)

    /* Equivalent macro that just indicates if there are SOME arguments 
       after the format string, or NONE 

       if the argument is just "hello" then VA_ANY_ARGS_IMPL returns _NONE, which is 
       used to form a macro REST_OF_ARGS_NONE

                      hello _S, _S, _S, _S, _S, _S, _S, _S, _S, _S, _S, _S, _S, _S, _S, _S, _NONE, _bonus
       VA_ANY_ARGS_IMPL(_a, _b, _c, _d, _e, _f, _g, _h, _i, _j, _k, _l, _m, _n, _o, _p, _q, SN, ...) SN

       */

#define VA_ANY_ARGS_IMPL(_a, _b, _c, _d, _e, _f, _g, _h, _i, _j, _k, _l, _m, _n, _o, _p, _q, SN, ...) SN
#define VA_ANY_ARGS(...) VA_ANY_ARGS_IMPL(__VA_ARGS__, _SOME, _SOME, _SOME, _SOME, _SOME, _SOME, _SOME, _SOME, _SOME, _SOME, _SOME, _SOME, _SOME, _SOME, _SOME, _SOME, _NONE, _bonus_as_no_ellipsis)

    /* Retrieve just the format string */
#define FMT(fmt,...) fmt

    /* macros that return additional parameters past the format string 
        The _NONE variant is needed as (fmt, ...) will not match if just passed "hello"
     */
#define REST_OF_ARGS_NONE(fmt) 
#define REST_OF_ARGS_SOME(fmt,...) ,__VA_ARGS__
#define _MAKE_REST_OF_ARGS(a,b) a##b
#define MAKE_REST_OF_ARGS(_num) _MAKE_REST_OF_ARGS(REST_OF_ARGS,_num)


extern void hydra_log_firm_variadic(const char *event_key, size_t n_args, ...);
/*! \endcond internals */

extern uint16 globalDebugLineCount;

typedef enum {
    DEBUG_LOG_LEVEL_ERROR,
    DEBUG_LOG_LEVEL_WARN,
    DEBUG_LOG_LEVEL_INFO,
    DEBUG_LOG_LEVEL_DEBUG,
    DEBUG_LOG_LEVEL_VERBOSE,
    DEBUG_LOG_LEVEL_V_VERBOSE,
} debug_log_level_t;

#define DEBUG_LOG_WITH_LEVEL _DEBUG_LOG_L

#define DEBUG_LOG_ERROR(...)        _DEBUG_LOG_L(DEBUG_LOG_LEVEL_ERROR,     __VA_ARGS__)
#define DEBUG_LOG_WARN(...)         _DEBUG_LOG_L(DEBUG_LOG_LEVEL_WARN,      __VA_ARGS__)
#define DEBUG_LOG_INFO(...)         _DEBUG_LOG_L(DEBUG_LOG_LEVEL_INFO,      __VA_ARGS__)
#define DEBUG_LOG_DEBUG(...)        _DEBUG_LOG_L(DEBUG_LOG_LEVEL_DEBUG,     __VA_ARGS__)
#define DEBUG_LOG_VERBOSE(...)      _DEBUG_LOG_L(DEBUG_LOG_LEVEL_VERBOSE,   __VA_ARGS__)
#define DEBUG_LOG_V_VERBOSE(...)    _DEBUG_LOG_L(DEBUG_LOG_LEVEL_V_VERBOSE, __VA_ARGS__)
#define DEBUG_LOG   DEBUG_LOG_VERBOSE
#define DEBUG_LOG_ALWAYS DEBUG_LOG_ERROR
#define DEBUG_LOG_STATE DEBUG_LOG_INFO
#define DEBUG_LOG_FN_ENTRY DEBUG_LOG_DEBUG


/*! Level at which logging defaults on boot. All messages with
 * this level or higher (numerically lower) will appear in the
 * log. The value can be changed on-the-fly by changing either
 * the global log level \c debug_log_level__global or a module
 * level variable where it exists (see below).
 */
#define DEFAULT_LOG_LEVEL           DEBUG_LOG_LEVEL_INFO

#define UNUSED0(a)                      (void)(a)
#define UNUSED1(a,b)                    (void)(a),UNUSED0(b)
#define UNUSED2(a,b,c)                  (void)(a),UNUSED1(b,c)
#define UNUSED3(a,b,c,d)                (void)(a),UNUSED2(b,c,d)
#define UNUSED4(a,b,c,d,e)              (void)(a),UNUSED3(b,c,d,e)
#define UNUSED5(a,b,c,d,e,f)            (void)(a),UNUSED4(b,c,d,e,f)
#define UNUSED6(a,b,c,d,e,f,g)          (void)(a),UNUSED5(b,c,d,e,f,g)
#define UNUSED7(a,b,c,d,e,f,g,h)        (void)(a),UNUSED6(b,c,d,e,f,g,h)
#define UNUSED8(a,b,c,d,e,f,g,h,i)      (void)(a),UNUSED7(b,c,d,e,f,g,h,i)
#define UNUSED9(a,b,c,d,e,f,g,h,i,j)    (void)(a),UNUSED8(b,c,d,e,f,g,h,i,j)

#define ALL_UNUSED_IMPL_(nargs) UNUSED ## nargs
#define ALL_UNUSED_IMPL(nargs) ALL_UNUSED_IMPL_(nargs)

#define DEBUG_LOG_NOT_USED_WITH_LEVEL(level, ...) ALL_UNUSED_IMPL(VA_NARGS(__VA_ARGS__)) (__VA_ARGS__)
#define DEBUG_LOG_NOT_USED(...) DEBUG_LOG_NOT_USED_WITH_LEVEL(DEFAULT_LOG_LEVEL, __VA_ARGS__)


#ifndef DISABLE_LOG

#if !defined(DESKTOP_BUILD) && defined(INSTALL_HYDRA_LOG)

#if defined(DEBUG_LOG_MODULE_NAME) && !defined(DISABLE_PER_MODULE_LOG_LEVELS) \
                                    && !defined(DISABLE_DEBUG_LOG_LEVELS)
/*! Macros to declare a variable to hold the log level for this module
 * The define of \c DEBUG_LOG_MODULE_NAME must occur in the header or
 * source file before the include of this file for these macros to work.
 */
#define __LOG_LEVEL_SYMBOL_FOR_NAME(name)   debug_log_level_##name
#define _LOG_LEVEL_SYMBOL_FOR_NAME(name)    __LOG_LEVEL_SYMBOL_FOR_NAME(name)
#define LOG_LEVEL_CURRENT_SYMBOL            _LOG_LEVEL_SYMBOL_FOR_NAME(DEBUG_LOG_MODULE_NAME)

/*! Macro used in a module to define the variable that holds its log level */
#define DEBUG_LOG_DEFINE_LEVEL_VAR          debug_log_level_t LOG_LEVEL_CURRENT_SYMBOL = DEFAULT_LOG_LEVEL;
#else
#define LOG_LEVEL_CURRENT_SYMBOL        debug_log_level__global

/* If per module log levels are disabled then define the statement to nothing
 * in case the module normally does support its own level */
#define DEBUG_LOG_DEFINE_LEVEL_VAR
#endif

/*! Declare a variable for the local log level. This will be either
 * a module defined log level or the global one. */
extern debug_log_level_t LOG_LEVEL_CURRENT_SYMBOL;

/*! Declare a variable for the local log level. This is always global level. */
extern debug_log_level_t debug_log_level__global;


/*! \brief  Unconditionally display the supplied string in the condensed log.
  Not intended to be used directly. Instead use the DEBUG_LOG_ERROR,
  DEBUG_LOG_WARN etc. macros.
    \param fmt  String to display in the log
 */
#define _DEBUG_LOG_UNCONDITIONAL(...) \
            do { \
                HYDRA_LOG_STRING(log_fmt, EXTRA_LOGGING_STRING FMT(__VA_ARGS__,bonus_arg)); \
                hydra_log_firm_variadic(log_fmt, VA_NARGS(__VA_ARGS__) + EXTRA_LOGGING_NUM_PARAMS EXTRA_LOGGING_PARAMS MAKE_REST_OF_ARGS(VA_ANY_ARGS(__VA_ARGS__))(__VA_ARGS__)); \
            } while (0)

/*! \brief  Display the supplied string in the condensed log
  Not intended to be used directly. Instead use the DEBUG_LOG_ERROR,
  DEBUG_LOG_WARN etc. macros.
    \param level Level from the enum DEBUG_LOG_LEVEL_ENUM to specify the
        severity of the message
    \param fmt  String to display in the log
 */
#ifndef DISABLE_DEBUG_LOG_LEVELS
#define _DEBUG_LOG_L(level, ...) \
            do { \
                if(level<=LOG_LEVEL_CURRENT_SYMBOL) \
                { \
                    _DEBUG_LOG_UNCONDITIONAL(__VA_ARGS__); \
                } \
            } while (0)
#else  /* DISABLE_DEBUG_LOG_LEVELS */
/*! If log levels are not enabled then assume all messages of DEFAULT_LOG_LEVEL
 * or more important should be shown */
#define _DEBUG_LOG_L(level, ...) \
            do { \
                if(level<=DEFAULT_LOG_LEVEL) \
                { \
                    _DEBUG_LOG_UNCONDITIONAL(__VA_ARGS__); \
                } \
            } while (0)
#endif /* DISABLE_DEBUG_LOG_LEVELS */

#else   /* DESKTOP_BUILD */
/* Hydra log on desktop builds just runs using printf */
#define _DEBUG_LOG_L(level, ...) \
            printf(FMT(__VA_ARGS__,bonus_arg) "\n" MAKE_REST_OF_ARGS(VA_ANY_ARGS(__VA_ARGS__))(__VA_ARGS__))

/* No per-module log levels */
#define DEBUG_LOG_DEFINE_LEVEL_VAR
#endif  /* DESKTOP_BUILD */

/*! \brief  Include a string, without parameters in the 
        character debug buffer

    See the description in the file as to why the use of this
    function is not recommended.

    \param fmt  String to display in the log
 */
#define DEBUG_PRINT(...) \
        printf(__VA_ARGS__)

#else   /* DISABLE_LOG */

#define _DEBUG_LOG_L(level, ...) ALL_UNUSED_IMPL(VA_NARGS(__VA_ARGS__)) (__VA_ARGS__ )
#define DEBUG_PRINT(...) ALL_UNUSED_IMPL(VA_NARGS(__VA_ARGS__)) (__VA_ARGS__ )

/* No per-module log levels */
#define DEBUG_LOG_DEFINE_LEVEL_VAR

#endif  /* DISABLE_LOG */

/*! Macro to print an unsigned long long in a DEBUG_LOG statement. 

    Usage: DEBUG_LOG("Print unsigned long long 0x%08lx%08lx", PRINT_ULL(unsigned long long))
*/
#define PRINT_ULL(x)   ((uint32)(((x) >> 32) & 0xFFFFFFFFUL)),((uint32)((x) & 0xFFFFFFFFUL))

#endif /* LOGGING_H */
