//
// Created by Saliya Ekanayake on 4/23/17.
//

#ifndef CLIONCPP_PARALLEL_OPS_H
#define CLIONCPP_PARALLEL_OPS_H

//extern int get_world_proc_rank;
//extern int get_world_procs_count;
//
//void initialize(int *argc, char ***argv);
//void teardown_parallelism();

#include <boost/shared_ptr.hpp>
#include <chrono>
#include <mpi.h>
#include "vertex.hpp"

class parallel_ops{
public:
  // to store msg count and msg size -- note msg count is stored as two shorts
  const int BUFFER_OFFSET = 3;
  // Maximum message size sent by a vertex. To be set later correctly.
  int max_msg_size = 500;
  int thread_count = 1;
  int node_count = 1;

  int my_vertex_displas = 0;
  int my_vertex_count = 0;

  std::shared_ptr<int> thread_id_to_vertex_offset = nullptr;
  std::shared_ptr<int> thread_id_to_vertex_count = nullptr;
  std::shared_ptr<std::map<int,int>> vertex_label_to_world_rank = nullptr;

  int get_world_proc_rank() const;
  int get_world_procs_count() const;

  ~parallel_ops();

  void teardown_parallelism();
  void set_parallel_decomposition(const char* file, int global_vertx_count, std::vector<std::shared_ptr<vertex>> *&vertices);
  void send_msgs(int msg_size);
  void recv_msgs();

  static parallel_ops * initialize(int *argc, char ***argv);

private:
  const int MSG_COUNT_OFFSET = 0;
  const int MSG_SIZE_OFFSET = 2;

  int world_proc_rank;
  int world_procs_count;

  int recv_req_offset;
  MPI_Request *send_recv_reqs = nullptr;
  MPI_Status *send_recv_reqs_status = nullptr;
  int total_reqs;
  int msg_size_to_recv;

  std::map<int, std::shared_ptr<std::vector<int>>> *recvfrom_rank_to_msgcount_and_destined_labels = nullptr;
  std::map<int, std::shared_ptr<short>> *recvfrom_rank_to_recv_buffer = nullptr;
  std::map<int, std::shared_ptr<short>> *sendto_rank_to_send_buffer = nullptr;

  parallel_ops(int world_proc_rank, int world_procs_count);

  void simple_graph_partition(const char* file, int global_vertex_count, std::vector<std::shared_ptr<vertex>> *&vertices);
  void decompose_among_threads(std::vector<std::shared_ptr<vertex>> *&vertices);
  void find_nbrs(int global_vertex_count, int local_vertex_count, std::vector<std::shared_ptr<vertex>> *&vertices);
  std::string mpi_gather_string(std::string &str);

  void print_timing(const std::chrono::time_point<std::chrono::high_resolution_clock> &start_ms,
                    const std::chrono::time_point<std::chrono::high_resolution_clock> &end_ms, const std::string &msg) const;

  void test_isend_irecv();

  void test_string_allreduce();
};

#endif //CLIONCPP_PARALLEL_OPS_H
