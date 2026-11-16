
#include "kernel/types.h"
#include "kernel/pstat.h"
#include "user/user.h"

static struct pstat ps;

int
find_process(int pid)
{
  if (getpinfo(&ps) < 0)
    return -1;

  for (int i = 0; i < ps.count; i++) {
    if (ps.procs[i].pid == pid)
      return i;
  }

  return -1;
}

int
main(void)
{
  int pid;
  int child_pid;

  printf("Testing getpinfo...\n");

  // Test 1: Basic syscall and process count.
  if (getpinfo(&ps) < 0) {
    printf("FAIL: getpinfo failed\n");
    exit(1);
  }

  if (ps.count <= 0 || ps.count > PSTAT_MAX) {
    printf("FAIL: invalid process count %d\n", ps.count);
    exit(1);
  }

  // Test 2: Current process must be present and RUNNING.
  pid = getpid();
  int index = find_process(pid);

  if (index < 0) {
    printf("FAIL: current process not found\n");
    exit(1);
  }

  if (ps.procs[index].state != 4) {
    printf("FAIL: current process is not RUNNING\n");
    exit(1);
  }

  printf("PASS: current process found, PID=%d\n", pid);

  // Test 3: Invalid pointer must be rejected.
  if (getpinfo((struct pstat *)0) != -1) {
    printf("FAIL: invalid pointer was accepted\n");
    exit(1);
  }

  printf("PASS: invalid pointer rejected\n");

  // Test 4: Create a child and check its process information.
  child_pid = fork();

  if (child_pid < 0) {
    printf("FAIL: fork failed\n");
    exit(1);
  }

  if (child_pid == 0) {
    // Consume CPU time so the child's runtime can be measured.
    for (volatile int i = 0; i < 100000000; i++)
      ;
    
    exit(0);
  }

  // Parent: wait until the child appears in the process table.
  index = -1;

  for (volatile int i = 0; i < 10000000; i++) {
  index = find_process(child_pid);

  if (index >= 0)
    break;
}

  if (index < 0) {
    printf("FAIL: child not found\n");
    wait(0);
    exit(1);
  }

  // Test 5: Verify parent-child relationship.
  if (ps.procs[index].ppid != pid) {
    printf("FAIL: incorrect child PPID\n");
    wait(0);
    exit(1);
  }

  printf("PASS: child PPID verified\n");

  // Test 6: Wait until the child has actually been scheduled.
int scheduled = 0;
int runtime = 0;

for (int i = 0; i < 1000; i++) {
  index = find_process(child_pid);

  if (index < 0)
    break;

  if (ps.procs[index].nsched > 0)
    scheduled = 1;

  if (ps.procs[index].ticks > 0)
    runtime = 1;

  if (scheduled && runtime)
    break;
}

if (!scheduled) {
  printf("FAIL: child scheduler count is zero\n");
  wait(0);
  exit(1);
}

printf("PASS: child scheduling count = %d\n", ps.procs[index].nsched);

if (!runtime) {
  printf("FAIL: child runtime is zero\n");
  wait(0);
  exit(1);
}

printf("PASS: child runtime = %d ticks\n", ps.procs[index].ticks);

  // Test 8: Wait for the child to exit.
  wait(0);

  // Test 9: Verify that the child disappears.
  if (find_process(child_pid) >= 0) {
    printf("FAIL: child still present after wait\n");
    exit(1);
  }

  printf("PASS: child removed after wait\n");

  printf("getpinfotest: OK\n");
  exit(0);
}
