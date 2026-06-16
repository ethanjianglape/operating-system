#include "arch/x64/percpu/percpu.hpp"
#include "containers/kstring.hpp"

#include "kpanic/kpanic.hpp"
#include "process/process.hpp"
#include <algo/algo.hpp>
#include <fs/devfs/devfs.hpp>
#include <fs/fs.hpp>
#include <log/log.hpp>

namespace fs {

static MountPointClass* g_root_mountpoint;
static kvector<MountPointClass*> g_mountpoints;

static devfs::DevFileSystem dev_fs{};

kstring canonicalize(const kstring& path)
{
    kvector<kstring> canonical;
    kvector<kstring> parts = algo::split(path, '/');

    for (const auto& part : parts) {
        if (part.empty() || part == ".") {
            continue;
        }

        if (part == "..") {
            if (!canonical.empty()) {
                canonical.pop_back();
            }
        } else {
            canonical.push_back(part);
        }
    }

    return "/" + algo::join(canonical, '/');
}

kstring getcwd(const InodeClass* inode)
{
    klist<kstring> path{};

    return "/" + algo::join(path, '/');
}

MountPointClass* find_mount_at(InodeClass* inode)
{
    for (auto* mp : g_mountpoints) {
        if (mp->mounted_on == inode) {
            return mp;
        }
    }

    return nullptr;
}

InodeClass* resolve_path(const kstring& path)
{
    if (!g_root_mountpoint) {
        kpanic("!! VFS: Root is NULL !!");
        return nullptr;
    }

    InodeClass* current = g_root_mountpoint->root();
    process::Process* proc = arch::percpu::current_process();

    if (path.front() == '/' || proc->cwd_inode == nullptr) {
        current = g_root_mountpoint->root();
    } else {
        current = proc->cwd_inode;
    }

    kvector<kstring> tokens = algo::split(path, '/');

    for (const kstring& token : tokens) {
        if (token.empty() || token == ".") {
            continue;
        }

        if (token == "..") {
            if (current == current->mountpoint->root() && current->mountpoint->mounted_on != nullptr) {
                current = current->mountpoint->mounted_on->parent;
            } else if (current->parent != nullptr) {
                current = current->parent;
            }

            continue;
        }

        InodeClass* next = current->lookup(token.c_str());

        if (!next) {
            return nullptr;
        }

        MountPointClass* mp = find_mount_at(next);

        if (mp) {
            current = mp->root();
        } else {
            current = next;
        }
    }

    return current;
}

void register_mount(const char* path, MountPointClass* mp)
{
    mp->path = path;

    if (g_root_mountpoint == nullptr) {
        log::info("VFS mount root at ", path);
        g_root_mountpoint = mp;
    }

    g_mountpoints.push_back(mp);
}

void mount(const char* path, FileSystemClass* fs, const char* source)
{
    MountPointClass* mp = fs->mount(source);
    mp->mounted_on = resolve_path(path);
    mp->root()->parent = mp->mounted_on;
    register_mount(path, mp);
}

FileDescriptor* open(const char* path, int flags)
{
    InodeClass* inode = resolve_path(path);

    if (!inode) {
        return nullptr;
    }

    auto* fd = new FileDescriptor{};

    fd->inode = inode;
    fd->flags = flags;
    fd->offset = 0;
    fd->path = path;

    return fd;
}

int stat(const kstring& path, Stat* out)
{
    InodeClass* inode = resolve_path(path);

    if (!inode) {
        return -1;
    }

    return inode->stat(out);
}

int readdir(const kstring& path, kvector<DirEntry>& out)
{
    InodeClass* inode = resolve_path(path);

    if (!inode) {
        return -1;
    }

    if (inode->type != FileType::DIRECTORY) {
        return -1;
    }

    auto* dir = static_cast<DirectoryInode*>(inode);

    return dir->readdir(out);
}

int mkdir(const kstring& path, int mode)
{
    InodeClass* inode = resolve_path(path);

    if (!inode) {
        return -1;
    }

    if (inode->type != FileType::DIRECTORY) {
        return -1;
    }

    auto* dir = static_cast<DirectoryInode*>(inode);

    return dir->mkdir(path.c_str(), mode);
}
}
