#ifdef KERNEL_TESTS

#include <test/test.hpp>
#include <fs/fs.hpp>
#include <log/log.hpp>

namespace test_fs {
    void test_open_existing_file() {
        fs::Inode* inode = fs::open("/bin/a", fs::O_RDONLY);
        test::assert_not_null(inode, "open existing file returns inode");

        if (inode && inode->ops && inode->ops->close) {
            fs::FileDescriptor fd = {.inode = inode, .offset = 0, .flags = fs::O_RDONLY};
            inode->ops->close(&fd);
        }
    }

    void test_open_nonexistent_file() {
        fs::Inode* inode = fs::open("/nonexistent/file/path", fs::O_RDONLY);
        test::assert_null(inode, "open nonexistent file returns nullptr");
    }

    void test_open_dev_null() {
        fs::Inode* inode = fs::open("/dev/null", fs::O_RDONLY);
        test::assert_not_null(inode, "/dev/null opens successfully");
        test::assert_eq(inode->type, fs::FileType::CHAR_DEVICE, "/dev/null is char device");
    }

    void test_dev_null_read_returns_eof() {
        fs::Inode* inode = fs::open("/dev/null", fs::O_RDONLY);
        test::assert_not_null(inode, "/dev/null opens for read test");

        if (inode && inode->ops && inode->ops->read) {
            char buf[16];
            fs::FileDescriptor fd = {.inode = inode, .offset = 0, .flags = fs::O_RDONLY};
            int result = inode->ops->read(&fd, buf, sizeof(buf));
            test::assert_eq(result, 0, "/dev/null read returns 0 (EOF)");
        }
    }

    void test_dev_null_write_succeeds() {
        fs::Inode* inode = fs::open("/dev/null", fs::O_RDONLY);
        test::assert_not_null(inode, "/dev/null opens for write test");

        if (inode && inode->ops && inode->ops->write) {
            const char* data = "test data";
            fs::FileDescriptor fd = {.inode = inode, .offset = 0, .flags = fs::O_RDONLY};
            int result = inode->ops->write(&fd, data, 9);
            test::assert_eq(result, 9, "/dev/null write returns byte count");
        }
    }

    void test_stat_existing_file() {
        fs::Stat st;
        int result = fs::stat("/bin/a", &st);
        test::assert_eq(result, 0, "stat on existing file returns 0");
        test::assert_eq(st.type, fs::FileType::REGULAR, "stat reports regular file");
        test::assert_true(st.size > 0, "stat reports non-zero size for ELF");
    }

    void test_stat_nonexistent_file() {
        fs::Stat st;
        int result = fs::stat("/nonexistent/file", &st);
        test::assert_ne(result, 0, "stat on nonexistent file returns error");
    }

    void test_stat_dev_null() {
        fs::Stat st;
        int result = fs::stat("/dev/null", &st);
        test::assert_eq(result, 0, "stat on /dev/null returns 0");
        test::assert_eq(st.type, fs::FileType::CHAR_DEVICE, "stat reports char device");
    }

    void test_readdir_root() {
        kvector<fs::DirEntry> entries;
        int result = fs::readdir("/", entries);
        test::assert_eq(result, 0, "readdir on / returns 0");
        test::assert_true(entries.size() > 0, "root has entries");
    }

    void test_readdir_bin() {
        kvector<fs::DirEntry> entries;
        int result = fs::readdir("/bin", entries);
        test::assert_eq(result, 0, "readdir on /bin returns 0");
        test::assert_true(entries.size() > 0, "/bin has entries");

        bool found_a = false;
        for (auto& entry : entries) {
            if (entry.name == "a") {
                found_a = true;
                test::assert_eq(entry.type, fs::FileType::REGULAR, "/bin/a is regular file");
                break;
            }
        }
        test::assert_true(found_a, "/bin contains file 'a'");
    }

    void test_readdir_dev() {
        kvector<fs::DirEntry> entries;
        int result = fs::readdir("/dev", entries);
        test::assert_eq(result, 0, "readdir on /dev returns 0");

        bool found_null = false;
        for (auto& entry : entries) {
            if (entry.name == "null") {
                found_null = true;
                break;
            }
        }
        test::assert_true(found_null, "/dev contains 'null'");
    }

    void test_readdir_nonexistent() {
        kvector<fs::DirEntry> entries;
        int result = fs::readdir("/nonexistent", entries);
        test::assert_ne(result, 0, "readdir on nonexistent dir returns error");
    }

    void run() {
        log::info("Running filesystem tests...");

        test_open_existing_file();
        test_open_nonexistent_file();
        test_open_dev_null();
        test_dev_null_read_returns_eof();
        test_dev_null_write_succeeds();
        test_stat_existing_file();
        test_stat_nonexistent_file();
        test_stat_dev_null();
        test_readdir_root();
        test_readdir_bin();
        test_readdir_dev();
        test_readdir_nonexistent();
    }
}

#endif // KERNEL_TESTS
