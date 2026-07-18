#include <algo/algo.hpp>
#include <fmt/fmt.hpp>
#include <fs/procfs/proc_self.hpp>
#include <log/log.hpp>
#include <memory/memory.hpp>
#include <process/process.hpp>

namespace fs::procfs {

ProcSelfInode::ProcSelfInode(MountPoint* mp, Inode* parent, int ino)
    : ReadOnlyInode{mp}
{
    this->type = FileType::CHAR_DEVICE;
    this->parent = parent;
    this->ino = ino;
}

int ProcSelfInode::read(FileDescriptor*, void* buf, std::size_t count)
{
    process::Process* proc = arch::percpu::current_process();
    kstring cwd = fs::getcwd(proc->cwd_inode);

    kstring format = "pid = {}\n"
                     "cwd = {}\n"
                     "state = {}\n"
                     "pml4         @ {}\n"
                     "kernel stack @ {}\n"
                     "kernel rsp   @ {}\n"
                     "entry rip    @ {}\n"
                     "context switch count = {}";

    kstring str = fmt::sprintf(
        format,
        proc->pid,
        cwd,
        proc->get_state_str(),
        fmt::hex{proc->pml4},
        fmt::hex{proc->kernel_stack},
        fmt::hex{proc->kernel_rsp},
        fmt::hex{proc->entry},
        proc->context_switches);

    kcopy_to_user(buf, str.data(), algo::min(str.length(), count));

    return str.length();
}

}
