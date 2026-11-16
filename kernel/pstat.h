#include "types.h"

#ifndef _PSTAT_H_
#define _PSTAT_H_

#define PSTAT_MAX 64

struct pinfo {
  int pid;
  int ppid;
  int state;
  uint ticks;
  uint nsched;
  uint ctime;
  uint64 sz;
  char name[16];
};

struct pstat {
  int count;
  uint now;
  struct pinfo procs[PSTAT_MAX];
};

#endif
