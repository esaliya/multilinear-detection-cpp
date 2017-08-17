//
// Created by Saliya Ekanayake on 6/24/17.
//

#ifndef CLIONCPP_VERTEX_HPP
#define CLIONCPP_VERTEX_HPP


#include <map>
#include <vector>
#include <random>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <iostream>
#include <bitset>
#include "recv_vertex_buffer.hpp"
#include "message.hpp"
#include "galois_field.hpp"

class vertex {
public:
  vertex(int label, double weight, int* outnbrs, int outnbrs_length){
    this->label = label;
    this->weight = weight;
    outnbr_lbl_to_instance_rank = new std::map<int,int>();
    outrank_to_send_buffer = new std::map<int,std::shared_ptr<vertex_buffer>>();
    recv_buffers = new std::vector<std::shared_ptr<recv_vertex_buffer>>();
    msg = new message();
    recvd_msgs = new std::vector<std::shared_ptr<message>>();

    for (int i = 0; i < outnbrs_length; ++i) {
      (*outnbr_lbl_to_instance_rank)[outnbrs[i]] = -1;
    }
  }

  vertex(std::vector<std::string> &tokens){
    this->label = std::stoi(tokens[0]);
    this->weight = std::stof(tokens[1]);
    outnbr_lbl_to_instance_rank = new std::map<int,int>();
    outrank_to_send_buffer = new std::map<int,std::shared_ptr<vertex_buffer>>();
    recv_buffers = new std::vector<std::shared_ptr<recv_vertex_buffer>>();
    msg = new message();
    recvd_msgs = new std::vector<std::shared_ptr<message>>();

    for (int i = 2; i < tokens.size(); ++i){
      (*outnbr_lbl_to_instance_rank)[std::stoi(tokens[i])] = -1;
    }
  }

  ~vertex(){
    delete outnbr_lbl_to_instance_rank;
    outnbr_lbl_to_instance_rank = nullptr;
    delete outrank_to_send_buffer;
    outrank_to_send_buffer = nullptr;
    delete recv_buffers;
    recv_buffers = nullptr;
    delete msg;
    msg = nullptr;
    delete recvd_msgs;
    recvd_msgs = nullptr;
    for (int i = 0; i < iter_bs; ++i) {
      delete uni_int_dist[i];
      uni_int_dist[i] = nullptr;
      delete rnd_engine[i];
      rnd_engine[i] = nullptr;
    }
    delete [] uni_int_dist;
    uni_int_dist = nullptr;
    delete [] rnd_engine;
    rnd_engine = nullptr;
    delete [] poly_arr;
    poly_arr = nullptr;
  }

  // locally allocated, so no need of shared_ptr
  std::map<int,int>* outnbr_lbl_to_instance_rank = nullptr;
  std::map<int,std::shared_ptr<vertex_buffer>>* outrank_to_send_buffer = nullptr;
  std::vector<std::shared_ptr<recv_vertex_buffer>>* recv_buffers = nullptr;

  message* msg = nullptr;
  std::vector<std::shared_ptr<message>>* recvd_msgs = nullptr;

  int label;
  double weight;
  long uniq_rand_seed;

  // The index of data to send for next super step
  int row_idx = 0;

  void compute(int super_step, int iter, std::shared_ptr<int> completion_vars, std::shared_ptr<std::map<int, int>> random_assignments){
    int I = super_step+1;
    row_idx = I;
    if (super_step == 0){
      reset(iter, completion_vars, random_assignments);
    } else if (super_step > 0){
      int field_size = gf->get_field_size();
      reset_super_step();

      // TODO - fix compute logic
/*      for (const std::shared_ptr<message> &msg : (*recvd_msgs)){
        for (int i = 0; i < iter_bs; ++i) {
          int weight = (*uni_int_dist[i])(*rnd_engine[i]);
          int product = gf->multiply(opt_tbl.get()[1*iter_bs+i], msg->get(i));
          product = gf->multiply(weight, product);
          poly_arr[i] = gf->add(poly_arr[i], product);
        }
      }

      for (int i = 0; i < iter_bs; ++i) {
        opt_tbl.get()[I*iter_bs+i] = (short) poly_arr[i];
      }*/
    }

    // TODO - dummy comp - list recvd messages
//    std::shared_ptr<short> data = std::shared_ptr<short>(new short[1](), std::default_delete<short[]>());
//    data.get()[0] = (short) label;
//    msg->set_data_and_msg_size(data, 1);

    /*std::shared_ptr<short> data = std::shared_ptr<short>(new short[2](), std::default_delete<short[]>());
    if (super_step == 0){
      data.get()[0] = (short) label;
      data.get()[1] = (short) label;
      msg->set_data_and_msg_size(data, 2);
    } else if (super_step > 0){
      data.get()[0] = (short) (label+1);
      data.get()[1] = (short) (label+1);
      msg->set_data_and_msg_size(data, 2);
      std::string str = "v";
      str.append(std::to_string(label)).append(" recvd [ ");
      for (const std::shared_ptr<message> msg : (*recvd_msgs)){
        str.append(std::to_string(msg->get(0))).append(" ")
            .append(std::to_string(msg->get(1))).append(" ");
      }
      str.append("] ss=").append(std::to_string(super_step)).append("\n");
      std::cout<<str;
    }*/
  }

  int prepare_send(int super_step){
    for (const auto &kv : (*outrank_to_send_buffer)){
      std::shared_ptr<vertex_buffer> b = kv.second;
      int offset = (b->get_buffer_offset_factor()+b->get_vertex_offset_factor())*msg->get_msg_size();
      msg->copy(b->get_buffer(), offset, row_idx);
    }
    return msg->get_msg_size();
  }

  void process_recvd(int super_step){
    for (int i = 0; i < recv_buffers->size(); ++i){
      std::shared_ptr<recv_vertex_buffer> b = (*recv_buffers)[i];
      std::shared_ptr<message> recvd_msg = (*recvd_msgs)[i];
      // Assume received msg size is as same as my send msg size
      recvd_msg->load(b->get_buffer(),
                      (b->get_vertex_offset_factor()+b->get_buffer_offset_factor())*msg->get_msg_size(),
                      msg->get_msg_size());
    }
  }

  void init(int k , int r, std::shared_ptr<galois_field> gf, int iter_bs){
    this->k = k;
    this->gf = gf;
    this->iter_bs = iter_bs;

    dim_rows = (k+1);
    dim_cols = (r+1);

    // We need a (k+1) x (r+1) matrix for each iteration, so
    // opt_table now contains matrices for iter_bs iterations.
    // All rows of each matrix are kept near,
    // i.e. opt_tbl[0,(((k+1)*(r+1))-1)] are rows of
    // matrix of iteration 0.
    // other indices follow the same convention
    // in a picture, this looks like,
    //   |<-all rows iter0->| |<-all rows iter1->|        |<-all rows iter_bs-1->|
    //   |<-- k+1 entries ->|
    // [ [[r+1][r+1]...[r+1]] [[r+1][r+1]...[r+1]] ......   [[r+1][r+1]...[r+1]]   ]
    opt_ext_tbl_length = dim_cols*dim_rows*iter_bs;
    opt_tbl = std::shared_ptr<short>(
        new short[opt_ext_tbl_length](), std::default_delete<short[]>());
    // ext_tbl is similar to opt_tbl in dimensions and data storage format
    ext_tbl = std::shared_ptr<short>(
        new short[opt_ext_tbl_length](), std::default_delete<short[]>());
    // add all iterations into total sum, so it's (r+1)*(k+1) in length
    total_sum_tbl_length = dim_cols*dim_rows;
    total_sum_tbl = std::shared_ptr<int>(new int[total_sum_tbl_length](), std::default_delete<int[]>());
    std::fill_n(total_sum_tbl.get(), total_sum_tbl_length, 0);
    cumulative_completion_variables = std::shared_ptr<int>(
        new int[k*iter_bs](), std::default_delete<int[]>());
    poly_arr = new int[iter_bs];

    msg->init(dim_rows, dim_cols, iter_bs);
    // msg_size is row size (i.e. dim_cols) * number of iterations
    msg->set_data_and_msg_size(opt_tbl, dim_cols*iter_bs);
    for (const auto &msg : (*recvd_msgs)){
      msg->recv_init(dim_rows, dim_cols, iter_bs);
    }
  }

  void reset(int iter, std::shared_ptr<int> completion_vars, std::shared_ptr<std::map<int,int>> random_assignments){

    // TODO - fix
    /* create the vertex unique random engine */
    // Note, in C++ the distribution is in closed interval [a,b]
    // whereas in Java it's [a,b), so the random.nextInt(fieldSize)
    // equivalent in C++ is [0,gf->get_field_size() - 1]

    // Note, now we need iter_bs of random engines
//    uni_int_dist = new std::uniform_int_distribution<int>*[iter_bs];
//    rnd_engine = new std::default_random_engine*[iter_bs];
//    for (int i = 0; i < iter_bs; ++i){
//      uni_int_dist[i] =  new std::uniform_int_distribution<int>(0, gf->get_field_size()-1);
//      rnd_engine[i] = new std::default_random_engine(uniq_rand_seed);
//    }
//
//    // set arrays in vertex data
//    for (int i = 0; i < opt_ext_tbl_length; ++i){
//      opt_tbl.get()[i] = 0;
//      ext_tbl.get()[i] = 0;
//    }
//
//    int nodeWeight = (int)weight;
//    for (int i = 0; i < iter_bs; ++i) {
//      // dot product is bitwise 'and'
//      int dot_product = (*random_assignments)[label] & (iter+i);
//      int eigen_val = (bit_count((unsigned int)dot_product) % 2 == 1) ? 0 : 1;
//      opt_tbl.get()[1*iter_bs+i] = (short) eigen_val;
//    }
  }

  void finalize_iteration(){
    // TODO - debug fix
//    for (int i = 0; i < iter_bs; ++i) {
//      total_sum = (short) (*gf).add(total_sum, opt_tbl.get()[k*iter_bs+i]);
//    }
  }

  double finalize_iterations(double alpha_max, int rounding_factor){
    // TODO - debug - let's return some double value for now
    return label;
  }

private:
  int k;
  int r;
  int iter_bs; // iteration block size
  int opt_ext_tbl_length;
  int total_sum_tbl_length;
  int dim_rows, dim_cols;
  std::shared_ptr<galois_field> gf;
  std::shared_ptr<short> opt_tbl = nullptr;
  std::shared_ptr<short> ext_tbl = nullptr;
  std::shared_ptr<int> total_sum_tbl = nullptr;
  std::shared_ptr<int> cumulative_completion_variables = nullptr;
  std::uniform_int_distribution<int>** uni_int_dist = nullptr;
  std::default_random_engine** rnd_engine = nullptr;

  int *poly_arr = nullptr;
  void reset_super_step(){
    for (int i = 0; i < iter_bs; ++i){
      poly_arr[i] = 0;
    }
  }

  double BJ(double alpha, int anomalous_count, int set_size) {
    return set_size * KL(((double) anomalous_count) / set_size, alpha);
  }

  double KL(double a, double b) {
    assert(a >= 0 && a <= 1);
    assert(b > 0 && b < 1);

    // corner cases
    if (a == 0) {
      return log(1 / (1 - b));
    }
    if (a == 1) {
      return log(1 / b);
    }
    return a * log(a / b) + (1 - a) * log((1 - a) / (1 - b));
  }

  // Converted method from Java Integer.bitCount(int i)
  int bit_count(unsigned int i){
    i = i - ((i >> 1) & 0x55555555);
    i = (i & 0x33333333) + ((i >> 2) & 0x33333333);
    i = (i + (i >> 4)) & 0x0f0f0f0f;
    i = i + (i >> 8);
    i = i + (i >> 16);
    return i & 0x3f;
  }
};


#endif //CLIONCPP_VERTEX_HPP
