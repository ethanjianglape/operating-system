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
    kstring str = proc->to_string();

    kcopy_to_user(buf, str.data(), algo::min(str.length(), count));

    return str.length();
}

}
