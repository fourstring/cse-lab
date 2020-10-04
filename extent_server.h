// this is the extent server

#ifndef extent_server_h
#define extent_server_h

#include <string>
#include <map>
#include "extent_protocol.h"
#include "inode_manager.h"
#include "Directory.h"




class extent_server {
protected:
#if 0
    typedef struct extent {
      std::string data;
      struct extent_protocol::attr attr;
    } extent_t;
    std::map <extent_protocol::extentid_t, extent_t> extents;
#endif
    inode_manager *im;

public:
    extent_server();

    int create(uint32_t type, extent_protocol::extentid_t &id);

    int put(extent_protocol::extentid_t id, std::string, int &);

    int get(extent_protocol::extentid_t id, std::string &);

    int getattr(extent_protocol::extentid_t id, extent_protocol::attr &);

    int remove(extent_protocol::extentid_t id, int &);

    int lookup(extent_protocol::extentid_t parent, const std::string &filename, bool &found, uint32_t &inum);

    int create_file(extent_protocol::extentid_t parent, const std::string &filename, uint32_t type, uint32_t &new_inum);

    int unlink(extent_protocol::extentid_t parent, const std::string &link_name);

    int readdir(extent_protocol::extentid_t id, std::list<extent_dirent> &entries);

    Directory get_dir(extent_protocol::extentid_t id);
};

#endif 







