
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
  // Default: display the current process table.
  if (argc == 1) {
    print_processes();
    exit(0);
  }

  // Generate CPU load, then display the process table.
  if (argc == 3 && strcmp(argv[1], "spin") == 0) {
    int n = atoi(argv[2]);

    if (n <= 0) {
      printf("Usage: ps_xv6 spin N (N must be positive)\n");
      exit(1);
    }

    printf("Generating CPU load for %d iterations...\n", n);

    for (volatile int i = 0; i < n; i++)
      ;

    print_processes();
    exit(0);
  }

  // Display N process-table snapshots, one second apart.
  if (argc == 3 && strcmp(argv[1], "watch") == 0) {
    int n = atoi(argv[2]);

    if (n <= 0) {
      printf("Usage: ps_xv6 watch N (N must be positive)\n");
      exit(1);
    }

    for (int i = 0; i < n; i++) {
      print_processes();

      // Wait approximately one second between snapshots.
      if (i < n - 1) {
        if (pause(10) < 0) {
          printf("ps_xv6: pause failed\n");
          exit(1);
        }
      }
    }

    exit(0);
  }

  printf("Usage: ps_xv6 [spin N | watch N]\n");
  exit(1);
}
