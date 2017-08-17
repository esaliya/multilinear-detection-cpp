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

  void copy(std::shared_ptr<short> buffer, int offset, int row_idx){
    for (int i = 0; i < iter_bs; ++i){
      int from_offset = i*dim_cols*dim_rows+row_idx*dim_cols;
      std::copy(data.get()+from_offset, data.get()+from_offset+dim_cols, &buffer.get()[offset+i*dim_cols]);
    }
  }

  void load(std::shared_ptr<short> buffer, int offset, int recvd_msg_size){
    msg_size = recvd_msg_size;
    read_offset = offset;
    this->buffer = buffer;
  }

  short get(int i, int j, int iter, int current_row){
    if (i == (current_row - 1)){
      // picking from this->buffer
      short val = buffer.get()[read_offset+(iter*dim_cols)+j];
      // copy this to recvd_opt_tbl for future use
      recvd_opt_tbl.get()[iter*dim_cols*dim_rows+i*dim_cols+j] = val;
      return val;
    } else {
      // the requested value must be in recvd_opt_tbl
      return recvd_opt_tbl.get()[iter*dim_cols*dim_rows+i*dim_cols+j];
    }
  }

  int get_msg_size(){
    return msg_size;
  }

  void set_data_and_msg_size(std::shared_ptr<short> data, int msg_size) {
    this->data = data;
    this->msg_size = msg_size;
  }
  // NOTE - let's not store dimension as it's always 1 for kpath. See above method
  /*void set_data_and_msg_size(std::shared_ptr<short> data, int msgSize) {
    this->data = data;
    // +1 to store dimension
    this->msg_size = msgSize+1;
    dim_a = (short) msgSize;
  }*/

  void init(int dim_rows, int dim_cols, int iter_bs){
    this->dim_rows = dim_rows;
    this->dim_cols = dim_cols;
    this->iter_bs = iter_bs;
  }
  void recv_init(int dim_rows, int dim_cols, int iter_bs){
    init(dim_rows, dim_cols, iter_bs);
    this->recvd_opt_tbl = std::shared_ptr<short>(
        new short[dim_cols*dim_rows*iter_bs](),
        std::default_delete<short[]>());
  }

private:
  int msg_size;
  std::shared_ptr<short> data = nullptr;

  int read_offset;
  std::shared_ptr<short> buffer;

  int dim_rows;
  int dim_cols;
  int iter_bs;
  std::shared_ptr<short> recvd_opt_tbl = nullptr;
};