#include "algo/algo.hpp"
#include "containers/kvector.hpp"
#include "log/log.hpp"
#include <fs/fs.hpp>
#include <fs/tmpfs/tmpfs.hpp>
#include <fs/fs_file_ops.hpp>
#include <cstddef>

namespace fs::tmpfs {
    struct TmpFile {
        kstring name;
        kstring path;
        FileType type;
        std::size_t size;
        std::uint8_t* data;
        TmpFile* parent;
        klist<TmpFile*>* children;
    };

    static kvector<TmpFile*> g_tmp_files;

    static void print_files() {
        for (TmpFile* file : g_tmp_files) {
            if (file->parent) {
                log::debugf("Temp file at path={}, name={}, size={}, children={}, parent path={}", file->path, file->name, file->size, file->children->size(), file->parent->path);
            } else {
                log::debugf("Temp file at path={}, name={}, size={}, children={}. ROOT FILE", file->path, file->name, file->size, file->children->size());
            }
        }
    }

    static TmpFile* find_file_by_path(const kstring& path) {
        for (TmpFile* file : g_tmp_files) {
            if (file->path == path) {
                return file;
            }
        }
        
        return nullptr;
    }
    
    static TmpFile* create_file(const kstring& path, const kstring& name, FileType type, TmpFile* parent) {
        auto* file = new TmpFile{};

        file->name = name;
        file->path = path;
        file->type = type;
        file->size = 0;
        file->parent = parent;
        file->children = new klist<TmpFile*>{};

        g_tmp_files.push_back(file);
        
        return file;
    }

    static TmpFile* ensure_path_exists(const kstring& path) {
        kvector<kstring> parts = algo::split(path, '/');
        kstring partial_path;
        TmpFile* parent = nullptr;

        log::debug("ensuring path exists: ", path, " ", parts);

        for (const kstring& part : parts) {
            if (!partial_path.empty()) {
                partial_path += '/';
            }
            
            partial_path += part;

            log::debug("check partial path: ", partial_path);
            
            TmpFile* file = find_file_by_path(partial_path);

            if (!file) {
                file = create_file(partial_path, part, FileType::DIRECTORY, parent);

                if (parent) {
                    parent->children->push_back(file);
                }
            }

            parent = file;
        }

        print_files();

        return parent;
    }
    
    static Inode* tmpfs_open(FileSystem* fs, const kstring& path, int flags) {
        TmpFile* file = find_file_by_path(path);

        if (!file) {
            return nullptr;
        }

        auto* inode = new Inode{};
        auto* meta = new FsFileMeta{};

        meta->data = file->data;
        meta->size = file->size;

        inode->type = file->type;
        inode->size = file->size;
        inode->private_data = meta;
        inode->ops = get_fs_file_ops();
        
        return inode;
    }

    static int tmpfs_stat(FileSystem* fs, const kstring& path, Stat* stat) {
        log::debugf("tmpfs state path = {}", path);

        if (path == "") {
            stat->type = FileType::DIRECTORY;
            stat->size = 0;
            return 0;
        }

        TmpFile* file = find_file_by_path(path.trim('/'));

        if (file) {
            stat->type = file->type;
            stat->size = file->size;
            return 0;
        }
        
        return -1;
    }

    static int tmpfs_readdir(FileSystem* fs, const kstring& path, kvector<DirEntry>& out) {
        return 0;
    }

    static int tmpfs_mkdir(FileSystem* fs, const kstring& path, int mode) {
        log::debug("/tmp mkdir: " , path);

        TmpFile* parent = ensure_path_exists(path);
        
        return 0;
        
        //TmpFile* parent = ensure_path_exists(path);
        
        //return parent != nullptr ? 0 : -1;
    }
    
    static FileSystem tmpfs_fs = {
        .name = "tmpfs",
        .private_data = nullptr,
        .open = tmpfs_open,
        .stat = tmpfs_stat,
        .readdir = tmpfs_readdir,
        .mkdir = tmpfs_mkdir
    };
    
    void init() {
        fs::mount("/tmp", &tmpfs_fs);
    }
}
