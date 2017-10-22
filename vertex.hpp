//
// Created by Saliya Ekanayake on 6/24/17.
//

#ifndef CLIONCPP_VERTEX_HPP
#define CLIONCPP_VERTEX_HPP


#include <iostream>
#include <map>
#include <vector>
#include <random>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <bitset>
#include <ratio>
#include <chrono>
#include <ctime>
#include "recv_vertex_buffer.hpp"
#include "message.hpp"
#include "galois_field.hpp"

extern double bare_dt;
extern double rand_dt;
extern double step1_dt;
extern double step2_dt;
extern double step3_dt;

class vertex {
public:
  int count_comp_call = 0;
  vertex(int label, double weight, int* outnbrs, int outnbrs_length){
    this->label = label;
    this->weight = weight;
    outnbr_lbl_to_instance_rank = new std::map<int,int>();
    outrank_to_send_buffer = new std::map<int,std::shared_ptr<vertex_buffer>>();
    recv_buffers = new std::vector<std::shared_ptr<recv_vertex_buffer>>();
    msg = new message();
    recvd_msgs = new std::vector<std::shared_ptr<message>>();

    init_times();

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

    init_times();

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

  // vertex times
  std::vector<std::vector<double>> times_v;

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
  int data_idx = 0;

  void compute(int super_step, int iter, std::shared_ptr<int> completion_vars, std::shared_ptr<std::map<int, int>> random_assignments){
    int I = super_step+1;
    data_idx = I;
    if (super_step == 0){
      start_ticks = hrc_t::now();
      reset(iter, random_assignments);
      end_ticks = hrc_t::now();
      duration = (ms_t(end_ticks - start_ticks)).count();
      times_v[0].push_back(duration);
    } else if (super_step > 0){
      start_ticks = hrc_t::now();
      int field_size = gf->get_field_size();
      reset_super_step();
      running_ticks = hrc_t::now();
      duration = (ms_t(running_ticks - start_ticks)).count();
      times_v[2].push_back(duration);

      double tmp_duration = 0.0;
      for (const std::shared_ptr<message> &msg : (*recvd_msgs)){
        //ticks_t tmp_ticks = hrc_t::now();
        for (int i = 0; i < iter_bs; ++i) {
          count_comp_call++;
          ticks_t rand_ticks = hrc_t::now();
          int weight = (*uni_int_dist[i])(*rnd_engine[i]);

          ticks_t bare_ticks = hrc_t::now();
          int product = gf->multiply(opt_tbl.get()[1*iter_bs+i], msg->get(i));
          ticks_t step1_ticks = hrc_t::now();
          step1_dt += (ms_t(step1_ticks - bare_ticks)).count();
          product = gf->multiply(weight, product);
          ticks_t step2_ticks = hrc_t::now();
          step2_dt += (ms_t(step2_ticks - step1_ticks)).count();
          poly_arr[i] = gf->add(poly_arr[i], product);
          ticks_t step3_ticks = hrc_t::now();
          step3_dt += (ms_t(step3_ticks - step2_ticks)).count();
          bare_dt += (ms_t(step3_ticks - bare_ticks)).count();
          rand_dt += (ms_t(step3_ticks - rand_ticks)).count();

        }
        //tmp_duration += (ms_t(hrc_t::now() - tmp_ticks)).count();
      }
      duration = (ms_t(hrc_t::now() - running_ticks)).count();
      times_v[3].push_back(duration);
      times_v[5].push_back(tmp_duration);
      times_v[6].push_back(duration - tmp_duration);

      running_ticks = hrc_t::now();
      for (int i = 0; i < iter_bs; ++i) {
        opt_tbl.get()[I*iter_bs+i] = (short) poly_arr[i];
      }
      end_ticks = hrc_t::now();
      duration = (ms_t(end_ticks - running_ticks)).count();
      times_v[4].push_back(duration);

      duration = (ms_t(end_ticks - start_ticks)).count();
      times_v[1].push_back(duration);
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
      msg->copy(b->get_buffer(), offset, data_idx, iter_bs);
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
    // opt_table now contains entries for iter_bs iterations
    // elements i of each iteration are kept near
    // i.e. opt_tbl[0,(iter_bs-1)] are idx 0 values for iter_bs iterations
    // other indices follow the same rule
    opt_tbl_length = (k+1)*iter_bs;
    opt_tbl = std::shared_ptr<short>(new short[(k+1)*iter_bs](), std::default_delete<short[]>());
    poly_arr = new int[iter_bs];
  }

  void reset(int iter, std::shared_ptr<std::map<int,int>> random_assignments){
    /* create the vertex unique random engine */
    // Note, in C++ the distribution is in closed interval [a,b]
    // whereas in Java it's [a,b), so the random.nextInt(fieldSize)
    // equivalent in C++ is [0,gf->get_field_size() - 1]

    // Note, now we need iter_bs of random engines
    uni_int_dist = new std::uniform_int_distribution<int>*[iter_bs];
    rnd_engine = new std::default_random_engine*[iter_bs];
    for (int i = 0; i < iter_bs; ++i){
      uni_int_dist[i] =  new std::uniform_int_distribution<int>(0, gf->get_field_size()-1);
      rnd_engine[i] = new std::default_random_engine(uniq_rand_seed);
    }

    // set arrays in vertex data
    for (int i = 0; i < opt_tbl_length; ++i){
      opt_tbl.get()[i] = 0;
    }

    for (int i = 0; i < iter_bs; ++i) {
      // dot product is bitwise 'and'
      int dot_product = (*random_assignments)[label] & (iter+i);
      std::bitset<sizeof(int) * 8> bs((unsigned int) dot_product);
      int eigen_val = (bs.count() % 2 == 1) ? 0 : 1;
      opt_tbl.get()[1*iter_bs+i] = (short) eigen_val;
    }

    msg->set_data_and_msg_size(opt_tbl, iter_bs);
    // NOTE - let's not use this, see above
//    msg->set_data_and_msg_size(opt_tbl, (k+1));
  }

  void finalize_iteration(){
    for (int i = 0; i < iter_bs; ++i) {
      total_sum = (short) (*gf).add(total_sum, opt_tbl.get()[k*iter_bs+i]);
    }
  }

  bool finalize_iterations(){
    return total_sum > 0;
  }

private:
  typedef std::chrono::duration<double, std::milli> ms_t;
  typedef std::chrono::time_point<std::chrono::high_resolution_clock> ticks_t;
  typedef std::chrono::high_resolution_clock hrc_t;

  ticks_t start_ticks, running_ticks, end_ticks;
  double duration;

  int k;
  int iter_bs; // iteration block size
  int opt_tbl_length;
  std::shared_ptr<galois_field> gf;
  std::shared_ptr<short> opt_tbl = nullptr;
  short total_sum;
  std::uniform_int_distribution<int>** uni_int_dist = nullptr;
  std::default_random_engine** rnd_engine = nullptr;

  int *poly_arr = nullptr;
  void reset_super_step(){
    for (int i = 0; i < iter_bs; ++i){
      poly_arr[i] = 0;
    }
  }

  void init_times(){
    for (int i = 0; i < 7; ++i){
      std::vector<double> t;
      times_v.push_back(t);
    }
  }
};


#endif //CLIONCPP_VERTEX_HPP
