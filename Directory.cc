//
// Created by fourstring on 2020/9/20.
//

#include "Directory.h"
#include <cstring>
#include <vector>

// Caller should guarantee bytes[0] is the start byte of an entry
DirectoryEntry::DirectoryEntry(char *bytes) {
    auto *u32buf = reinterpret_cast<uint32_t *>(bytes);
    filename_length = u32buf[0];
    auto *filename_start = bytes + sizeof(uint32_t);
    filename = std::string{filename_start, filename_length};
    auto *file_inum_buf = reinterpret_cast<uint32_t * >(filename_start + filename_length);
    file_inum = file_inum_buf[0];
}

std::unique_ptr<char[]> DirectoryEntry::to_bytes() const {
    auto buf = std::unique_ptr<char[]>{new char[size_of_bytes()]};
    auto *filename_length_buf = reinterpret_cast<uint32_t *>(buf.get());
    filename_length_buf[0] = filename_length;
    auto *filename_buf = reinterpret_cast<char *>(buf.get() + sizeof(uint32_t));
    std::memcpy(filename_buf, filename.c_str(), filename_length);
    auto *file_inum_buf = reinterpret_cast<uint32_t *>(filename_buf + filename_length);
    file_inum_buf[0] = file_inum;
    return buf;
}

inline size_t DirectoryEntry::size_of_bytes() const {
    return 2 * sizeof(uint32_t) + filename_length;
}

DirectoryEntry::DirectoryEntry(size_t filenameLength, const std::string &filename, uint32_t fileInum) : filename_length(
        filenameLength), filename(filename), file_inum(fileInum) {

}

Directory::Directory(inode_manager *im, uint32_t inum) : im{im}, inum{inum} {
    read_directory();
}

void Directory::read_directory() {
    char *_dir_buf = nullptr;
    int size = 0;
    im->read_file(inum, &_dir_buf, &size);
    auto dir_buf = std::unique_ptr<char[]>{_dir_buf};
    for (int read_byte = 0; read_byte < size;) {
        auto dir_entry = DirectoryEntry{_dir_buf + read_byte};
        read_byte += dir_entry.size_of_bytes();
        filenames.insert({dir_entry.filename, dir_entry.file_inum});
    }
}

void Directory::sync_directory() {
    auto dir_entries = std::vector<DirectoryEntry>{};
    dir_entries.reserve(filenames.size());
    auto total_bytes = 0;
    for (const auto &filename:filenames) {
        auto entry = DirectoryEntry{filename.first.length(), filename.first, filename.second};
        dir_entries.push_back(entry);
        total_bytes += entry.size_of_bytes();
    }

    auto buf = std::unique_ptr<char[]>(new char[total_bytes]);
    auto entry = dir_entries.begin();
    for (int wrote_bytes = 0; wrote_bytes < total_bytes; entry++) {
        auto *entry_buf = buf.get() + wrote_bytes;
        auto entry_bytes = entry->to_bytes();
        std::memcpy(entry_buf, entry_bytes.get(), entry->size_of_bytes());
        wrote_bytes += entry->size_of_bytes();
    }

    im->write_file(inum, buf.get(), total_bytes);
}

uint32_t Directory::filename_to_inum(const std::string &filename) {
    auto file_pair = filenames.find(filename);
    if (file_pair != filenames.end()) {
        return file_pair->second;
    }
    return 0;
}

uint32_t Directory::create_file(const std::string &filename, uint32_t type) {
    auto file_inum = im->alloc_inode(type);
    im->increase_ref(file_inum);

    filenames.insert({filename, file_inum});
    sync_directory();

    return file_inum;
}

void Directory::link(const std::string &link_name, uint32_t to_inum) {
    auto link_pair = filenames.find(link_name);
    if (link_pair == filenames.end()) {
        filenames.insert({link_name, to_inum});
    } else {
        filenames[link_name] = to_inum;
    }
    sync_directory();
}

void Directory::unlink(const std::string &link_name) {
    auto link_pair = filenames.find(link_name);
    if (link_pair == filenames.end()) {
        return;
    }
    im->decrease_ref(link_pair->second);
    filenames.erase(link_pair);
    sync_directory();
}

void Directory::record_change() {
    change_counter++;
    if (change_counter >= CHANGE_THRESHOLD) {
        sync_directory();
        change_counter = 0;
    }
}

std::map<std::string, uint32_t>::const_iterator Directory::cbegin() {
    return filenames.cbegin();
}

std::map<std::string, uint32_t>::const_iterator Directory::cend() {
    return filenames.cend();
}
