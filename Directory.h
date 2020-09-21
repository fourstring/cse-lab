//
// Created by fourstring on 2020/9/20.
//

#ifndef LAB1_DIRECTORY_H
#define LAB1_DIRECTORY_H

#include <memory>
#include <string>
#include <map>
#include "inode_manager.h"

struct DirectoryEntry {
    size_t filename_length;
    std::string filename;
    uint32_t file_inum;

    DirectoryEntry(size_t filenameLength, const std::string &filename, uint32_t fileInum);

    explicit DirectoryEntry(char *bytes);

    std::unique_ptr<char[]> to_bytes() const;

    inline size_t size_of_bytes() const;
};

class Directory {
private:
    inode_manager *im;
    uint32_t inum;
    uint32_t change_counter{0};
    std::map<std::string, uint32_t> filenames{};

    void read_directory();

    void sync_directory();

    void record_change();

public:
    Directory(inode_manager *im, uint32_t inum);

    uint32_t filename_to_inum(const std::string &filename);

    uint32_t create_file(const std::string &filename, uint32_t type);

    void link(const std::string &link_name, uint32_t to_inum);

    void unlink(const std::string &link_name);

    std::map<std::string, uint32_t>::const_iterator cbegin();

    std::map<std::string, uint32_t>::const_iterator cend();
};


#endif //LAB1_DIRECTORY_H
