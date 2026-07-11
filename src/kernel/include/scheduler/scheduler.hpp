#pragma once

#include <cstdint>
#include <process/process.hpp>

namespace scheduler {
void init();

// Called directly by apic_timer_handler, not via timer::register_handler —
// it may context_switch() into a different process and not return for an
// arbitrary time, which the generic per-tick handler list does not allow.
void preempt();

void wake_single(process::WaitReason wait_reason);
void wake_all(process::WaitReason wait_reason);

[[noreturn]]
void yield_dead();

[[noreturn]]
void yield_zombie();

void yield_sleep(std::uint64_t sleep_time_ms);

void yield_blocked(process::WaitReason wait_reason);

int yield_to_child(int pid);

void add_process(process::Process* p);
}
