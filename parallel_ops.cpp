//
// Created by Saliya Ekanayake on 4/22/17.
//
#include "parallel_ops.hpp"
#include "polynomial.hpp"
#include <mpi.h>
#include <iostream>
#include <fstream>
#include <chrono>

typedef std::chrono::duration<double, std::milli> ms_t;

parallel_ops * parallel_ops::initialize(int *argc, char ***argv) {
  int rank, count;
  MPI_Init(argc, argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &count);
  return new parallel_ops(rank, count);
}

void parallel_ops::teardown_parallelism() {
    MPI_Finalize();
}

parallel_ops::~parallel_ops() {
  delete recvfrom_rank_to_msgcount_and_destined_labels;
  recvfrom_rank_to_msgcount_and_destined_labels = nullptr;
  delete [] send_recv_reqs_status;
  send_recv_reqs_status = nullptr;
  delete [] send_recv_reqs;
  send_recv_reqs = nullptr;
  /*delete recvfrom_rank_to_recv_buffer;
  recvfrom_rank_to_recv_buffer = nullptr;
  delete sendto_rank_to_send_buffer;
  sendto_rank_to_send_buffer = nullptr;*/
  delete out_file;
  out_file = nullptr;

  delete [] sdisplas;
  sdisplas = nullptr;
  delete [] rdisplas;
  rdisplas = nullptr;
  delete [] scounts;
  scounts = nullptr;
  delete [] rcounts;
  rcounts = nullptr;
}

parallel_ops::parallel_ops(int world_proc_rank, int world_procs_count) :
    world_proc_rank(world_proc_rank),
    world_procs_count(world_procs_count) {
}

void parallel_ops::set_parallel_decomposition(const char *file, const char *out_file,
                                              int global_vertx_count, int global_edge_count,
                                              std::vector<std::shared_ptr<vertex>> *&vertices,
                                              int is_binary, int pic) {
  /* Decompose world into pic instances */
  instance_id = world_proc_rank / (world_procs_count/pic);
  MPI_Comm_split(MPI_COMM_WORLD, instance_id, world_proc_rank, &MPI_COMM_INSTANCE);
  MPI_Comm_rank(MPI_COMM_INSTANCE, &instance_proc_rank);
  MPI_Comm_size(MPI_COMM_INSTANCE, &instance_procs_count);
  assert(instance_procs_count == (world_procs_count/pic));

  /* Allocate timing buffer */
  times = std::shared_ptr<double>(new double[world_procs_count](), std::default_delete<double[]>());

  parallel_ops::out_file = out_file;
  if (is_binary){
    simple_graph_partition_binary(file, global_vertx_count, global_edge_count, vertices);
  } else {
    simple_graph_partition(file, global_vertx_count, global_edge_count, vertices);
  }
}

// NOTE - Old method keep it for now
void parallel_ops::simple_graph_partition(const char *file, int global_vertex_count, int global_edge_count, std::vector<std::shared_ptr<vertex>> *&vertices) {
  std::chrono::time_point<std::chrono::high_resolution_clock > start, end;

  int q = global_vertex_count/instance_procs_count;
  int r = global_vertex_count % instance_procs_count;
  int local_vertex_count = (instance_proc_rank < r) ? q+1: q;
  int skip_vertex_count = q*instance_proc_rank + (instance_proc_rank < r ? instance_proc_rank : r);

#ifdef LONG_DEBUG
  std::string debug_str = (instance_id == 0 && instance_proc_rank==0) ? "DEBUG: simple_graph_partition: 1: q,r,localvc  [ " : " ";
  debug_str.append("[").append(std::to_string(q)).append(",").append(std::to_string(r)).append(",").append(std::to_string(local_vertex_count)).append("] ");
  debug_str = mpi_gather_string(debug_str);
  if (instance_id == 0 && instance_proc_rank == 0){
    std::cout<<std::endl<<std::string(debug_str).append("]")<<std::endl;
  }
#endif

  std::ifstream fs;
  std::string line;
  std::vector<std::string> tokens;
  vertices = new std::vector<std::shared_ptr<vertex>>((unsigned long) local_vertex_count);

  fs.open(file);

  start = std::chrono::high_resolution_clock::now();
  int local_idx;
  for (int i = 0; i < global_vertex_count; ++i) {
    getline(fs, line);
    if (i < skip_vertex_count) {
      continue;
    }
    local_idx = i-skip_vertex_count;
    boost::split(tokens, line, boost::is_any_of(" "), boost::token_compress_on);
    (*vertices)[local_idx] = std::make_shared<vertex>(tokens);

    if (local_idx+1 == local_vertex_count){
      break;
    }
  }
  end = std::chrono::high_resolution_clock::now();
  print_timing(start, end, "simple_graph_partition: graph_read");

  fs.close();

  start = std::chrono::high_resolution_clock::now();
  find_nbrs(global_vertex_count, local_vertex_count, vertices);
  end = std::chrono::high_resolution_clock::now();
  print_timing(start, end, "simple_graph_partition: find_nbrs total");
}

void parallel_ops::find_nbrs(int global_vertex_count, int local_vertex_count, std::vector<std::shared_ptr<vertex>> *&vertices) {
  std::chrono::time_point<std::chrono::high_resolution_clock > start_ms, end_ms;

#ifdef LONG_DEBUG
  int tmp_lbl = 0;
#endif

  start_ms = std::chrono::high_resolution_clock::now();
  /* Create a map to quickly lookup vertices given their label for my vertices */
  std::map<int, std::shared_ptr<vertex>> *label_to_vertex = new std::map<int, std::shared_ptr<vertex>>();
  for (std::vector<std::shared_ptr<vertex>>::iterator it = vertices->begin(); it != vertices->end(); ++it){
#ifdef LONG_DEBUG
    tmp_lbl = (*it)->label;
#endif
    (*label_to_vertex)[(*it)->label] = *it;
  }
  end_ms = std::chrono::high_resolution_clock::now();
  print_timing(start_ms, end_ms, "find_nbr: label_to_vertex map creation");


#ifdef LONG_DEBUG
  /* Check vertex label to vertex map */
  std::string debug_str = (instance_id == 0 && instance_proc_rank==0) ? "DEBUG: find_nbrs: 1: lastvertex [ " : " ";
  debug_str.append(std::to_string((*label_to_vertex)[tmp_lbl]->label)).append(" ");
  debug_str = mpi_gather_string(debug_str);
  if (instance_id == 0 && instance_proc_rank == 0){
    std::cout<<std::endl<<std::string(debug_str).append("]")<<std::endl;
  }
#endif

  int *local_vertex_counts = new int[instance_procs_count];
  MPI_Allgather(&local_vertex_count, 1, MPI_INT, local_vertex_counts, 1, MPI_INT, MPI_COMM_INSTANCE);

  my_vertex_count = local_vertex_count;
#ifdef LONG_DEBUG
  /* Check local vertex counts */
  debug_str = (instance_id == 0 && world_proc_rank==0) ? "DEBUG: find_nbrs: 2: vcount [ " : " ";
  for (int i = 0; i < instance_procs_count; ++i) {
    debug_str.append(std::to_string(local_vertex_counts[i])).append(" ");
  }
  if (instance_id == 0 && instance_proc_rank == 0){
    std::cout<<std::endl<<std::string(debug_str).append("]")<<std::endl;
  }
#endif

  int *local_vertex_displas = new int[instance_procs_count];
  local_vertex_displas[0] = 0;
  for (int i = 1; i < instance_procs_count; ++i){
    local_vertex_displas[i] = local_vertex_displas[i-1]+local_vertex_counts[i-1];
  }

  my_vertex_displas = local_vertex_displas[instance_proc_rank];

#ifdef LONG_DEBUG
  /* Check local vertex displas */
  debug_str = (instance_id == 0 && instance_proc_rank==0) ? "DEBUG: find_nbrs: 3: vdisplas [ " : " ";
  for (int i = 0; i < instance_procs_count; ++i) {
    debug_str.append(std::to_string(local_vertex_displas[i])).append(" ");
  }
  if (instance_id == 0 && instance_proc_rank == 0){
    std::cout<<std::endl<<std::string(debug_str).append("]")<<std::endl;
  }
#endif

  start_ms = std::chrono::high_resolution_clock::now();
  int *global_vertex_labels = new int[global_vertex_count];
  int offset = local_vertex_displas[instance_proc_rank];
  for (int i = 0; i < local_vertex_count; ++i){
    global_vertex_labels[i+offset] = (*vertices)[i]->label;
  }
  MPI_Allgatherv(MPI_IN_PLACE, local_vertex_count, MPI_INT, global_vertex_labels,
                 local_vertex_counts, local_vertex_displas, MPI_INT, MPI_COMM_INSTANCE);
  end_ms = std::chrono::high_resolution_clock::now();
  print_timing(start_ms, end_ms, "find_nbr: global_vertex_labels allgather");

#ifdef LONG_DEBUG
  /* Check global vertex labels */
  debug_str = (instance_id == 0 && instance_proc_rank==0) ? "DEBUG: find_nbrs: 4: vlabels [ \n" : " ";
  for (int i = 0; i < instance_procs_count; ++i) {
    debug_str.append("  r").append(std::to_string(i)).append("[ ");
    for (int j = 0; j < local_vertex_counts[i]; ++j){
      debug_str.append(std::to_string(global_vertex_labels[local_vertex_displas[i]+j])).append(" ");
    }
    debug_str.append("]\n");
  }
  if (instance_id == 0 && world_proc_rank == 0){
    std::cout<<std::endl<<std::string(debug_str).append("]")<<std::endl;
  }
#endif

  /* Just keep in mind this map and the global_vertex_labels can be really large
   * Think of optimizations if this becomes a bottleneck */
  start_ms = std::chrono::high_resolution_clock::now();
  vertex_label_to_instance_rank = std::make_shared<std::map<int,int>>();
  std::map<int,int> *label_to_instance_rank = new std::map<int,int>();
  for (int rank = 0; rank < instance_procs_count; ++rank){
    for (int i = 0; i < local_vertex_counts[rank]; ++i){
      offset = local_vertex_displas[rank];
      (*vertex_label_to_instance_rank)[global_vertex_labels[i + offset]] = rank;
    }
  }
  end_ms = std::chrono::high_resolution_clock::now();
  print_timing(start_ms, end_ms, "find_nbr: label_to_instance_rank map creation");

  /* Set where out-neighbors of vertices live */
  start_ms = std::chrono::high_resolution_clock::now();
  for (const std::shared_ptr<vertex> &v : (*vertices)){
    std::map<int, int> *outnbr_label_to_instance_rank = v->outnbr_lbl_to_instance_rank;
    for (const auto &kv : (*outnbr_label_to_instance_rank)){
      int rank = (*vertex_label_to_instance_rank)[kv.first];
      (*outnbr_label_to_instance_rank)[kv.first] = rank;
      (*v->outrank_to_send_buffer)[rank] = std::make_shared<vertex_buffer>();
    }
  }
  end_ms = std::chrono::high_resolution_clock::now();
  print_timing(start_ms, end_ms, "find_nbr: set outnbr_label_to_instance_rank for each my v");

#ifdef LONG_DEBUG
  debug_str = (instance_id == 0 && world_proc_rank==0) ? "DEBUG: find_nbrs: 5: out_nbrs [ \n" : " ";
  debug_str.append("  r").append(std::to_string(instance_proc_rank)).append("[\n");
  for (const std::shared_ptr<vertex> &v : (*vertices)){
    std::map<int, int> *outnbr_label_to_instance_rank = v->outnbr_lbl_to_instance_rank;
    debug_str.append("    v").append(std::to_string(v->label)).append("[\n");
    for (const auto &kv : (*outnbr_label_to_instance_rank)){
      debug_str.append("      nbr").append(std::to_string(kv.first)).append("->r").append(std::to_string(kv.second)).append("\n");
    }
    debug_str.append("    ]\n");
  }
  debug_str = mpi_gather_string(debug_str);
  if (instance_id == 0 && instance_proc_rank == 0){
    std::cout<<std::endl<<std::string(debug_str).append("]")<<std::endl;
  }
#endif

  /* The following sections are very subtle, so keep your eyes open */

  // BEGIN ----------------
  start_ms = std::chrono::high_resolution_clock::now();
  std::map<int, std::shared_ptr<std::vector<int>>> *sendto_rank_to_msgcount_and_destined_labels
      = new std::map<int, std::shared_ptr<std::vector<int>>>();
  int msg_size = 0;
  // The vertex order in the loop is important as it's implicitly assumed to reduce communication cost
  for (const std::shared_ptr<vertex> &v : (*vertices)){
    // inverse_map is essentially a mapping from ranks this particular vertex
    // needs to communicate to the neighbor vertices that reside on those ranks
    std::map<int, std::shared_ptr<std::vector<int>>> *inverse_map
        = new std::map<int, std::shared_ptr<std::vector<int>>>();
    // Populate inverse_map
    for (const auto &kv : (*v->outnbr_lbl_to_instance_rank)){
      int destined_label = kv.first;
      int destined_rank = kv.second;
      std::map<int, std::shared_ptr<std::vector<int>>>::iterator it = inverse_map->find(destined_rank);
      if (it == inverse_map->end()){
        (*inverse_map)[destined_rank] = std::make_shared<std::vector<int>>();
      }
      it = inverse_map->find(destined_rank);
      // Now, "it" iterator should not be empty
      assert(it != inverse_map->end());
      it->second->push_back(destined_label);
    }

    for (const auto &kv : (*inverse_map)){
      int destined_rank = kv.first;
      std::shared_ptr<std::vector<int>> destined_labels = kv.second;
      std::map<int, std::shared_ptr<std::vector<int>>>::iterator it
          = sendto_rank_to_msgcount_and_destined_labels->find(destined_rank);
      if (it == sendto_rank_to_msgcount_and_destined_labels->end()){
        (*sendto_rank_to_msgcount_and_destined_labels)[destined_rank] = std::make_shared<std::vector<int>>();
        msg_size += 2;
      }
      it = sendto_rank_to_msgcount_and_destined_labels->find(destined_rank);
      // Now, "it" iterator should not be empty
      assert(it != sendto_rank_to_msgcount_and_destined_labels->end());
      std::shared_ptr<std::vector<int>> count_and_destined_vertex_labels = it->second;
      if (count_and_destined_vertex_labels->size() > 0){
        ++(*count_and_destined_vertex_labels)[0];
      } else {
        count_and_destined_vertex_labels->push_back(1);
      }

      if (destined_labels->size() > 1){
        count_and_destined_vertex_labels->push_back((int &&) (-1 * destined_labels->size()));
        std::copy(destined_labels->begin(), destined_labels->end(),
                  std::back_inserter(*count_and_destined_vertex_labels));
        msg_size += destined_labels->size()+1;
      } else {
        count_and_destined_vertex_labels->push_back((*destined_labels)[0]);
        msg_size += 1;
      }
    }
    delete inverse_map;
  }

//#ifdef LONG_DEBUG
  // perf counter
  if (instance_id == 0 && instance_proc_rank == 0){
    std::cout<<"** Instance 0 Rank 0 writing perf output to "<< out_file;
  }
  all_msg_counts = new long[instance_procs_count*instance_procs_count];
  my_msg_counts = new long[instance_procs_count];
  int rank_offset = instance_proc_rank*instance_procs_count;
  for (const auto &kv : (*sendto_rank_to_msgcount_and_destined_labels)){
    my_msg_counts[kv.first] = (*kv.second)[0];
  }
  MPI_Gather(my_msg_counts, instance_procs_count, MPI_LONG, all_msg_counts, instance_procs_count, MPI_LONG, 0, MPI_COMM_INSTANCE);

  std::ofstream out_fs;
  if (instance_id == 0 && instance_proc_rank == 0){
    out_fs.open(out_file);
    out_fs << "from/to rank ";
    for (int i = 0; i < instance_procs_count; ++i){
      out_fs << i << " ";
    }
    out_fs << std::endl;
    for (int i = 0; i < instance_procs_count; ++i){
      int total_out_degree = 0;
      int total_degree = 0;
      out_fs << i << " ";
      for (int j = 0; j < instance_procs_count; ++j){
        long msg_count = all_msg_counts[i * instance_procs_count + j];
        out_fs << msg_count << " ";
        total_degree += msg_count;
        if (i != j){
          total_out_degree += msg_count;
        }
      }
      out_fs << total_degree << " " << total_out_degree << std::endl;
    }
  }
  out_fs.close();
//#endif

  delete [] all_msg_counts;
  all_msg_counts = nullptr;
  delete [] my_msg_counts;
  my_msg_counts = nullptr;

  end_ms = std::chrono::high_resolution_clock::now();
  print_timing(start_ms, end_ms, "find_nbr: sendto_rank_to_msgcount_and_destined_labels");

#ifdef LONG_DEBUG
  /* print how many message counts and what are destined vertex labels (in order) for each rank from me */
  debug_str = (instance_id == 0 && instance_proc_rank==0) ? "DEBUG: find_nbrs: 6: msg_counts_and_labels [ \n" : " ";
  debug_str.append("  r").append(std::to_string(instance_proc_rank))
      .append(" msg_size=").append(std::to_string(msg_size)).append("[\n");
  for (const auto &pair : (*sendto_rank_to_msgcount_and_destined_labels)){
    std::shared_ptr<std::vector<int>> list = pair.second;
    debug_str.append("    sends ").append(std::to_string((*list)[0])).append(" msgs to rank ").append(std::to_string(pair.first)).append(" destined labels [ ");
    for (auto it = ++(list->begin()); it != list->end(); ++it){
      debug_str.append(std::to_string(*it)).append(" ");
    }
    debug_str.append("]\n");
  }
  debug_str = mpi_gather_string(debug_str);
  if (instance_id == 0 && instance_proc_rank == 0){
    std::cout<<std::endl<<std::string(debug_str).append("]")<<std::endl;
  }
#endif
  // END ----------------

  // BEGIN ################
  start_ms = std::chrono::high_resolution_clock::now();
  int max_buffer_size = -1;
  MPI_Allreduce(&msg_size, &max_buffer_size, 1, MPI_INT, MPI_MAX, MPI_COMM_INSTANCE);
  max_buffer_size += 1; // +1 to send msgSize
  recvfrom_rank_to_msgcount_and_destined_labels
      = new std::map<int, std::shared_ptr<std::vector<int>>>();
  int *buffer = new int[max_buffer_size];
  for (int rank = 0; rank < instance_procs_count; ++rank){
    if (rank == instance_proc_rank){
      buffer[0] = msg_size;
      int idx = 1;
      for (const auto &kv : (*sendto_rank_to_msgcount_and_destined_labels)){
        buffer[idx++] = -1 * (kv.first + global_vertex_count + 1);
        for (const auto &val : (*kv.second)){
          buffer[idx++] = val;
        }
      }
    }
    MPI_Bcast(buffer, max_buffer_size, MPI_INT, rank, MPI_COMM_INSTANCE);
    int recv_msg_size = buffer[0];

    for (int i = 1; i <= recv_msg_size; ++i) {
      int val = buffer[i];
      if (val < 0 && val < -1 * (global_vertex_count)) {
        // It's the rank information
        int destined_rank = (-1 * val) - (global_vertex_count + 1);
        if (destined_rank == instance_proc_rank) {
          // It'll always be a unique rank, so no need to check if exists
          std::shared_ptr<std::vector<int>> count_and_destined_vertex_labels
              = std::make_shared<std::vector<int>>();
          (*recvfrom_rank_to_msgcount_and_destined_labels)[rank] = count_and_destined_vertex_labels;
          for (int j = i + 1; j <= recv_msg_size; ++j) {
            val = buffer[j];
            if (val >= 0 || (val < 0 && val >= -1 * global_vertex_count)) {
              count_and_destined_vertex_labels->push_back(val);
            } else {
              break;
            }
          }
          break;
        }
      }
    }
  }
  delete [] buffer;
  end_ms = std::chrono::high_resolution_clock::now();
  print_timing(start_ms, end_ms, "find_nbr: recvfrom_rank_to_msgcount_and_destined_labels");

#ifdef LONG_DEBUG
  /* print how many message counts and what are destined vertex labels (in order) for me from each rank */
  debug_str = (instance_id == 0 && instance_proc_rank==0) ? "DEBUG: find_nbrs: 7: msg_counts_and_labels [ \n" : " ";
  debug_str.append("  r").append(std::to_string(instance_proc_rank)).append("[\n");
  for (const auto &pair : (*recvfrom_rank_to_msgcount_and_destined_labels)){
    std::shared_ptr<std::vector<int>> list = pair.second;
    debug_str.append("    recvs ").append(std::to_string((*list)[0])).append(" msgs from rank ").append(std::to_string(pair.first)).append(" destined labels [ ");
    for (auto it = ++(list->begin()); it != list->end(); ++it){
      debug_str.append(std::to_string(*it)).append(" ");
    }
    debug_str.append("]\n");
  }
  debug_str = mpi_gather_string(debug_str);
  if (instance_id == 0 && world_proc_rank == 0){
    std::cout<<std::endl<<std::string(debug_str).append("]")<<std::endl;
  }
#endif
  // END ################

  // BEGIN ~~~~~~~~~~~~~~~~
  start_ms = std::chrono::high_resolution_clock::now();
  /* All-to-all-v modification */
  rcounts = new int [instance_procs_count]();
  rdisplas = new int [instance_procs_count]();

  int total_recv_msg_count = 0;
  for (int i = 0; i < instance_procs_count; ++i){
    auto it = recvfrom_rank_to_msgcount_and_destined_labels->find(i);
    if (it == recvfrom_rank_to_msgcount_and_destined_labels->end()){
      // This rank recvs nothing from rank i, so set dummy count to 1
      rcounts[i] = 1;
      ++total_recv_msg_count;
    } else {
      int msg_count = (*(it->second))[0];
      rcounts[i] = msg_count;
      total_recv_msg_count += msg_count;
    }
  }

  rdisplas[0] = 0;
  for (int i = 1; i < instance_procs_count; ++i){
    rdisplas[i] = rdisplas[i-1] + rcounts[i-1];
  }
  rbuff = std::shared_ptr<short>(new short[total_recv_msg_count*max_msg_size](), std::default_delete<short[]>());


//  recvfrom_rank_to_recv_buffer = new std::map<int, std::shared_ptr<short>>();
  for (const auto &kv : (*recvfrom_rank_to_msgcount_and_destined_labels)){
    int recvfrom_rank = kv.first;
    std::shared_ptr<std::vector<int>> count_and_destined_vertex_labels = kv.second;
    int msg_count = (*count_and_destined_vertex_labels)[0];

    int current_msg = 0;
    for (int i = 1; i < count_and_destined_vertex_labels->size(); ){
      int val = (*count_and_destined_vertex_labels)[i];
      if (val >= 0){
        std::shared_ptr<vertex> vertex = (*label_to_vertex)[val];
        vertex->recv_buffers->push_back(
            std::make_shared<recv_vertex_buffer>(current_msg, rdisplas[recvfrom_rank],
                                                 recvfrom_rank, rbuff));
        vertex->recvd_msgs->push_back(std::make_shared<message>());
        ++current_msg;
        ++i;
      } else if (val < 0){
        // message intended for multiple vertices
        int intended_vertex_count = -1 * val;
        for (int j = i+1; j <= intended_vertex_count+i; ++j){
          val = (*count_and_destined_vertex_labels)[j];
          std::shared_ptr<vertex> vertex = (*label_to_vertex)[val];
          vertex->recv_buffers->push_back(
              std::make_shared<recv_vertex_buffer>(current_msg, rdisplas[recvfrom_rank],
                                                   recvfrom_rank, rbuff));
          vertex->recvd_msgs->push_back(std::make_shared<message>());
        }
        i+=intended_vertex_count+1;
        ++current_msg;
      }
    }
  }
  end_ms = std::chrono::high_resolution_clock::now();
  print_timing(start_ms, end_ms, "find_nbr: all-to-all-v part of recvfrom_rank_to_recv_buffer");

#ifdef LONG_DEBUG
  /* print recvfrom_rank_to_recv_buffer */
  debug_str = (instance_id == 0 && instance_proc_rank==0) ? "DEBUG: find_nbrs: 8: recvfrom_rank_to_recv_buffer [ \n" : "";
  debug_str.append("  r").append(std::to_string(instance_proc_rank)).append("[\n");
  for (const auto &vertex : (*vertices)){
    debug_str.append("    vlabel ").append(std::to_string((*vertex).label)).append(" recvs [\n");
    for (auto &recv_buffer : (*(*vertex).recv_buffers)){
      debug_str.append("      from rank ").append(std::to_string((*recv_buffer).get_recvfrom_rank()))
          .append(" offset_factor ").append(std::to_string((*recv_buffer).get_offset_factor())).append("\n");
    }
    debug_str.append("    ]\n");
  }
  debug_str.append("  ]\n");
  debug_str = mpi_gather_string(debug_str);
  if (instance_id == 0 && world_proc_rank == 0){
    std::cout<<std::endl<<std::string(debug_str).append("]")<<std::endl;
  }
#endif
  // END ~~~~~~~~~~~~~~~~

  // BEGIN ================
  start_ms = std::chrono::high_resolution_clock::now();
  /* All-to-all-v modification */
  // Note, some ranks won't have anything to send to some other ranks
  // and sending count=0 with alltoallv is not working correctly, so
  // send one dummy value for such cases. The good thing is we'll
  // never be reading those dummy values because of the correct offsets
  scounts = new int[instance_procs_count]();
  sdisplas = new int[instance_procs_count]();

  int total_send_msg_count = 0;
  for (int i = 0; i < instance_procs_count; ++i){
    auto it = sendto_rank_to_msgcount_and_destined_labels->find(i);
    if (it == sendto_rank_to_msgcount_and_destined_labels->end()){
      // This rank sends nothing to rank i, so set the dummy count to 1
      scounts[i] = 1;
      ++total_send_msg_count;
    } else {
      int msg_count = (*(it->second))[0];
      scounts[i] = msg_count;
      total_send_msg_count += msg_count;
    }
  }

  sdisplas[0] = 0;
  for (int i = 1; i < instance_procs_count; ++i){
    sdisplas[i] = sdisplas[i-1] + scounts[i-1];
  }
  sbuff = std::shared_ptr<short>(new short[total_send_msg_count*max_msg_size](), std::default_delete<short[]>());

  std::map<int,int> *outrank_to_offset_factor = new std::map<int,int>();
  for (const std::shared_ptr<vertex> &vertex : (*vertices)){
    for (const auto &kv : *(*vertex).outrank_to_send_buffer){
      int outrank = kv.first;
      if(outrank_to_offset_factor->find(outrank) == outrank_to_offset_factor->end()){
        (*outrank_to_offset_factor)[outrank] = 0;
      } else {
        ++((*outrank_to_offset_factor)[outrank]);
      }
      std::shared_ptr<vertex_buffer> vertex_send_buffer = (*vertex->outrank_to_send_buffer)[outrank];
      vertex_send_buffer->set_vertex_offset_factor((*outrank_to_offset_factor)[outrank]);
      vertex_send_buffer->set_buffer(sbuff, sdisplas[outrank]);
    }
  }
  end_ms = std::chrono::high_resolution_clock::now();
  print_timing(start_ms, end_ms, "find_nbr: all-to-all-v part of sendto_rank_to_send_buffer");

#ifdef LONG_DEBUG
  /* print sendto_rank_to_send_buffer */
  debug_str = (instance_id == 0 && instance_proc_rank==0) ? "DEBUG: find_nbrs: 9: sendto_rank_to_send_buffer [ \n" : "";
  debug_str.append("  r").append(std::to_string(instance_proc_rank)).append("[ ");
  int recvfrom_rank_count = (int) recvfrom_rank_to_msgcount_and_destined_labels->size();
  int recv_msg_count = 0;
  for (const auto &kv : (*recvfrom_rank_to_msgcount_and_destined_labels)){
    recv_msg_count += (*kv.second)[0];
  }
  int sendto_rank_count = (int) sendto_rank_to_msgcount_and_destined_labels->size();
  int send_msg_count = 0;
  for (const auto &kv : (*sendto_rank_to_msgcount_and_destined_labels)){
    send_msg_count += (*kv.second)[0];
  }

  debug_str.append(std::to_string(recvfrom_rank_count)).append(" ")
      .append(std::to_string(recv_msg_count)).append(" ")
      .append(std::to_string(sendto_rank_count)).append(" ")
      .append(std::to_string(send_msg_count));
  debug_str.append(" ]\n");
  debug_str = mpi_gather_string(debug_str);
  if (instance_id == 0 && world_proc_rank == 0){
    std::cout<<std::endl<<std::string(debug_str).append("]")<<std::endl;
  }
#endif
  // END ================

  delete outrank_to_offset_factor;
  delete sendto_rank_to_msgcount_and_destined_labels;
  delete [] global_vertex_labels;
  delete [] local_vertex_displas;
  delete [] local_vertex_counts;
  delete label_to_vertex;
}

void parallel_ops::print_timing(
    const std::chrono::time_point<std::chrono::high_resolution_clock> &start_ms,
    const std::chrono::time_point<std::chrono::high_resolution_clock> &end_ms,
    const std::string &msg) const {
  double duration_ms, avg_duration_ms, min_duration_ms, max_duration_ms;
  duration_ms = (ms_t(end_ms - start_ms)).count();
  MPI_Reduce(&duration_ms, &min_duration_ms, 1, MPI_DOUBLE, MPI_MIN, 0, MPI_COMM_INSTANCE);
  MPI_Reduce(&duration_ms, &max_duration_ms, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_INSTANCE);
  MPI_Reduce(&duration_ms, &avg_duration_ms, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_INSTANCE);
  if (instance_id == 0 && instance_proc_rank == 0){
    std::cout<<msg<<" [min max avg]ms: ["<< min_duration_ms
             << " " << max_duration_ms << " " << (avg_duration_ms / instance_procs_count) << "]" <<std::endl;
  }
}

std::string parallel_ops::mpi_gather_string(std::string &str) {
  int len = (int) str.length();
  int *lengths = new int[instance_procs_count];

  MPI_Allgather(&len, 1, MPI_INT, lengths, 1, MPI_INT, MPI_COMM_INSTANCE);

  int *displas = new int[instance_procs_count];
  int total_length = lengths[0];
  displas[0] = 0;
  for (int i = 1; i < instance_procs_count; ++i){
    displas[i] = displas[i-1]+lengths[i-1];
    total_length += lengths[i];
  }
  char *result = new char[total_length];
  MPI_Gatherv(str.c_str(), len, MPI_CHAR, result, lengths, displas, MPI_CHAR, 0, MPI_COMM_INSTANCE);

  std::string r = std::string(result);

  delete [] result;
  delete [] displas;
  delete [] lengths;
  return r;
}

int parallel_ops::read_int(long byte_idx, char *f) {
  return (((unsigned char) f[byte_idx + 3] << 0))
         + ((unsigned char) (f[byte_idx + 2] << 8))
         + ((unsigned char) (f[byte_idx + 1] << 16))
         + ((unsigned char) (f[byte_idx + 0] << 24));

}

void parallel_ops::update_counts_and_displas(int msg_size) {
  for (int i = 0; i < instance_procs_count; ++i){
    scounts[i] *= msg_size;
    rcounts[i] *= msg_size;
    sdisplas[i] *= msg_size;
    rdisplas[i] *= msg_size;
  }

}

void parallel_ops::all_to_all_v() {
  MPI_Alltoallv(sbuff.get(), scounts, sdisplas, MPI_SHORT, rbuff.get(), rcounts, rdisplas, MPI_SHORT, MPI_COMM_INSTANCE);
}

void parallel_ops::simple_graph_partition_binary(const char *file, int global_vertex_count, int global_edge_count,
                                                 std::vector<std::shared_ptr<vertex>> *&vertices) {
  std::chrono::time_point<std::chrono::high_resolution_clock > start, end;
  start = std::chrono::high_resolution_clock::now();

  int q = global_vertex_count/instance_procs_count;
  int r = global_vertex_count % instance_procs_count;
  int local_vertex_count = (instance_proc_rank < r) ? q+1: q;
  my_vertex_count = local_vertex_count;
  int skip_vertex_count = q*instance_proc_rank + (instance_proc_rank < r ? instance_proc_rank : r);

#ifndef NDEBUG
  std::string debug_str = (instance_id == 0 && instance_proc_rank==0) ? "DEBUG: simple_graph_partition_binary: 1: q,r,localvc  [ " : " ";
  debug_str.append("[").append(std::to_string(q)).append(",").append(std::to_string(r)).append(",").append(std::to_string(local_vertex_count)).append("] ");
  debug_str = mpi_gather_string(debug_str);
  if (instance_id == 0 && instance_proc_rank == 0){
    std::cout<<std::endl<<std::string(debug_str).append("]")<<std::endl;
  }
#endif

  vertices = new std::vector<std::shared_ptr<vertex>>((unsigned long) local_vertex_count);

  std::ifstream fs(file, std::ios::in | std::ios::binary);

  int int_bytes = sizeof(int);
  // each record is a tuple of integers (node-id, #nbrs+1), +1 is because in data we store node-weight as well
  int header_record_size = 2;
  long header_count = global_vertex_count * header_record_size;
  long header_extent = header_count*int_bytes;
  char *header = new char[header_extent];

  fs.read(header, header_extent);

  long data_offset = 0L;
  long data_extent = 0L;
  for (int i = 0; i < global_vertex_count; ++i){
    if (skip_vertex_count == i){
      break;
    }
    // RHS is #nbrs + 1 (for weight) + 1 (for node-id)
    // #nbrs+1 is what you read from the header entry (i*header_record_size+1)
    // This is the record length for vertex i in the binary file
    // Think of this as the total number of splits for row i in the text version of the graph file
    data_offset += read_int((i*header_record_size+1)*int_bytes, header)+1;
  }
  data_offset *= int_bytes;

  int my_vertex_start_offset_in_header = skip_vertex_count*header_record_size;
  int *vertex_nbr_length = new int[my_vertex_count];
  int *out_nbrs = new int[global_vertex_count];
  long running_extent;
  long read_extent = 0L;
  int read_vertex = -1;
  for (int i = 0; i < my_vertex_count; ++i){
    // my ith vertex's #nbrs+1 (+1 is for weight)
    // the header entry (my_vertex_start_offset_in_header+1) contains #nbrs+1 value
    int len = read_int((my_vertex_start_offset_in_header+i*header_record_size+1)*int_bytes, header);
    vertex_nbr_length[i] = len-1;

    running_extent = data_extent + ((long)len) + 1;
    if (running_extent*int_bytes <= INT32_MAX){
      data_extent = running_extent;
    } else {
      data_extent = read_vertices(vertices, skip_vertex_count, fs, header_extent, data_offset,
                                  data_extent, vertex_nbr_length, out_nbrs, read_extent, read_vertex, i);
      read_extent += data_extent;
      data_extent = (long)(len +1);
      read_vertex = (i-1);
    }
  }
  read_vertices(vertices, skip_vertex_count, fs, header_extent, data_offset,
                data_extent, vertex_nbr_length, out_nbrs, read_extent, read_vertex, my_vertex_count);

  end = std::chrono::high_resolution_clock::now();
  print_timing(start, end, "simple_graph_partition_binary: graph_read");


  start = std::chrono::high_resolution_clock::now();
  find_nbrs(global_vertex_count, local_vertex_count, vertices);
  end = std::chrono::high_resolution_clock::now();
  print_timing(start, end, "simple_graph_partition_binary: find_nbrs total");

  fs.close();
  delete [] out_nbrs;
  delete [] vertex_nbr_length;
  delete [] header;
}

long
parallel_ops::read_vertices(std::vector<std::shared_ptr<vertex>> *vertices, int skip_vertex_count, std::ifstream &fs,
                            long header_extent, long data_offset, long data_extent, int *vertex_nbr_length,
                            int *out_nbrs, long read_extent, int read_vertex, int i) {
  int int_bytes = sizeof(int);
  data_extent *= int_bytes;
  char *data = new char[data_extent];
  fs.seekg(data_offset+header_extent+read_extent);
  fs.read(data, data_extent);
  long idx = 0;
  for (int j = read_vertex+1; j < i; ++j){
    int vertex_label = read_int((idx++)*int_bytes, data);
    double vertex_weight = read_int((idx++)*int_bytes, data);
    for (int k = 0; k < vertex_nbr_length[j]; ++k){
      out_nbrs[k] = read_int((idx++)*int_bytes, data);
    }
    (*vertices)[j] = std::make_shared<vertex>(vertex_label, vertex_weight, out_nbrs, vertex_nbr_length[j]);
  }

  delete [] data;
  return data_extent;
}

void parallel_ops::append_timings(double t, int at_rank, std::string &str) {
  MPI_Gather(&t, 1, MPI_DOUBLE, times.get(), 1, MPI_DOUBLE, at_rank, MPI_COMM_WORLD);
  str.append("[ ");
  for (int i = 0; i < world_procs_count; ++i){
    str.append(std::to_string(times.get()[i])).append(" ");
  }
  str.append("]\n");
}
