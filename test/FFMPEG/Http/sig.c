#include <signal.h>

typedef void (*_sighandler_t)(int);

_sighandler_t signal(int sig, _sighandler_t _hander);

int sigaction(int sig, const struct sigaction *act, struct sigaction *oact);

int interrupt()

hard_reconfig

setsockopt