
#include "kernel/types.h"
#include "kernel/pstat.h"
#include "user/user.h"

static struct pstat ps;

char *
state_name(int state)
{
  switch (state) {
  case 0: return "UNUSED";
  case 1: return "USED";
  case 2: return "SLEEPING";
  case 3: return "RUNNABLE";
  case 4: return "RUNNING";
  case 5: return "ZOMBIE";
  default: return "UNKNOWN";
  }
}

void
print_processes(void)
{
  if (getpinfo(&ps) < 0) {
    printf("ps_xv6: getpinfo failed\n");
    return;
  }

  printf("\nPID  PPID  STATE      MEM(KB)  TICKS  CPU%%  SCHED  AGE  NAME\n");
  for (int i = 0; i < ps.count; i++) {
    struct pinfo *p = &ps.procs[i];

    uint age = ps.now - p->ctime;
    uint mem = p->sz / 1024;
    uint cpu = age ? (p->ticks * 100) / age : 0;

    printf("%d    %d     %s    %d       %d      %d     %d      %d    %s\n",
       p->pid,
       p->ppid,
       state_name(p->state),
       mem,
       p->ticks,
       cpu,
       p->nsched,
       age,
       p->name);
  }
}

int
main(int argc, char *argv[])
{
  if (argc == 1) {
    print_processes();
    exit(0);
  }

  if (argc == 3 && strcmp(argv[1], "spin") == 0) {
    int n = atoi(argv[2]);

    printf("Generating CPU load for %d iterations...\n", n);

    for (int i = 0; i < n; i++)
      ;

    print_processes();
    exit(0);
  }

  if (argc == 3 && strcmp(argv[1], "watch") == 0) {
  int n = atoi(argv[2]);

  for (int i = 0; i < n; i++) {
    print_processes();
  }

  exit(0);
}

  printf("Usage: ps_xv6 [spin N | watch N]\n");
  exit(1);
}
