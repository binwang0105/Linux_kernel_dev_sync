cmd_arch/arm/kernel/built-in.o :=  arm-eabi-ld -EL    -r -o arch/arm/kernel/built-in.o arch/arm/kernel/elf.o arch/arm/kernel/entry-armv.o arch/arm/kernel/entry-common.o arch/arm/kernel/irq.o arch/arm/kernel/opcodes.o arch/arm/kernel/process.o arch/arm/kernel/ptrace.o arch/arm/kernel/return_address.o arch/arm/kernel/sched_clock.o arch/arm/kernel/setup.o arch/arm/kernel/signal.o arch/arm/kernel/stacktrace.o arch/arm/kernel/sys_arm.o arch/arm/kernel/time.o arch/arm/kernel/traps.o arch/arm/kernel/sleep.o arch/arm/kernel/suspend.o arch/arm/kernel/unwind.o arch/arm/kernel/pmu.o arch/arm/kernel/io.o 