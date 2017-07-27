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
  delete recvfrom_rank_to_recv_buffer;
  recvfrom_rank_to_recv_buffer = nullptr;
  delete sendto_rank_to_send_buffer;
  sendto_rank_to_send_buffer = nullptr;
}

parallel_ops::parallel_ops(int world_proc_rank, int world_procs_count) :
    world_proc_rank(world_proc_rank),
    world_procs_count(world_procs_count) {
}

int parallel_ops::get_world_proc_rank() const {
  return world_proc_rank;
}

int parallel_ops::get_world_procs_count() const {
  return world_procs_count;
}

void parallel_ops::set_parallel_decomposition(const char *file, int global_vertx_count, std::vector<std::shared_ptr<vertex>> *&vertices) {
  // TODO - add logic to switch between different partition methods as well as txt vs binary files
  // for now let's assume simple partitioning with text files
  simple_graph_partition(file, global_vertx_count, vertices);
  decompose_among_threads(vertices);
}

void parallel_ops::simple_graph_partition(const char *file, int global_vertex_count, std::vector<std::shared_ptr<vertex>> *&vertices) {
  std::chrono::time_point<std::chrono::high_resolution_clock > start, end;

  int q = global_vertex_count/world_procs_count;
  int r = global_vertex_count % world_procs_count;
  int local_vertex_count = (world_proc_rank < r) ? q+1: q;
  int skip_vertex_count = q*world_proc_rank + (world_proc_rank < r ? world_proc_rank : r);

#ifdef LONG_DEBUG
  std::string debug_str = (world_proc_rank==0) ? "DEBUG: simple_graph_partition: 1: q,r,localvc  [ " : " ";
  debug_str.append("[").append(std::to_string(q)).append(",").append(std::to_string(r)).append(",").append(std::to_string(local_vertex_count)).append("] ");
  debug_str = mpi_gather_string(debug_str);
  if (world_proc_rank == 0){
    std::cout<<std::endl<<std::string(debug_str).append("]")<<std::endl;
  }
//  std::cout<<"Rank: " << world_proc_rank << " q: " << q << " r: " << r << " local_vertex_count: " << local_vertex_count << std::endl;
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

void parallel_ops::decompose_among_threads(std::vector<std::shared_ptr<vertex>> *&vertices) {
  int length = (int) vertices->size();
  int p = length / thread_count;
  int q = length % thread_count;
  thread_id_to_vertex_offset = std::shared_ptr<int>(new int[thread_count](), std::default_delete<int[]>());
  thread_id_to_vertex_count = std::shared_ptr<int>(new int[thread_count](), std::default_delete<int[]>());
  for (int i = 0; i < thread_count; ++i){
    thread_id_to_vertex_count.get()[i] = (i < q) ? (p+1) : p;
  }
  thread_id_to_vertex_offset.get()[0] = 0;
  for (int i = 1; i < thread_count; ++i){
    thread_id_to_vertex_offset.get()[i]
        = thread_id_to_vertex_offset.get()[i-1]
          + thread_id_to_vertex_count.get()[i-1];
  }
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
  std::string debug_str = (world_proc_rank==0) ? "DEBUG: find_nbrs: 1: lastvertex [ " : " ";
  debug_str.append(std::to_string((*label_to_vertex)[tmp_lbl]->label)).append(" ");
  debug_str = mpi_gather_string(debug_str);
  if (world_proc_rank == 0){
    std::cout<<std::endl<<std::string(debug_str).append("]")<<std::endl;
  }
#endif

  int *local_vertex_counts = new int[world_procs_count];
  MPI_Allgather(&local_vertex_count, 1, MPI_INT, local_vertex_counts, 1, MPI_INT, MPI_COMM_WORLD);

  my_vertex_count = local_vertex_count;
#ifdef LONG_DEBUG
  /* Check local vertex counts */
  debug_str = (world_proc_rank==0) ? "DEBUG: find_nbrs: 2: vcount [ " : " ";
  for (int i = 0; i < world_procs_count; ++i) {
    debug_str.append(std::to_string(local_vertex_counts[i])).append(" ");
  }
  if (world_proc_rank == 0){
    std::cout<<std::endl<<std::string(debug_str).append("]")<<std::endl;
  }
#endif

  int *local_vertex_displas = new int[world_procs_count];
  local_vertex_displas[0] = 0;
  for (int i = 1; i < world_procs_count; ++i){
    local_vertex_displas[i] = local_vertex_displas[i-1]+local_vertex_counts[i-1];
  }

  my_vertex_displas = local_vertex_displas[world_proc_rank];

#ifdef LONG_DEBUG
  /* Check local vertex displas */
  debug_str = (world_proc_rank==0) ? "DEBUG: find_nbrs: 3: vdisplas [ " : " ";
  for (int i = 0; i < world_procs_count; ++i) {
    debug_str.append(std::to_string(local_vertex_displas[i])).append(" ");
  }
  if (world_proc_rank == 0){
    std::cout<<std::endl<<std::string(debug_str).append("]")<<std::endl;
  }
#endif

  start_ms = std::chrono::high_resolution_clock::now();
  int *global_vertex_labels = new int[global_vertex_count];
  int offset = local_vertex_displas[world_proc_rank];
  for (int i = 0; i < local_vertex_count; ++i){
    global_vertex_labels[i+offset] = (*vertices)[i]->label;
  }
  MPI_Allgatherv(MPI_IN_PLACE, local_vertex_count, MPI_INT, global_vertex_labels,
                 local_vertex_counts, local_vertex_displas, MPI_INT, MPI_COMM_WORLD);
  end_ms = std::chrono::high_resolution_clock::now();
  print_timing(start_ms, end_ms, "find_nbr: global_vertex_labels allgather");

#ifdef LONG_DEBUG
  /* Check global vertex labels */
  debug_str = (world_proc_rank==0) ? "DEBUG: find_nbrs: 4: vlabels [ \n" : " ";
  for (int i = 0; i < world_procs_count; ++i) {
    debug_str.append("  r").append(std::to_string(i)).append("[ ");
    for (int j = 0; j < local_vertex_counts[i]; ++j){
      debug_str.append(std::to_string(global_vertex_labels[local_vertex_displas[i]+j])).append(" ");
    }
    debug_str.append("]\n");
  }
  if (world_proc_rank == 0){
    std::cout<<std::endl<<std::string(debug_str).append("]")<<std::endl;
  }
#endif

  /* Just keep in mind this map and the global_vertex_labels can be really large
   * Think of optimizations if this becomes a bottleneck */
  start_ms = std::chrono::high_resolution_clock::now();
  vertex_label_to_world_rank = std::make_shared<std::map<int,int>>();
  std::map<int,int> *label_to_world_rank = new std::map<int,int>();
  for (int rank = 0; rank < world_procs_count; ++rank){
    for (int i = 0; i < local_vertex_counts[rank]; ++i){
      offset = local_vertex_displas[rank];
      (*vertex_label_to_world_rank)[global_vertex_labels[i + offset]] = rank;
    }
  }
  end_ms = std::chrono::high_resolution_clock::now();
  print_timing(start_ms, end_ms, "find_nbr: label_to_world_rank map creation");

  /* Set where out-neighbors of vertices live */
  start_ms = std::chrono::high_resolution_clock::now();
  for (const std::shared_ptr<vertex> &v : (*vertices)){
    std::map<int, int> *outnbr_label_to_world_rank = v->outnbr_lbl_to_world_rank;
    for (const auto &kv : (*outnbr_label_to_world_rank)){
      int rank = (*vertex_label_to_world_rank)[kv.first];
      (*outnbr_label_to_world_rank)[kv.first] = rank;
      (*v->outrank_to_send_buffer)[rank] = std::make_shared<vertex_buffer>();
    }
  }
  end_ms = std::chrono::high_resolution_clock::now();
  print_timing(start_ms, end_ms, "find_nbr: set outnbr_label_to_world_rank for each my v");

#ifdef LONG_DEBUG
  debug_str = (world_proc_rank==0) ? "DEBUG: find_nbrs: 5: out_nbrs [ \n" : " ";
  debug_str.append("  r").append(std::to_string(world_proc_rank)).append("[\n");
  for (const std::shared_ptr<vertex> &v : (*vertices)){
    std::map<int, int> *outnbr_label_to_world_rank = v->outnbr_lbl_to_world_rank;
    debug_str.append("    v").append(std::to_string(v->label)).append("[\n");
    for (const auto &kv : (*outnbr_label_to_world_rank)){
      debug_str.append("      nbr").append(std::to_string(kv.first)).append("->r").append(std::to_string(kv.second)).append("\n");
    }
    debug_str.append("    ]\n");
  }
  debug_str = mpi_gather_string(debug_str);
  if (world_proc_rank == 0){
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
    // TODO - possibly this map can be reused by doing erase() on each loop instance
    // inverse_map is essentially a mapping from ranks this particular vertex
    // needs to communicate to the neighbor vertices that reside on those ranks
    std::map<int, std::shared_ptr<std::vector<int>>> *inverse_map
        = new std::map<int, std::shared_ptr<std::vector<int>>>();
    // Populate inverse_map
    for (const auto &kv : (*v->outnbr_lbl_to_world_rank)){
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
  end_ms = std::chrono::high_resolution_clock::now();
  print_timing(start_ms, end_ms, "find_nbr: sendto_rank_to_msgcount_and_destined_labels");

#ifdef LONG_DEBUG
  /* print how many message counts and what are destined vertex labels (in order) for each rank from me */
  debug_str = (world_proc_rank==0) ? "DEBUG: find_nbrs: 6: msg_counts_and_labels [ \n" : " ";
  debug_str.append("  r").append(std::to_string(world_proc_rank))
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
  if (world_proc_rank == 0){
    std::cout<<std::endl<<std::string(debug_str).append("]")<<std::endl;
  }
#endif
  // END ----------------

  // BEGIN ################
  start_ms = std::chrono::high_resolution_clock::now();
  int max_buffer_size = -1;
  MPI_Allreduce(&msg_size, &max_buffer_size, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD);
  max_buffer_size += 1; // +1 to send msgSize
  recvfrom_rank_to_msgcount_and_destined_labels
      = new std::map<int, std::shared_ptr<std::vector<int>>>();
  int *buffer = new int[max_buffer_size];
  for (int rank = 0; rank < world_procs_count; ++rank){
    if (rank == world_proc_rank){
      buffer[0] = msg_size;
      int idx = 1;
      for (const auto &kv : (*sendto_rank_to_msgcount_and_destined_labels)){
        buffer[idx++] = -1 * (kv.first + global_vertex_count + 1);
        for (const auto &val : (*kv.second)){
          buffer[idx++] = val;
        }
      }
    }
    MPI_Bcast(buffer, max_buffer_size, MPI_INT, rank, MPI_COMM_WORLD);
    int recv_msg_size = buffer[0];

    for (int i = 1; i <= recv_msg_size; ++i) {
      int val = buffer[i];
      if (val < 0 && val < -1 * (global_vertex_count)) {
        // It's the rank information
        int destined_rank = (-1 * val) - (global_vertex_count + 1);
        if (destined_rank == world_proc_rank) {
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
  debug_str = (world_proc_rank==0) ? "DEBUG: find_nbrs: 7: msg_counts_and_labels [ \n" : " ";
  debug_str.append("  r").append(std::to_string(world_proc_rank)).append("[\n");
  for (const auto &pair : (*recvfrom_rank_to_msgcount_and_destined_labels)){
    std::shared_ptr<std::vector<int>> list = pair.second;
    debug_str.append("    recvs ").append(std::to_string((*list)[0])).append(" msgs from rank ").append(std::to_string(pair.first)).append(" destined labels [ ");
    for (auto it = ++(list->begin()); it != list->end(); ++it){
      debug_str.append(std::to_string(*it)).append(" ");
    }
    debug_str.append("]\n");
  }
  debug_str = mpi_gather_string(debug_str);
  if (world_proc_rank == 0){
    std::cout<<std::endl<<std::string(debug_str).append("]")<<std::endl;
  }
#endif
  // END ################

  // BEGIN ~~~~~~~~~~~~~~~~
  start_ms = std::chrono::high_resolution_clock::now();
  recvfrom_rank_to_recv_buffer = new std::map<int, std::shared_ptr<short>>();
  for (const auto &kv : (*recvfrom_rank_to_msgcount_and_destined_labels)){
    int recvfrom_rank = kv.first;
    std::shared_ptr<std::vector<int>> count_and_destined_vertex_labels = kv.second;
    int msg_count = (*count_and_destined_vertex_labels)[0];
    std::shared_ptr<short> b = std::shared_ptr<short>(
        new short [BUFFER_OFFSET + msg_count * max_msg_size](), std::default_delete<short[]>());
    (*recvfrom_rank_to_recv_buffer)[recvfrom_rank] = b;
    int current_msg = 0;
    for (int i = 1; i < count_and_destined_vertex_labels->size(); ){
      int val = (*count_and_destined_vertex_labels)[i];
      if (val >= 0){
        std::shared_ptr<vertex> vertex = (*label_to_vertex)[val];
        vertex->recv_buffers->push_back(
            std::make_shared<recv_vertex_buffer>(current_msg, b, recvfrom_rank, MSG_SIZE_OFFSET));
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
              std::make_shared<recv_vertex_buffer>(current_msg, b, recvfrom_rank, MSG_SIZE_OFFSET));
          vertex->recvd_msgs->push_back(std::make_shared<message>());
        }
        i+=intended_vertex_count+1;
        ++current_msg;
      }
    }
  }
  end_ms = std::chrono::high_resolution_clock::now();
  print_timing(start_ms, end_ms, "find_nbr: recvfrom_rank_to_recv_buffer");

#ifdef LONG_DEBUG
  /* print recvfrom_rank_to_recv_buffer */
  debug_str = (world_proc_rank==0) ? "DEBUG: find_nbrs: 8: recvfrom_rank_to_recv_buffer [ \n" : "";
  debug_str.append("  r").append(std::to_string(world_proc_rank)).append("[\n");
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
  if (world_proc_rank == 0){
    std::cout<<std::endl<<std::string(debug_str).append("]")<<std::endl;
  }
#endif
  // END ~~~~~~~~~~~~~~~~

  // BEGIN ================
  start_ms = std::chrono::high_resolution_clock::now();
  sendto_rank_to_send_buffer = new std::map<int, std::shared_ptr<short>>();
  for (const auto &kv : (*sendto_rank_to_msgcount_and_destined_labels)){
    int sendto_rank = kv.first;
    std::shared_ptr<std::vector<int>> count_and_destined_vertex_labels = kv.second;
    int msg_count = (*count_and_destined_vertex_labels)[0];
    std::shared_ptr<short> b = std::shared_ptr<short>(
        new short[BUFFER_OFFSET + msg_count * max_msg_size](), std::default_delete<short[]>());
    // msg_count is an integer, so we'll pack it as two short values
    // to reconstruct the integer, do (firstHalf << 16) | (secondHalf & 0xffff)
    b.get()[MSG_COUNT_OFFSET] = (short) (msg_count >> 16);
    b.get()[MSG_COUNT_OFFSET+1] = (short) (msg_count & 0xffff);
    (*sendto_rank_to_send_buffer)[sendto_rank] = b;
  }

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
      vertex_send_buffer->set_offset_factor((*outrank_to_offset_factor)[outrank]);
      vertex_send_buffer->set_buffer((*sendto_rank_to_send_buffer)[outrank]);
    }
  }
  end_ms = std::chrono::high_resolution_clock::now();
  print_timing(start_ms, end_ms, "find_nbr: sendto_rank_to_send_buffer");

#ifdef LONG_DEBUG
  /* print sendto_rank_to_send_buffer */
  debug_str = (world_proc_rank==0) ? "DEBUG: find_nbrs: 9: sendto_rank_to_send_buffer [ \n" : "";
  debug_str.append("  r").append(std::to_string(world_proc_rank)).append("[ ");
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
  if (world_proc_rank == 0){
    std::cout<<std::endl<<std::string(debug_str).append("]")<<std::endl;
  }
#endif
  // END ================

  int num_sendto_ranks = (int) ((sendto_rank_to_send_buffer->find(world_proc_rank) != sendto_rank_to_send_buffer->end())
                           ? sendto_rank_to_send_buffer->size()-1
                           : sendto_rank_to_send_buffer->size());
  int num_recvfrom_ranks = (int) ((recvfrom_rank_to_recv_buffer->find(world_proc_rank) != recvfrom_rank_to_recv_buffer->end())
                           ? recvfrom_rank_to_recv_buffer->size() - 1
                           : recvfrom_rank_to_recv_buffer->size());
  recv_req_offset = num_sendto_ranks;
  total_reqs = num_sendto_ranks+num_recvfrom_ranks;
  send_recv_reqs = new MPI_Request[total_reqs]();
  send_recv_reqs_status = new MPI_Status[total_reqs]();

  delete outrank_to_offset_factor;
  delete sendto_rank_to_msgcount_and_destined_labels;
  delete [] global_vertex_labels;
  delete [] local_vertex_displas;
  delete [] local_vertex_counts;
  delete label_to_vertex;
}

void parallel_ops::test_string_allreduce() {/* Allreduce string test */
  std::string debug_str = "[hello";
  if (world_proc_rank == 0){
    debug_str = debug_str.append("wonderful");
  }
  debug_str.append(std::to_string(world_proc_rank)).append("]");
  debug_str = mpi_gather_string(debug_str);
  if (world_proc_rank == 0){
    std::cout << std::endl << std::string(debug_str) << std::endl;
  }
}

void parallel_ops::test_isend_irecv() {/* test ISend/IRecv */
  int *buff = new int[2]();
  buff[0] = world_proc_rank;
  int recvfrom_rank = world_proc_rank - 1;
  if (recvfrom_rank < 0){
    recvfrom_rank = world_procs_count - 1;
  }

  MPI_Request *requests = new MPI_Request[2]();
  MPI_Irecv(&buff[1], 1, MPI_INT, recvfrom_rank, 99, MPI_COMM_WORLD, &requests[1]);

  int sendto_rank = world_proc_rank + 1;
  if (sendto_rank == world_procs_count) {
    sendto_rank = 0;
  }

  MPI_Isend(&buff[0], 1, MPI_INT, sendto_rank, 99, MPI_COMM_WORLD, &requests[0]);

  MPI_Status *status = new MPI_Status[2]();
  MPI_Waitall(2, requests, status);

  std::string debug_str = (world_proc_rank == 0) ? "DEBUG: find_nbrs: 10: ISend/Irecv [ \n" : "";
  debug_str.append("  r").append(std::to_string(world_proc_rank)).append("[ ");
  debug_str.append(std::to_string(buff[0])).append(" ").append(std::to_string(buff[1]));
  debug_str.append(" ]\n");
  debug_str = mpi_gather_string(debug_str);
  if (world_proc_rank == 0){
    std::cout << std::endl << std::string(debug_str).append("]") << std::endl;
  }

  delete [] status;
  delete [] requests;
  delete [] buff;
}

void parallel_ops::print_timing(
    const std::chrono::time_point<std::chrono::high_resolution_clock> &start_ms,
    const std::chrono::time_point<std::chrono::high_resolution_clock> &end_ms,
    const std::string &msg) const {
  double duration_ms, avg_duration_ms, min_duration_ms, max_duration_ms;
  duration_ms = (ms_t(end_ms - start_ms)).count();
  MPI_Reduce(&duration_ms, &min_duration_ms, 1, MPI_DOUBLE, MPI_MIN, 0, MPI_COMM_WORLD);
  MPI_Reduce(&duration_ms, &max_duration_ms, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
  MPI_Reduce(&duration_ms, &avg_duration_ms, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
  if (world_proc_rank == 0){
    std::cout<<msg<<" [min max avg]ms: ["<< min_duration_ms
             << " " << max_duration_ms << " " << (avg_duration_ms / world_procs_count) << "]" <<std::endl;
  }
}

std::string parallel_ops::mpi_gather_string(std::string &str) {
  int len = (int) str.length();
  int *lengths = new int[world_procs_count];

  MPI_Allgather(&len, 1, MPI_INT, lengths, 1, MPI_INT, MPI_COMM_WORLD);

  int *displas = new int[world_procs_count];
  int total_length = lengths[0];
  displas[0] = 0;
  for (int i = 1; i < world_procs_count; ++i){
    displas[i] = displas[i-1]+lengths[i-1];
    total_length += lengths[i];
  }
  char *result = new char[total_length];
  MPI_Gatherv(str.c_str(), len, MPI_CHAR, result, lengths, displas, MPI_CHAR, 0, MPI_COMM_WORLD);

  std::string r = std::string(result);

  delete [] result;
  delete [] displas;
  delete [] lengths;
  return r;
}

void parallel_ops::send_msgs(int msg_size) {
  msg_size_to_recv = msg_size;
  int req_count = 0;
  for (const auto &kv : (*sendto_rank_to_send_buffer)){
    int sendto_rank = kv.first;
    std::shared_ptr<short> buffer = kv.second;
    buffer.get()[MSG_SIZE_OFFSET] = (short)msg_size;
    int msg_count = ((buffer.get()[MSG_COUNT_OFFSET]) << 16 | (buffer.get()[MSG_COUNT_OFFSET+1] & 0xffff));
    // This is different from buffer size, which is
    // BUFFER_OFFSET + msg_count * max_msg_size
    // Notice here we use msg_size instead of max_msg_size
    int buffer_content_size = BUFFER_OFFSET + msg_count * msg_size;

    if (sendto_rank == world_proc_rank){
      // local copy
      std::shared_ptr<short> b = (*recvfrom_rank_to_recv_buffer)[world_proc_rank];
      std::copy(buffer.get(), buffer.get()+buffer_content_size, b.get());
    } else {
      MPI_Isend(buffer.get(), buffer_content_size, MPI_SHORT, sendto_rank, world_proc_rank, MPI_COMM_WORLD, &send_recv_reqs[req_count]);
      ++req_count;
    }
  }
}

void parallel_ops::recv_msgs() {
  int req_count = 0;
  for (const auto &kv : (*recvfrom_rank_to_recv_buffer)){
    int recvfrom_rank = kv.first;
    std::shared_ptr<short> buffer = kv.second;
    int msg_count = (*(*recvfrom_rank_to_msgcount_and_destined_labels)[recvfrom_rank])[0];
    if (recvfrom_rank != world_proc_rank){
      MPI_Irecv(buffer.get(), BUFFER_OFFSET + msg_count * msg_size_to_recv,
                MPI_SHORT, recvfrom_rank, recvfrom_rank, MPI_COMM_WORLD,
                &send_recv_reqs[req_count+recv_req_offset]);
      ++req_count;
    }
  }

  MPI_Waitall(total_reqs, send_recv_reqs, send_recv_reqs_status);
}
