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
//  const int BUFFER_OFFSET = 3;
  // Maximum message size sent by a vertex. To be set later correctly.
  int max_msg_size = 500;
  int node_count = 1;

  int my_vertex_displas = 0;
  int my_vertex_count = 0;

  int world_proc_rank;
  int world_procs_count;

  MPI_Comm MPI_COMM_INSTANCE;
  int instance_id;
  int instance_proc_rank;
  int instance_procs_count;

  std::shared_ptr<int> thread_id_to_vertex_offset = nullptr;
  std::shared_ptr<int> thread_id_to_vertex_count = nullptr;
  std::shared_ptr<std::map<int,int>> vertex_label_to_instance_rank = nullptr;

  ~parallel_ops();

  void teardown_parallelism();
  void set_parallel_decomposition(const char* file, const char* partfile, const char *out_file, int global_vertx_count, int global_edge_count, std::vector<std::shared_ptr<vertex>> *&vertices, int is_binary, int pic);
  void update_counts_and_displas(int msg_size);
  void all_to_all_v();

  static parallel_ops * initialize(int *argc, char ***argv);

private:
//  const int MSG_COUNT_OFFSET = 0;
//  const int MSG_SIZE_OFFSET = 2;



  MPI_Request *send_recv_reqs = nullptr;
  MPI_Status *send_recv_reqs_status = nullptr;

  /* All-to-all-v buffers */
  int *sdisplas = nullptr;
  int *rdisplas = nullptr;
  int *scounts = nullptr;
  int *rcounts = nullptr;
  std::shared_ptr<short> sbuff = nullptr;
  std::shared_ptr<short> rbuff = nullptr;

  /* performance counters */
  const char *out_file = nullptr;
  long *my_msg_counts = nullptr;
  long *all_msg_counts = nullptr;

  std::map<int, std::shared_ptr<std::vector<int>>> *recvfrom_rank_to_msgcount_and_destined_labels = nullptr;

  parallel_ops(int world_proc_rank, int world_procs_count);

  void simple_graph_partition(const char* file, int global_vertex_count, int global_edge_count, std::vector<std::shared_ptr<vertex>> *&vertices);
  void simple_graph_partition_file(const char* file, const char* partfile, int global_vertex_count, int global_edge_count, std::vector<std::shared_ptr<vertex>> *&vertices);
  void simple_graph_partition_binary(const char* file, int global_vertex_count, int global_edge_count, std::vector<std::shared_ptr<vertex>> *&vertices);
  long read_vertices(std::vector<std::shared_ptr<vertex>> *vertices, int skip_vertex_count, std::ifstream &fs, long header_extent, long data_offset,
                     long data_extent, int *vertex_nbr_length, int *out_nbrs, long read_extent, int read_vertex, int i);
  void find_nbrs(int global_vertex_count, int local_vertex_count, std::vector<std::shared_ptr<vertex>> *&vertices);
  std::string mpi_gather_string(std::string &str);

  void print_timing(const std::chrono::time_point<std::chrono::high_resolution_clock> &start_ms,
                    const std::chrono::time_point<std::chrono::high_resolution_clock> &end_ms, const std::string &msg) const;

  int read_int(long byte_idx, char *f);

};

#endif //CLIONCPP_PARALLEL_OPS_H
