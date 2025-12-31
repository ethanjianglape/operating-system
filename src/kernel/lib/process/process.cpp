#include "arch/x86_64/process/process.hpp"
#include "arch/x86_64/memory/vmm.hpp"
#include <cstddef>
#include <cstdint>
#include <process/process.hpp>
#include <process/elf.hpp>
#include <log/log.hpp>
#include <arch.hpp>
#include <crt/crt.h>

namespace process {
    constexpr std::uintptr_t USER_STACK_BASE = 0x00800000;
    constexpr std::size_t    USER_SACK_SIZE  = 16 * 1024; // 16KiB
    constexpr std::uintptr_t USER_STACK_TOP  = USER_STACK_BASE + USER_SACK_SIZE;
    
    void load_elf(std::uint8_t* buffer, std::size_t size) {
        elf::Elf64_File file = elf::parse_file(buffer, size);

        if (!file.is_valid_elf) {
            return;
        }

        for (const elf::Elf64_ProgramHeader& header : file.program_headers) {
            auto virt = header.p_vaddr;
            auto size = header.p_filesz;
            auto offset = header.p_offset;

            std::size_t code_pages = arch::vmm::map_mem_at(virt, size, arch::vmm::PAGE_USER);

            memcpy((void*)virt, (void*)(buffer + offset), size);
        }

        std::size_t stack_pages = arch::vmm::map_mem_at(USER_STACK_BASE, USER_SACK_SIZE, arch::vmm::PAGE_USER);

        arch::process::enter_userspace(file.entry, USER_STACK_TOP);
    }
    
    void load(std::uint8_t* buffer, std::size_t size) {
        if (buffer == nullptr) {
            log::error("Attmpt to load program at NULL");
            return;
        }
        
        load_elf(buffer, size);
    }
}
