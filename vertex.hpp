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
    outnbr_lbl_to_world_rank = new std::map<int,int>();
    outrank_to_send_buffer = new std::map<int,std::shared_ptr<vertex_buffer>>();
    recv_buffers = new std::vector<std::shared_ptr<recv_vertex_buffer>>();
    msg = new message();
    recvd_msgs = new std::vector<std::shared_ptr<message>>();

    for (int i = 0; i < outnbrs_length; ++i) {
      (*outnbr_lbl_to_world_rank)[outnbrs[i]] = -1;
    }
  }

  vertex(std::vector<std::string> &tokens){
    this->label = std::stoi(tokens[0]);
    this->weight = std::stof(tokens[1]);
    outnbr_lbl_to_world_rank = new std::map<int,int>();
    outrank_to_send_buffer = new std::map<int,std::shared_ptr<vertex_buffer>>();
    recv_buffers = new std::vector<std::shared_ptr<recv_vertex_buffer>>();
    msg = new message();
    recvd_msgs = new std::vector<std::shared_ptr<message>>();

    for (int i = 2; i < tokens.size(); ++i){
      (*outnbr_lbl_to_world_rank)[std::stoi(tokens[i])] = -1;
    }
  }

  ~vertex(){
    delete outnbr_lbl_to_world_rank;
    outnbr_lbl_to_world_rank = nullptr;
    delete outrank_to_send_buffer;
    outrank_to_send_buffer = nullptr;
    delete recv_buffers;
    recv_buffers = nullptr;
    delete msg;
    msg = nullptr;
    delete recvd_msgs;
    recvd_msgs = nullptr;
    delete uni_int_dist;
    uni_int_dist = nullptr;
    delete rnd_engine;
    rnd_engine = nullptr;
  }

  // locally allocated, so no need of shared_ptr
  std::map<int,int>* outnbr_lbl_to_world_rank = nullptr;
  std::map<int,std::shared_ptr<vertex_buffer>>* outrank_to_send_buffer = nullptr;
  std::vector<std::shared_ptr<recv_vertex_buffer>>* recv_buffers = nullptr;

  message* msg = nullptr;
  std::vector<std::shared_ptr<message>>* recvd_msgs = nullptr;

  int label;
  double weight;
  long uniq_rand_seed;

  // The index of data to send for next super step
  int data_idx = 0;

  void compute(int super_step, int iter, std::shared_ptr<int> completion_vars, std::shared_ptr<std::map<int, int>> random_assignments){
    int I = super_step+1;
    data_idx = I;
    if (super_step == 0){
      reset(iter, random_assignments);
    } else if (super_step > 0){
      int field_size = gf->get_field_size();
      int poly = 0;
      for (const std::shared_ptr<message> &msg : (*recvd_msgs)){
        int weight = (*uni_int_dist)(*rnd_engine);
        int product = gf->multiply(opt_tbl.get()[1], msg->get());
        // NOTE - let's not use this, see above
//        int product = gf->multiply(opt_tbl.get()[1], msg->get(I-1));
        product = gf->multiply(weight, product);
        poly = gf->add(poly, product);
      }
      opt_tbl.get()[I] = (short)poly;
    }

    // TODO - dummy comp - list recvd messages
//    std::shared_ptr<short> data = std::shared_ptr<short>(new short[1](), std::default_delete<short[]>());
//    data.get()[0] = (short) label;
//    msg->set_data_and_msg_size(data, 1);

    /*if (super_step == 0){
      std::shared_ptr<short> data = std::shared_ptr<short>(new short[1](), std::default_delete<short[]>());
      data.get()[0] = (short) label;
      msg->set_data_and_msg_size(data, 1);
    } else if (super_step > 0){
      std::string str = "v";
      str.append(std::to_string(label)).append(" recvd [ ");
      for (const std::shared_ptr<message> msg : (*recvd_msgs)){
        str.append(std::to_string(msg->get())).append(" ");
      }
      str.append("] ss=").append(std::to_string(super_step)).append("\n");
      std::cout<<str;
    }*/
  }

  int prepare_send(int super_step, int shift){
    for (const auto &kv : (*outrank_to_send_buffer)){
      std::shared_ptr<vertex_buffer> b = kv.second;
      int offset = shift + b->get_offset_factor() * msg->get_msg_size();
      msg->copy(b->get_buffer(), offset, data_idx);
      // NOTE - let's not use this, see above
//      msg->copy(b->get_buffer(), offset);
    }
    return msg->get_msg_size();
  }

  void process_recvd(int super_step, int shift){
    for (int i = 0; i < recv_buffers->size(); ++i){
      std::shared_ptr<recv_vertex_buffer> b = (*recv_buffers)[i];
      std::shared_ptr<message> recvd_msg = (*recvd_msgs)[i];
      int recvd_msg_size = b->get_msg_size();
      recvd_msg->load(b->get_buffer(), shift+b->get_offset_factor()*recvd_msg_size, recvd_msg_size);
    }
  }

  void init(int k , int r, std::shared_ptr<galois_field> gf){
    this->k = k;
    this->gf = gf;
    opt_tbl = std::shared_ptr<short>(new short[k+1](), std::default_delete<short[]>());
  }

  void reset(int iter, std::shared_ptr<std::map<int,int>> random_assignments){
    /* create the vertex unique random engine */
    // Note, in C++ the distribution is in closed interval [a,b]
    // whereas in Java it's [a,b), so the random.nextInt(fieldSize)
    // equivalent in C++ is [0,gf->get_field_size() - 1]
    uni_int_dist = new std::uniform_int_distribution<int>(0, gf->get_field_size()-1);
    rnd_engine = new std::default_random_engine(uniq_rand_seed);

    // set arrays in vertex data
    for (int i = 0; i < k+1; ++i){
      opt_tbl.get()[i] = 0;
    }


    // dot product is bitwise 'and'
    int dot_product = (*random_assignments)[label] & iter;
    std::bitset<sizeof(int)*8> bs((unsigned int)dot_product);
    int eigen_val = (bs.count() % 2 == 1) ? 0 : 1;
    opt_tbl.get()[1] = (short) eigen_val;

    msg->set_data_and_msg_size(opt_tbl, 1);
    // NOTE - let's not use this, see above
//    msg->set_data_and_msg_size(opt_tbl, (k+1));
  }

  void finalize_iteration(){
    total_sum = (short) (*gf).add(total_sum, opt_tbl.get()[k]);
  }

  bool finalize_iterations(){
    return total_sum > 0;
  }

private:
  int k;
  std::shared_ptr<galois_field> gf;
  std::shared_ptr<short> opt_tbl = nullptr;
  short total_sum;
  std::uniform_int_distribution<int>* uni_int_dist = nullptr;
  std::default_random_engine* rnd_engine = nullptr;
};


#endif //CLIONCPP_VERTEX_HPP
