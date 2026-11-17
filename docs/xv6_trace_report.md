# xv6 RISC-V System Call Trace Report
## M5 — `getyear()` System Call

**Project:** MiniOS  
**Component:** xv6 Kernel (M5)  
**System Call:** `getyear()`  
**System Call Number:** 23  
**Expected Return Value:** 2026  
**Architecture:** RISC-V 64-bit

---

## 1. Objective

The objective of this experiment is to trace the execution of the `getyear()` system call in xv6, from its invocation by a user-space program to its execution inside the kernel and its eventual return to user mode.

GDB was used to inspect the system call dispatch mechanism, kernel execution flow, trapframe, register values, and trampoline instructions.

The experiment verifies that:
- The system call is invoked using the correct system call number.
- The kernel correctly dispatches the request to `sys_getyear()`.
- The system call returns the expected value, 2026.
- The return value is stored in the process trapframe.
- The trampoline restores the return value into the user-mode register before executing `sret`.

## 2. System Call Implementation

The system call number is defined in `kernel/syscall.h`:

```c
#define SYS_getyear 23
```

The kernel implementation is in `kernel/sysproc.c`:

```c
uint64
sys_getyear(void)
{
    return 2026;
}
```

The syscall dispatcher in `kernel/syscall.c` invokes the corresponding handler and stores its return value in the process trapframe:

```c
p->trapframe->a0 = syscalls[num]();
```

The RISC-V calling convention uses register `a7` to identify the system call and register `a0` to carry its return value.

## 3. Tools and Environment

- Operating system: CachyOS Linux
- Debugger: GDB
- Emulator: QEMU
- Architecture: RISC-V 64-bit
- Kernel: xv6-riscv
- Test program: `getyeartest`

QEMU was started in debugging mode using:

```bash
make qemu-gdb
```

The QEMU command exposed the GDB server on port `26000`.

GDB was started using:

```bash
gdb -q kernel/kernel
```

The debugger was connected using:

```gdb
target remote 127.0.0.1:26000
```

## 4. Execution Trace

### Stage 1: User-space system call invocation

The user program invokes `getyear()` through the generated system call stub.

The stub loads the system call number into register `a7` and executes the environment call instruction:

```asm
li a7, 23
ecall
```

The `ecall` instruction transfers execution from user mode to the kernel.

### Stage 2: Trap handling

The trap is handled through the trampoline and `usertrap()`.

The call stack observed in GDB was:

```text
#0 sys_getyear () at kernel/sysproc.c:27
#1 syscall () at kernel/syscall.c:150
#2 usertrap () at kernel/trap.c:68
#3 0x3ffffff09c in ?? ()
```

This confirms that execution reached `sys_getyear()` through the expected syscall dispatch path.

### Stage 3: System call identification

A breakpoint was placed at the system call handler:

```gdb
break sys_getyear
continue
```

GDB stopped at:

```text
Thread 3 hit Breakpoint 1, sys_getyear ()
```

The registers were inspected:

```gdb
info registers pc a7
```

Observed output:

```text
pc    0x80002b14    0x80002b14 <sys_getyear>
a7    0x17          23
```

**Observation:** Register `a7` contains hexadecimal `0x17`, which is decimal 23. This confirms that the kernel received the expected system call number.

### Stage 4: System call execution

The `sys_getyear()` function returns the integer value `2026`.

GDB was used to execute the function until it returned:

```gdb
finish
```

Observed output:

```text
Value returned is $1 = 2026
```

The return register was inspected:

```gdb
info registers a0 pc
```

Observed output:

```text
a0    0x7ea    2026
pc    0x80002ab0    0x80002ab0 <syscall+58>
```

**Observation:** The return value is correctly placed in register `a0`. Hexadecimal `0x7ea` represents decimal `2026`.

### Stage 5: Trapframe verification

The syscall dispatcher stores the handler's return value in the process trapframe:

```c
p->trapframe->a0 = syscalls[num]();
```

After stepping over this assignment, the trapframe was inspected using:

```gdb
next
p/x myproc()->trapframe->a0
```

Observed output:

```text
usertrap () at kernel/trap.c:81
$3 = 0x7ea
```

**Observation:** The process trapframe contains `0x7ea`, confirming that the kernel saved the system call return value for restoration to user mode.

### Stage 6: Trampoline register restoration

The trampoline's `userret` routine restores the saved user registers.

Its mapped virtual address was inspected using:

```gdb
break *0x3ffffff09c
continue
```

The following instructions were observed:

```asm
fence.i
sfence.vma
csrw satp,a0
sfence.vma
```

These instructions synchronize instruction execution and switch to the user process's page table.

The remaining instructions restore the saved registers from the trapframe. The final two instructions are:

```asm
ld a0,112(a0)
sret
```

The `ld a0,112(a0)` instruction restores the saved user-mode `a0` register. The `sret` instruction returns execution to user mode.

### Stage 7: Verification immediately before returning to user mode

A breakpoint was placed at the `sret` instruction:

```gdb
break *0x3ffffff120
continue
```

GDB stopped at:

```text
Thread 3 hit Breakpoint 2, 0x0000003ffffff120 in ?? ()
```

The registers were inspected:

```gdb
info registers pc a0
```

Observed output:

```text
pc    0x3ffffff120    0x3ffffff120
a0    0x7ea           2026
```

**Observation:** Immediately before executing `sret`, register `a0` contains `2026`. This confirms that the trampoline restored the correct system call return value.

## 5. Summary of GDB Observations

| Verification | Observed value | Result |
|---|---|---|
| System call number (`a7`) | `0x17` = 23 | Passed |
| Handler return value | 2026 | Passed |
| Return register (`a0`) | `0x7ea` = 2026 | Passed |
| Trapframe `a0` | `0x7ea` | Passed |
| PC before `sret` | `0x3ffffff120` | Passed |
| Restored user `a0` | `0x7ea` = 2026 | Passed |

## 6. System Call Execution Flow

The complete execution flow is:

```text
User program: getyeartest
        |
        v
getyear() system call stub
        |
        | a7 = 23
        | ecall
        v
trampoline.S: uservec
        |
        v
trap.c: usertrap()
        |
        v
syscall.c: syscall()
        |
        | Identify syscall number 23
        v
sysproc.c: sys_getyear()
        |
        | Return 2026
        v
syscall.c: store result in trapframe->a0
        |
        v
trampoline.S: userret
        |
        | Restore user registers
        v
ld a0,112(a0)
        |
        v
sret
        |
        v
User mode resumes with a0 = 2026
```

## 7. Conclusion

The GDB tracing experiment successfully verified the execution of the `getyear()` system call in xv6.

The system call number was correctly identified as 23, the kernel handler returned 2026, and the return value was stored in the process trapframe.

The trampoline subsequently restored the value into register `a0` before executing `sret`.

Therefore, the experiment verifies the complete system call return-value path from kernel execution through trapframe restoration to the user-mode return boundary.