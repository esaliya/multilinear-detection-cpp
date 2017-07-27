//
// Created by Saliya Ekanayake on 7/9/17.
//

#ifndef CLIONCPP_RECV_VERTEX_BUFFER_HPP
#define CLIONCPP_RECV_VERTEX_BUFFER_HPP


#include "vertex_buffer.hpp"

class recv_vertex_buffer : public vertex_buffer{
public:
  recv_vertex_buffer(int offset_factor, std::shared_ptr<short> buffer, int recvfrom_rank,
                     int msg_size_offset) : vertex_buffer(offset_factor, buffer){
    this->recvfrom_rank=recvfrom_rank;
    this->msg_size_offset=msg_size_offset;
  }

  ~recv_vertex_buffer(){}

  int get_msg_size(){
    return buffer.get()[msg_size_offset];
  }

  int get_recvfrom_rank(){
    return recvfrom_rank;
  }
private:
  int recvfrom_rank;
  int msg_size_offset;
};


#endif //CLIONCPP_RECV_VERTEX_BUFFER_HPP
