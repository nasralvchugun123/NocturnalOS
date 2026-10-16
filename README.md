
```
NocturnalOS
├─ CMakeLists.txt
├─ Fusion
│  ├─ CMakeLists.txt
│  ├─ dwm
│  │  ├─ shared.h
│  │  └─ types.h
│  ├─ fusion
│  │  ├─ shared.h
│  │  └─ types.h
│  ├─ fusion.cmake
│  ├─ intinit
│  │  ├─ builtin
│  │  │  └─ commands
│  │  │     ├─ Avaible.txt
│  │  │     ├─ as.c
│  │  │     ├─ cat.c
│  │  │     ├─ cd.c
│  │  │     ├─ dlltest.c
│  │  │     ├─ echo.c
│  │  │     ├─ edit.c
│  │  │     ├─ exit.c
│  │  │     ├─ find.c
│  │  │     ├─ help.c
│  │  │     ├─ kill.c
│  │  │     ├─ ls.c
│  │  │     ├─ mkfile.c
│  │  │     ├─ operands.c
│  │  │     ├─ reboot.c
│  │  │     ├─ shutdown.c
│  │  │     └─ ver.c
│  │  ├─ color.h
│  │  ├─ commandlist.c
│  │  ├─ init
│  │  │  └─ entry.c
│  │  ├─ logtracing.c
│  │  ├─ scroll.c
│  │  ├─ services.c
│  │  ├─ shutdown.c
│  │  └─ terminalstyle.c
│  └─ ser.c
├─ NTloader
│  ├─ CMakeLists.txt
│  ├─ Legacyboot
│  │  └─ .keep
│  ├─ PE_loader
│  │  ├─ PE_loader.c
│  │  └─ PE_loader.h
│  └─ UEFI
│     ├─ boot.c
│     ├─ uefi.c
│     └─ uefi.h
├─ README.md
├─ base
│  ├─ CMakeLists.txt
│  ├─ Sys
│  │  ├─ CMakeLists.txt
│  │  ├─ NTDLL
│  │  │  ├─ CMakeLists.txt
│  │  │  ├─ NTdll.c
│  │  │  ├─ NTdll.h
│  │  │  ├─ ldr.c
│  │  │  ├─ ldr_imports.c
│  │  │  └─ ldr_pe.c
│  │  ├─ csrss
│  │  │  └─ csr.c
│  │  └─ services
│  │     └─ srv.c
│  └─ app
│     ├─ CMakeLists.txt
│     ├─ systest
│     │  ├─ dlltest.c
│     │  └─ ringtest.c
│     └─ tests
│        ├─ CMakeLists.txt
│        ├─ testdll
│        │  ├─ CMakeLists.txt
│        │  ├─ testdll.c
│        │  └─ testdll.def
│        └─ testload
│           ├─ CMakeLists.txt
│           └─ testload.c
├─ cmake
│  ├─ NocturnalOS.cmake
│  ├─ iso.cmake
│  └─ sysroot.cmake
├─ drivers
│  ├─ fs
│  │  ├─ GPT
│  │  │  ├─ gpt.c
│  │  │  └─ gpt.h
│  │  ├─ fat
│  │  │  └─ fat32
│  │  │     ├─ fat32.c
│  │  │     └─ fat32.h
│  │  └─ vfs
│  │     ├─ vfs.c
│  │     └─ vfs.h
│  └─ storage
│     └─ nvme
│        ├─ nvme.c
│        └─ nvme.h
├─ hal
│  ├─ CMakeLists.txt
│  └─ halx64
│     ├─ Apic
│     │  ├─ Apic.h
│     │  ├─ acpi.h
│     │  ├─ ioapic.c
│     │  ├─ lapic.c
│     │  ├─ madt.c
│     │  ├─ smp_boot.c
│     │  └─ trampoline.S
│     ├─ Init
│     │  ├─ hal_init.c
│     │  ├─ hal_irq.c
│     │  ├─ hal_irq.h
│     │  ├─ pic.c
│     │  └─ pic.h
│     └─ PCI
│        ├─ pci.c
│        └─ pci.h
├─ ntoskrnl
│  ├─ CMakeLists.txt
│  ├─ cc
│  │  ├─ file.c
│  │  ├─ filecache.c
│  │  ├─ filelarping
│  │  │  ├─ id.c
│  │  │  └─ recall.c
│  │  ├─ fs.c
│  │  ├─ fuckfat.c
│  │  ├─ inc
│  │  │  ├─ file.h
│  │  │  ├─ filecache.h
│  │  │  ├─ fs.h
│  │  │  ├─ fuckfat.h
│  │  │  ├─ id.h
│  │  │  └─ recall.h
│  │  └─ mem.c
│  ├─ dbg
│  │  └─ print
│  │     ├─ kdprint.c
│  │     └─ kdprint.h
│  ├─ dll
│  │  ├─ dll.c
│  │  ├─ dll.h
│  │  ├─ dllsys.c
│  │  ├─ imgsec.c
│  │  ├─ imgsec.h
│  │  ├─ loader.c
│  │  └─ loader.h
│  ├─ ds
│  │  └─ fb
│  │     ├─ fb.c
│  │     └─ fb.h
│  ├─ hal_stub.c
│  ├─ io
│  │  ├─ IRP.h
│  │  ├─ device.c
│  │  ├─ dispatch.c
│  │  ├─ driver.c
│  │  ├─ io.h
│  │  ├─ io_const.h
│  │  ├─ io_init.c
│  │  ├─ io_types.h
│  │  ├─ irp.c
│  │  └─ pnprel
│  ├─ ke
│  │  ├─ arch
│  │  │  ├─ except.h
│  │  │  └─ ke_arch.h
│  │  ├─ clock.c
│  │  ├─ clock.h
│  │  ├─ exept.c
│  │  ├─ exept.h
│  │  ├─ fatal.c
│  │  ├─ fatal.h
│  │  ├─ freez.c
│  │  ├─ freez.h
│  │  ├─ kbd.c
│  │  ├─ kbd.h
│  │  ├─ kbdinput.c
│  │  ├─ kbdmap.c
│  │  ├─ kcpu.c
│  │  ├─ kcpu.h
│  │  ├─ ke.h
│  │  ├─ ke_init.c
│  │  ├─ processr.c
│  │  ├─ processr.h
│  │  ├─ shell.c
│  │  ├─ shell.h
│  │  ├─ spinlock.c
│  │  ├─ spinlock.h
│  │  ├─ time.c
│  │  ├─ time.h
│  │  ├─ wait.c
│  │  ├─ wait.h
│  │  └─ x64
│  │     ├─ except.c
│  │     ├─ gdt.c
│  │     ├─ gdt.h
│  │     ├─ idt.c
│  │     ├─ idt.h
│  │     ├─ isr.S
│  │     ├─ msr.h
│  │     ├─ switch.S
│  │     ├─ syscall.c
│  │     ├─ syscall_entry.S
│  │     └─ usrmod.S
│  ├─ ki
│  │  ├─ Ki_init.c
│  │  ├─ ki_ap.c
│  │  ├─ pe_user.c
│  │  └─ pe_user.h
│  ├─ mm
│  │  ├─ memory
│  │  │  ├─ mem.c
│  │  │  ├─ mm_init.c
│  │  │  ├─ pmm.c
│  │  │  ├─ pmm.h
│  │  │  ├─ vmm.c
│  │  │  └─ vmm.h
│  │  └─ mm.h
│  ├─ ntoskrnl.h
│  ├─ ob
│  │  ├─ Devlink.c
│  │  ├─ Init.c
│  │  ├─ devices.c
│  │  ├─ directory.c
│  │  ├─ handle.c
│  │  ├─ obcache.c
│  │  ├─ obname.c
│  │  ├─ obsec.c
│  │  ├─ reference.c
│  │  ├─ shared.h
│  │  └─ wait.c
│  ├─ po
│  │  ├─ shit.c
│  │  └─ shit.h
│  ├─ ps
│  │  ├─ allc.c
│  │  ├─ def.c
│  │  ├─ ps.h
│  │  ├─ ps_init.c
│  │  ├─ thread.c
│  │  ├─ while.c
│  │  └─ while.h
│  ├─ sched
│  │  ├─ al
│  │  │  ├─ FCFS
│  │  │  ├─ Priority
│  │  │  ├─ RR
│  │  │  │  ├─ mp
│  │  │  │  │  └─ sched.c
│  │  │  │  └─ sp
│  │  │  │     └─ sched.c
│  │  │  └─ SJF 
│  │  ├─ job
│  │  ├─ mp
│  │  └─ task
│  └─ vfs
│     ├─ vfs.h
│     └─ vfs_init.c
├─ sdk
│  ├─ CMakeLists.txt
│  ├─ RTL
│  │  ├─ format.h
│  │  ├─ mem.h
│  │  ├─ rt.h
│  │  └─ string.h
│  ├─ inc
│  │  ├─ boot_info.h
│  │  ├─ font8x16.h
│  │  ├─ font8x8.h
│  │  ├─ nt
│  │  │  ├─ atomic.h
│  │  │  ├─ ntdefs.h
│  │  │  ├─ ntdll.h
│  │  │  ├─ ntstatus.h
│  │  │  ├─ pe.h
│  │  │  └─ syscall.h
│  │  └─ syscall
│  │     └─ syscall.s
│  └─ lib
│     ├─ crt
│     │  ├─ dllmain.c
│     │  ├─ errno.c
│     │  ├─ init.c
│     │  ├─ libc
│     │  │  ├─ standart
│     │  │  │  ├─ errno.h
│     │  │  │  ├─ stdint.h
│     │  │  │  ├─ stdio.h
│     │  │  │  ├─ stdlib.h
│     │  │  │  └─ string.h
│     │  │  ├─ stdio
│     │  │  │  ├─ printf.c
│     │  │  │  ├─ putchar.c
│     │  │  │  ├─ puts.c
│     │  │  │  ├─ sprintf.c
│     │  │  │  └─ vsprintf.c
│     │  │  ├─ stdlib
│     │  │  │  ├─ abort.c
│     │  │  │  ├─ atoi.c
│     │  │  │  ├─ exit.c
│     │  │  │  ├─ free.c
│     │  │  │  └─ malloc.c
│     │  │  └─ str
│     │  │     ├─ memcmp.c
│     │  │     ├─ memcpy.c
│     │  │     ├─ memmove.c
│     │  │     ├─ memset.c
│     │  │     ├─ strchr.c
│     │  │     ├─ strcmp.c
│     │  │     ├─ strcpy.c
│     │  │     ├─ strlen.c
│     │  │     ├─ strncmp.c
│     │  │     ├─ strncpy.c
│     │  │     └─ strrchr.c
│     │  └─ start.c
│     ├─ ntdll
│     │  ├─ con.c
│     │  ├─ dll.c
│     │  ├─ file.c
│     │  ├─ internal.h
│     │  ├─ memory.c
│     │  ├─ object.c
│     │  ├─ proc.c
│     │  └─ sync.c
│     └─ rt
│        ├─ format.c
│        ├─ mem.c
│        ├─ string.c
│        └─ unicode.c
├─ toolchain-Linux.cmake
└─ tools
   ├─ cmmdutils
   │  └─ psf2c
   │     ├─ psf2c
   │     └─ psf2c.c
   └─ scriptutils

```