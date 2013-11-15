#ifndef macro_h
#define macro_h

/*
Useful preprocessor macros.
*/

#define CONCATENATE_DIRECT(s1, s2) s1##s2
#define CONCATENATE(s1, s2) CONCATENATE_DIRECT(s1, s2)

#define ANONYMOUS_VARIABLE(s) CONCATENATE(s, __LINE__)

#define STRINGIFY_DIRECT(s) #s
#define STRINGIFY(s) STRINGIFY_DIRECT(s)

#define HERE __FILE__ "(" STRINGIFY(__LINE__) ")"

#endif
