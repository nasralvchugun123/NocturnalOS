add_krnl_component(hal
    SOURCES
        halx64/Init/pic.c
        halx64/Init/hal_irq.c
        halx64/Init/hal_init.c
        halx64/Apic/lapic.c
        halx64/Apic/ioapic.c
        halx64/PCI/pci.c
        halx64/Apic/madt.c
        halx64/Apic/madt.c
        halx64/Apic/smp_boot.c
        halx64/Apic/trampoline.S

    INCLUDES
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/halx64
        ${CMAKE_CURRENT_SOURCE_DIR}/halx64/Apic
        ${CMAKE_CURRENT_SOURCE_DIR}/halx64/Init
        ${CMAKE_CURRENT_SOURCE_DIR}/halx64/PCI
)

target_include_directories(hal PRIVATE
    ${CMAKE_SOURCE_DIR}/sdk
    ${CMAKE_SOURCE_DIR}/sdk/inc
    ${CMAKE_SOURCE_DIR}/sdk/RTL

    ${CMAKE_SOURCE_DIR}/ntoskrnl
    ${CMAKE_SOURCE_DIR}/ntoskrnl/ke
)