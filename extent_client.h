// extent client interface.

#ifndef extent_client_h
#define extent_client_h

#include <string>
#include "extent_protocol.h"
#include "extent_server.h"

class extent_client {
 private:
  rpcc *cl;

 public:
  extent_client(std::string dst);

    extent_protocol::status create(uint32_t type, extent_protocol::extentid_t &eid);

    extent_protocol::status get(extent_protocol::extentid_t eid,
                                std::string &buf);

    extent_protocol::status getattr(extent_protocol::extentid_t eid,
                                    extent_protocol::attr &a);

    extent_protocol::status put(extent_protocol::extentid_t eid, std::string buf);

    extent_protocol::status remove(extent_protocol::extentid_t eid);

    extent_protocol::status
    lookup(extent_protocol::extentid_t parent, const std::string &filename, bool &found, uint32_t &inum);

    extent_protocol::status
    create_file(extent_protocol::extentid_t parent, const std::string &filename, uint32_t type, uint32_t &new_inum);

    extent_protocol::status unlink(extent_protocol::extentid_t parent, const std::string &link_name);

    extent_protocol::status readdir(extent_protocol::extentid_t id, std::list<extent_dirent> &entries);

    Directory get_dir(extent_protocol::extentid_t eid);
};

#endif