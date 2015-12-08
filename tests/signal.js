/*
=!EXPECTSTART!=
[ "SIGHUP", "SIGINT", "SIGQUIT", "SIGILL", "SIGTRAP", "SIGABRT", "SIGBUS", "SIGFPE", "SIGKILL", "SIGUSR1", "SIGSEGV", "SIGUSR2", "SIGPIPE", "SIGALRM", "SIGTERM", "SIGCLD", "SIGCONT", "SIGSTOP", "SIGTSTP", "SIGTTIN", "SIGTTOU", "SIGURG", "SIGXCPU", "SIGXFSZ", "SIGVTALRM", "SIGPROF", "SIGWINCH", "SIGIO", "SIGPWR", "SIGSYS" ]
MYSIG
=!EXPECTEND!=
*/

puts(signal.names());
function mysig() {
  puts("MYSIG");
  exit(0);
}

i = sys.getpid();
//signal.handle('USR1');
signal.callback(mysig,'USR1');

signal.kill(sys.getpid(),'USR1');
while (true) {
   sys.update(1000);
}
