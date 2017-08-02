//
// Created by Saliya Ekanayake on 7/8/17.
//

#include <algorithm>

#ifndef CLIONCPP_MESSAGE_HPP
#define CLIONCPP_MESSAGE_HPP

#endif //CLIONCPP_MESSAGE_HPP

class message{
public:
  message(){}
  ~message(){}

  void copy(std::shared_ptr<short> buffer, int offset, int data_idx, int data_length){
    // data is such that all data_idx elements of data_length are nearby elements
    std::copy(data.get()+data_idx, data.get()+data_idx+data_length, &buffer.get()[offset]);
  }
  // NOTE - let's not use this method, see above
  /*void copy(std::shared_ptr<short> buffer, int offset, int msg_size){
    short* b = buffer.get();
    std::copy(data.get(), data.get()+msg_size, &b[offset]);
  }*/

  void load(std::shared_ptr<short> buffer, int offset, int recvd_msg_size){
    msg_size = recvd_msg_size;
    read_offset = offset;
    this->buffer = buffer;
  }
  // NOTE - let's reduce the data transmitted, see the above method
  /*void load(std::shared_ptr<short> buffer, int offset, int recvd_msg_size){
    msg_size = recvd_msg_size;
    dim_a = buffer.get()[offset];
    read_offset = offset+1;
    this->buffer = buffer;
  }*/

  short get(int i){
    return buffer.get()[read_offset+i];
  }

  int get_msg_size(){
    return msg_size;
  }

  void set_data_and_msg_size(std::shared_ptr<short> data, int msgSize) {
    this->data = data;
    this->msg_size = msgSize;
  }
  // NOTE - let's not store dimension as it's always 1 for kpath. See above method
  /*void set_data_and_msg_size(std::shared_ptr<short> data, int msgSize) {
    this->data = data;
    // +1 to store dimension
    this->msg_size = msgSize+1;
    dim_a = (short) msgSize;
  }*/


private:
  int msg_size;
  std::shared_ptr<short> data = nullptr;
  int read_offset;
  std::shared_ptr<short> buffer;

};