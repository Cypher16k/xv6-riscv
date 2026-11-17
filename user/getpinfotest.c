
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
  int index;

  printf("Testing getpinfo...\n");

  // Test 1: System call succeeds.
  if (getpinfo(&ps) < 0) {
    printf("FAIL: getpinfo failed\n");
    exit(1);
  }
  printf("PASS 1: getpinfo syscall succeeded\n");

  // Test 2: Process count is valid.
  if (ps.count <= 0 || ps.count > PSTAT_MAX) {
    printf("FAIL: invalid process count\n");
    exit(1);
  }
  printf("PASS 2: process count = %d\n", ps.count);

  // Test 3: System uptime is populated.
  if (ps.now == 0) {
    printf("FAIL: system tick count is zero\n");
    exit(1);
  }
  printf("PASS 3: system ticks = %d\n", ps.now);

  // Test 4: Current process is present.
  pid = getpid();
  index = find_process(pid);

  if (index < 0) {
    printf("FAIL: current process not found\n");
    exit(1);
  }
  printf("PASS 4: current process found\n");

  // Test 5: Current process is RUNNING.
  if (ps.procs[index].state != 4) {
    printf("FAIL: current process is not RUNNING\n");
    exit(1);
  }
  printf("PASS 5: current process state is RUNNING\n");

  // Test 6: Current process has allocated memory.
  if (ps.procs[index].sz == 0) {
    printf("FAIL: invalid process memory size\n");
    exit(1);
  }
  printf("PASS 6: process memory size = %d bytes\n",
         (int)ps.procs[index].sz);

  // Test 7: Process name is correct.
  if (strcmp(ps.procs[index].name, "getpinfotest") != 0) {
    printf("FAIL: incorrect process name: %s\n",
           ps.procs[index].name);
    exit(1);
  }
  printf("PASS 7: process name verified\n");

  // Test 8: Null pointer must be rejected.
  if (getpinfo((struct pstat *)0) != -1) {
    printf("FAIL: null pointer accepted\n");
    exit(1);
  }
  printf("PASS 8: null pointer rejected\n");

  // Test 9: Low, read-only user address must be rejected.
  if (getpinfo((struct pstat *)1) != -1) {
    printf("FAIL: low invalid pointer accepted\n");
    exit(1);
  }
  printf("PASS 9: low invalid pointer rejected\n");

  // Test 10: Address beyond the process size must be rejected.
  if (getpinfo((struct pstat *)0xFFFFFFFFULL) != -1) {
    printf("FAIL: above-size pointer accepted\n");
    exit(1);
  }
  printf("PASS 10: above-size pointer rejected\n");

  // Create a child process.
  child_pid = fork();

  if (child_pid < 0) {
    printf("FAIL: fork failed\n");
    exit(1);
  }

  if (child_pid == 0) {
    // Consume CPU time so accounting can be checked.
    for (volatile uint i = 0; i < 200000000; i++)
      ;

    exit(0);
  }

  // Test 11: Child appears in the process table.
  index = -1;

  for (volatile uint i = 0; i < 10000000; i++) {
    index = find_process(child_pid);

    if (index >= 0)
      break;
  }

  if (index < 0) {
    printf("FAIL: child not found\n");
    wait(0);
    exit(1);
  }
  printf("PASS 11: child process found\n");

  // Test 12: Child has the correct parent PID.
  if (ps.procs[index].ppid != pid) {
    printf("FAIL: incorrect child PPID\n");
    wait(0);
    exit(1);
  }
  printf("PASS 12: child PPID verified\n");

  // Wait until the child has been scheduled and charged runtime.
  int scheduled = 0;
  int runtime = 0;

  for (int i = 0; i < 10000; i++) {
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

  // Test 13: Scheduler accounting works.
  if (!scheduled) {
    printf("FAIL: child was never scheduled\n");
    wait(0);
    exit(1);
  }
  printf("PASS 13: child scheduling count verified\n");

  // Test 14: Runtime accounting works.
  if (!runtime) {
    printf("FAIL: child runtime is zero\n");
    wait(0);
    exit(1);
  }
  printf("PASS 14: child runtime is positive\n");

  // Reap the child.
  wait(0);

  // Test 15: Reaped child disappears from the process table.
  if (find_process(child_pid) >= 0) {
    printf("FAIL: child still present after wait\n");
    exit(1);
  }
  printf("PASS 15: child removed after wait\n");

  printf("getpinfotest: OK\n");
  exit(0);
}
