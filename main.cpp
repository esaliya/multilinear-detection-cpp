#include <iostream>
#include <vector>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/option.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/program_options/parsers.hpp>
#include <fstream>
#include "parallel_ops.hpp"
#include "constants.h"
#include "utils.hpp"
#include "polynomial.hpp"

typedef std::chrono::duration<double, std::milli> ms_t;
typedef std::chrono::duration<double, std::micro> micros_t;
typedef std::chrono::time_point<std::chrono::high_resolution_clock> ticks_t;

typedef std::chrono::high_resolution_clock hrc_t;
namespace po = boost::program_options;

int parse_args(int argc, char **argv);
void run_program(std::vector<std::shared_ptr<vertex>> *vertices);
void init_comp(std::vector<std::shared_ptr<vertex>> *vertices);
bool run_graph_comp(int loop_id, std::vector<std::shared_ptr<vertex>> *vertices);
void init_loop(std::vector<std::shared_ptr<vertex>> *vertices);
void run_super_steps(std::vector<std::shared_ptr<vertex>> *vertices, int local_iter, int global_iter);
void compute(int iter, std::vector<std::shared_ptr<vertex>> *vertices, int super_step);
void recv_msgs(std::vector<std::shared_ptr<vertex>> *vertices, int super_step);
void process_recvd_msgs(std::vector<std::shared_ptr<vertex>> *vertices, int super_step);
void send_msgs(std::vector<std::shared_ptr<vertex>> *vertices, int super_step);
void finalize_iteration(std::vector<std::shared_ptr<vertex>> *vertices);
bool finalize_iterations(std::vector<std::shared_ptr<vertex>> *vertices);

void pretty_print_config(std::string &str);
int log2(int x);
void print_timing(const double duration,const std::string &msg);
void print_timing_global(const double duration,const std::string &msg);

int global_vertex_count;
int global_edge_count;
int k;
int r; // not used in k-path problem
int delta;
double alpha;
double epsilon;
std::string input_file;
std::string out_file;
std::string partition_file;

int two_raised_to_k;
std::shared_ptr<galois_field> gf = nullptr;
int max_iterations;
std::shared_ptr<std::map<int,int>> random_assignments = nullptr;
std::shared_ptr<int> completion_vars = nullptr;

// default values;
int node_count = 1;
int thread_count = 1;
int max_msg_size = 500;
int parallel_instance_count = 1;
int iter_bs = 1;
int is_binary = 0;

bool is_print_rank = false;

parallel_ops *p_ops;

double times[5] = {0};
/*
void poly_test(){
  std::shared_ptr<polynomial> p = std::make_shared<polynomial>();
  p->degrees->insert(4);
  p->degrees->insert(3);
  std::cout<<*p<<std::endl;

  std::shared_ptr<polynomial> q = std::make_shared<polynomial>();
  q->degrees->insert(4);
  q->degrees->insert(3);
  std::cout<<*p<<std::endl;

  std::shared_ptr<polynomial> p_xor_q = p->poly_xor(q);
  std::cout<<*p_xor_q<<std::endl;

}
*/

int main(int argc, char **argv) {
  p_ops = parallel_ops::initialize(&argc, &argv);
  int ret = parse_args(argc, argv);
  if (ret < 0){
    p_ops->teardown_parallelism();
    return ret;
  }

  std::vector<std::shared_ptr<vertex>> *vertices = nullptr;
  p_ops->set_parallel_decomposition(input_file.c_str(), out_file.c_str(),
                                    global_vertex_count, global_edge_count,
                                    vertices, is_binary, parallel_instance_count);

  is_print_rank = (p_ops->instance_id == 0 && p_ops->instance_proc_rank == 0);

  run_program(vertices);
  delete vertices;
  p_ops->teardown_parallelism();
  return 0;
}

int parse_args(int argc, char **argv) {
  // Declare the supported options.
  po::options_description desc(PROGRAM_NAME);
  desc.add_options()
      ("help", "produce help message")
      (CMD_OPTION_SHORT_VC, po::value<int>(), CMD_OPTION_DESCRIPTION_VC)
      (CMD_OPTION_SHORT_EC, po::value<int>(), CMD_OPTION_DESCRIPTION_EC)
      (CMD_OPTION_SHORT_K, po::value<int>(), CMD_OPTION_DESCRIPTION_K)
      (CMD_OPTION_SHORT_DELTA, po::value<int>(), CMD_OPTION_DESCRIPTION_DELTA)
      (CMD_OPTION_SHORT_ALPHA, po::value<double>(), CMD_OPTION_DESCRIPTION_ALPHA)
      (CMD_OPTION_SHORT_EPSILON, po::value<double>(), CMD_OPTION_DESCRIPTION_EPSILON)
      (CMD_OPTION_SHORT_INPUT, po::value<std::string>(), CMD_OPTION_DESCRIPTION_INPUT)
      (CMD_OPTION_SHORT_PARTS, po::value<std::string>(), CMD_OPTION_DESCRIPTION_PARTS)
      (CMD_OPTION_SHORT_NC, po::value<int>(), CMD_OPTION_DESCRIPTION_NC)
      (CMD_OPTION_SHORT_MMS, po::value<int>(), CMD_OPTION_DESCRIPTION_MMS)
      (CMD_OPTION_SHORT_PIC, po::value<int>(), CMD_OPTION_DESCRIPTION_PIC)
      (CMD_OPTION_SHORT_IBS, po::value<int>(), CMD_OPTION_DESCRIPTION_IBS)
      (CMD_OPTION_SHORT_BIN, po::value<int>(), CMD_OPTION_DESCRIPTION_BIN)
      (CMD_OPTION_SHORT_OUT, po::value<std::string>(), CMD_OPTION_DESCRIPTION_OUT)
      ;

  po::variables_map vm;
  po::store(po::parse_command_line(argc, (const char *const *) argv, desc), vm);
  po::notify(vm);

  bool is_world_rank0 = p_ops->world_proc_rank == 0;
  if (vm.count("help")) {
    std::cout << desc << "\n";
    return 1;
  }

  if (vm.count(CMD_OPTION_SHORT_VC)){
    global_vertex_count = vm[CMD_OPTION_SHORT_VC].as<int>();
  } else {
    if (is_world_rank0)
      std::cout<<"ERROR: Vertex count not specified"<<std::endl;
    return -1;
  }

  if (vm.count(CMD_OPTION_SHORT_EC)){
    global_edge_count = vm[CMD_OPTION_SHORT_EC].as<int>();
  } else {
    if (is_world_rank0)
      std::cout<<"INFO: Edge count not specified, ignoring edge count"<<std::endl;
    global_edge_count = -1;
  }

  if(vm.count(CMD_OPTION_SHORT_K)){
    k = vm[CMD_OPTION_SHORT_K].as<int>();
  } else {
    if (is_world_rank0)
      std::cout<<"ERROR: K is not specified"<<std::endl;
    return -1;
  }

  if(vm.count(CMD_OPTION_SHORT_DELTA)){
    delta = vm[CMD_OPTION_SHORT_DELTA].as<int>();
  } else {
    if (is_world_rank0)
      std::cout<<"ERROR: Delta not specified"<<std::endl;
    return -1;
  }

  if(vm.count(CMD_OPTION_SHORT_ALPHA)){
    alpha = vm[CMD_OPTION_SHORT_ALPHA].as<double>();
  } else {
    if (is_world_rank0)
      std::cout<<"ERROR: Alpha not specified"<<std::endl;
    return -1;
  }

  if(vm.count(CMD_OPTION_SHORT_EPSILON)){
    epsilon = vm[CMD_OPTION_SHORT_EPSILON].as<double>();
  } else {
    if (is_world_rank0)
      std::cout<<"ERROR: Epsilon not specified"<<std::endl;
    return -1;
  }

  if (vm.count(CMD_OPTION_SHORT_INPUT)){
    input_file = vm[CMD_OPTION_SHORT_INPUT].as<std::string>();
  }else {
    if (is_world_rank0)
      std::cout<<"ERROR: Input file not specified"<<std::endl;
    return -1;
  }

  if(vm.count(CMD_OPTION_SHORT_PARTS)){
    partition_file = vm[CMD_OPTION_SHORT_PARTS].as<std::string>().c_str();
  }else {
    if (is_world_rank0)
      std::cout<<"INFO: Partition file not specified, assuming uniform rank partitioning"<<std::endl;
  }

  if(vm.count(CMD_OPTION_SHORT_NC)){
    node_count = vm[CMD_OPTION_SHORT_NC].as<int>();
  }else {
    if (is_world_rank0)
      std::cout<<"ERROR: Node count not specified"<<std::endl;
    return -1;
  }
  p_ops->node_count = node_count;

  if(vm.count(CMD_OPTION_SHORT_MMS)){
    max_msg_size = vm[CMD_OPTION_SHORT_MMS].as<int>();
  }else {
    if (is_world_rank0)
      std::cout<<"INFO: Max message size not specified, assuming "<<max_msg_size<<std::endl;
  }

  if(vm.count(CMD_OPTION_SHORT_PIC)){
    parallel_instance_count = vm[CMD_OPTION_SHORT_PIC].as<int>();
  }else {
    if (is_world_rank0)
      std::cout<<"INFO: Parallel instance count not specified, assuming "<<parallel_instance_count<<std::endl;
  }

  if (vm.count(CMD_OPTION_SHORT_OUT)){
    out_file = vm[CMD_OPTION_SHORT_OUT].as<std::string>();
  }else {
    if (is_world_rank0)
      std::cout<<"ERROR: Output file not specified"<<std::endl;
    return -1;
  }

  if(vm.count(CMD_OPTION_SHORT_IBS)){
    iter_bs = vm[CMD_OPTION_SHORT_IBS].as<int>();
  }else {
    iter_bs = 1;
    if (is_world_rank0)
      std::cout<<"INFO: Iteration block size not specified, assuming "<<iter_bs<<std::endl;
  }
  max_msg_size = max_msg_size * iter_bs;
  p_ops->max_msg_size = max_msg_size;
  if(is_world_rank0){
    std::cout<<"INFO: Scaled max message size by iteration chunk size "<<max_msg_size<<std::endl;
  }

  if(vm.count(CMD_OPTION_SHORT_BIN)){
    is_binary = vm[CMD_OPTION_SHORT_BIN].as<int>();
  }else {
    is_binary = 0;
    if (is_world_rank0)
      std::cout<<"INFO: Is binary not specified, assuming text"<<std::endl;
  }

  return 0;
}

void run_program(std::vector<std::shared_ptr<vertex>> *vertices) {
  ticks_t start_prog = std::chrono::system_clock::now();
  std::time_t start_prog_time = std::chrono::system_clock::to_time_t(start_prog);
  std::string print_str = "\nINFO: Run program started on ";
  print_str.append(std::ctime(&start_prog_time));
  pretty_print_config(print_str);
  if (is_print_rank){
    std::cout<<print_str;
  }

  // get number of iterations for a target error bound (epsilon)
  double prob_success = 0.2;
  int external_loops = (int) round(log(epsilon) / log(1 - prob_success));

  print_str = "  INFO: ";
  print_str.append(std::to_string(external_loops)).append(" external loops will be evaluated for epsilon ")
      .append(std::to_string(epsilon)).append("\n");
  if (is_print_rank){
    std::cout<<print_str;
  }

  ticks_t start_loops = std::chrono::high_resolution_clock::now();
  bool found_path_globally_across_all_instances = false;
  init_comp(vertices);

  // Call parallel ops update count and displas
  p_ops->update_counts_and_displas(max_msg_size);

  // TODO - debug - set external_loops to 1
  external_loops = 1;
  for(int i = 0; i < external_loops; ++i){
    print_str = "  INFO: Start of external loop ";
    print_str.append(std::to_string(i+1)).append("\n");
    if (is_print_rank) std::cout<<print_str;

    ticks_t start_loop = std::chrono::high_resolution_clock::now();
    // every rank in the parallel instance knows about found_path_globally_across_all_instances
    found_path_globally_across_all_instances = run_graph_comp(i, vertices);

    ticks_t end_loop = std::chrono::high_resolution_clock::now();
    print_str = "  INFO: End of external loop ";
    print_str.append(std::to_string(i+1)).append(" duration (ms) ").
        append(std::to_string((ms_t(end_loop - start_loop)).count())).append("\n");
    if(is_print_rank) std::cout<<print_str;

    /* Timings are like this
   * times[0] += recv_time_ms;
   * times[1] += process_recvd_time_ms;
   * times[2] += comp_time_ms;
   * times[3] += send_time_ms;
   * times[4] += finalize_iter_time_ms;
   */
    print_timing_global(times[2], "    comp all iter: ");
    print_timing_global(times[0], "    comm all iter: ");
    print_timing_global(times[1]+times[3], "    mem copy all iter: ");
    print_timing_global(times[2]+times[0]+times[1]+times[3], "    Total all iter: ");

    if (found_path_globally_across_all_instances){
      break;
    }
  }

  ticks_t end_loops = std::chrono::high_resolution_clock::now();
  print_str = "  INFO: Graph ";
  print_str.append(found_path_globally_across_all_instances ? "contains " : "does not contain ").append("a ");
  print_str.append(std::to_string(k)).append("-path");
  if (is_print_rank) std::cout<<print_str<<std::endl;

  print_str = "  INFO: External loops total time (ms) ";
  print_str.append(std::to_string((ms_t(end_loops - start_loops)).count())).append("\n");

  ticks_t end_prog = std::chrono::high_resolution_clock::now();
  std::time_t end_prog_time = std::chrono::system_clock::to_time_t(end_prog);

  print_str.append("INFO: Run program ended on ");
  print_str.append(std::ctime(&end_prog_time));

  if(is_print_rank){
    std::cout<<print_str;
  }
}

void init_comp(std::vector<std::shared_ptr<vertex>> *vertices) {
  two_raised_to_k = 1 << k;
  max_iterations = k - 1;
  random_assignments = std::make_shared<std::map<int,int>>();
  completion_vars = std::shared_ptr<int>(new int[k-1](), std::default_delete<int[]>());
}

bool run_graph_comp(int loop_id, std::vector<std::shared_ptr<vertex>> *vertices) {
  std::string gap = "    ";

  ticks_t start_ticks = hrc_t::now();
  init_loop(vertices);
  ticks_t running_ticks = hrc_t::now();

  std::string print_str = gap;
  print_str.append("INFO: Init loop duration (ms) ");
  print_str.append(std::to_string((ms_t(running_ticks - start_ticks)).count())).append("\n");
  if(is_print_rank) std::cout<<print_str;

  // assume twoRaisedToK can be divisible by ParallelOps.parallelInstanceCount
  int iterations_per_parallel_instance = two_raised_to_k / parallel_instance_count;

  print_str = gap;
  print_str.append("INFO: Parallel instance ").append(std::to_string(p_ops->instance_id))
      .append(" starting [").append(std::to_string(iterations_per_parallel_instance))
      .append("/").append(std::to_string(two_raised_to_k)).append("] iterations").append("\n");
  if(is_print_rank) std::cout<<print_str;

  ticks_t iterations_ticks = hrc_t::now();
  // Assume iterations_per_parallel_instance is a multiple of iter_bs
  assert(iterations_per_parallel_instance % iter_bs == 0);

  for (int iter = 0; iter < iterations_per_parallel_instance; iter += iter_bs){

    ticks_t iter_ticks = std::chrono::high_resolution_clock::now();
    int final_iter = iter+(p_ops->instance_id*iterations_per_parallel_instance);

    print_str = gap;
    print_str.append("  INFO: Starting iterations[[").append(std::to_string(final_iter+1))
        .append("-").append(std::to_string(final_iter+iter_bs)).append("]/")
        .append(std::to_string(two_raised_to_k)).append("]\n");
    if (is_print_rank) std::cout<<print_str;

    run_super_steps(vertices, iter, final_iter);
    running_ticks = hrc_t::now();

    print_str = gap;
    print_str.append("  INFO: Iterations range [[").append(std::to_string(final_iter+1))
        .append("-").append(std::to_string(final_iter+iter_bs)).append("]")
        .append("/").append(std::to_string(two_raised_to_k))
        .append("] duration (ms) ").append(std::to_string(ms_t(running_ticks - iter_ticks).count())).append("\n");
    if (is_print_rank) std::cout<<print_str;
  }
  running_ticks = hrc_t::now();

  print_str = gap;
  print_str.append("INFO: Parallel instance ").append(std::to_string(p_ops->instance_id))
      .append(" ended [").append(std::to_string(iterations_per_parallel_instance))
      .append("/").append(std::to_string(two_raised_to_k)).append("] iterations, duration (ms)")
      .append(std::to_string(ms_t(running_ticks - iterations_ticks).count())).append("\n");
  if(is_print_rank) std::cout<<print_str;

  int found_k_path = (finalize_iterations(vertices) ? 1 : 0);
  int found_k_path_globally_across_all_instances;
  // MPI_COMM_WORLD is valid here even when running on multiple parallel instances
  // The idea is to see if any of the ranks in whole world has found a path
  MPI_Allreduce(&found_k_path, &found_k_path_globally_across_all_instances, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD);

  return found_k_path_globally_across_all_instances > 0;
}

void init_loop(std::vector<std::shared_ptr<vertex>> *vertices) {
  long long per_loop_random_seed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
  // MPI_COMM_WORLD is correct here even when running multiple parallel instances
  MPI_Bcast(&per_loop_random_seed, 1, MPI_LONG_LONG, 0, MPI_COMM_WORLD);

  std::default_random_engine re(per_loop_random_seed);
  std::uniform_int_distribution<int> uni_id_byte(-128,127);

  int degree = 3+log2(k);

  std::function<int()> gen_rnd_byte = std::bind(uni_id_byte, re);
  gf = galois_field::getInstance(1<<degree, (int)polynomial::create_irreducible(degree, gen_rnd_byte)->to_long());

  std::uniform_int_distribution<long> uni_ld;
  std::function<long()> gen_rnd_long = std::bind(uni_ld, re);

  int displas = p_ops->my_vertex_displas;
  int count = p_ops->my_vertex_count;
  for (int i = 0; i < global_vertex_count; ++i){
    // this step ensures every proc invokes the random engine
    // the same number of times, so when each of them take different
    // parts from this generated sequence they (the values) will still
    // be different and random.
    long uniq_rand_val = gen_rnd_long();
    if (i >= displas && i < displas+count){
      (*vertices)[i-displas]->uniq_rand_seed = uniq_rand_val;
    }
  }

  // Note, in C++ the distribution is in closed interval [a,b]
  // whereas in Java it's [a,b), so the random.nextInt(twoRaisedToK)
  // equivalent in C++ is [0,two_raised_to_k - 1]
  std::uniform_int_distribution<int> uni_id_2tothek(0,two_raised_to_k-1);
  std::function<int()> gen_rnd_int = std::bind(uni_id_2tothek, re);
  for (const auto &kv : (*p_ops->vertex_label_to_instance_rank)){
    int label = kv.first;
    (*random_assignments)[label] = gen_rnd_int();
  }

  for (int i = 0; i < k-1; ++i){
    completion_vars.get()[i] = gen_rnd_int();
  }

  for (const std::shared_ptr<vertex> &v : (*vertices)){
    v->init(k, r, gf, iter_bs);
  }
}

void run_super_steps(std::vector<std::shared_ptr<vertex>> *vertices, int local_iter, int global_iter) {
  std::string gap = "      ";
  double process_recvd_time_ms = 0;
  double recv_time_ms = 0;
  double comp_time_ms = 0;
  double send_time_ms = 0;
  double finalize_iter_time_ms = 0;

  ticks_t start_ticks, end_ticks;

  int worker_steps = max_iterations + 1;

  for (int ss = 0; ss < worker_steps; ++ss){
    if (ss > 0){
      start_ticks = hrc_t::now();
      recv_msgs(vertices, ss);
      end_ticks = hrc_t::now();
      recv_time_ms += ms_t(end_ticks - start_ticks).count();

      start_ticks = hrc_t::now();
      /* Assuming message size doesn't change with
       * iterations and super steps we can do this process received once
       * and be done with it */
      if (local_iter == 0 && ss < 2) {
        process_recvd_msgs(vertices, ss);
      }
      end_ticks = hrc_t::now();
      process_recvd_time_ms += ms_t(end_ticks - start_ticks).count();
    }

    start_ticks = hrc_t::now();
    compute(global_iter, vertices, ss);
    end_ticks = hrc_t::now();
    comp_time_ms += ms_t(end_ticks - start_ticks).count();

    if (ss< worker_steps - 1){
      start_ticks = hrc_t::now();
      send_msgs(vertices, ss);
      end_ticks = hrc_t::now();
      send_time_ms += ms_t(end_ticks - start_ticks).count();
    }
  }

  start_ticks = hrc_t::now();
  finalize_iteration(vertices);
  end_ticks = hrc_t::now();
  finalize_iter_time_ms += ms_t(end_ticks - start_ticks).count();

  times[0] += recv_time_ms;
  times[1] += process_recvd_time_ms;
  times[2] += comp_time_ms;
  times[3] += send_time_ms;
  times[4] += finalize_iter_time_ms;

  gap.append("-- Iter ").append(std::to_string(global_iter+1));

  std::string print_str = gap;
  print_str.append(" comp:");
  print_timing(comp_time_ms, print_str);

  print_str = gap;
  print_str.append(" process recvd:");
  print_timing(process_recvd_time_ms, print_str);

  print_str = gap;
  print_str.append(" recv:");
  print_timing(recv_time_ms, print_str);

  print_str = gap;
  print_str.append(" recv total:");
  print_timing((process_recvd_time_ms+recv_time_ms), print_str);

  print_str = gap;
  print_str.append(" send with prepare:");
  print_timing(send_time_ms, print_str);

  print_str = gap;
  print_str.append(" finalize iter:");
  print_timing(finalize_iter_time_ms, print_str);
}

void compute(int iter, std::vector<std::shared_ptr<vertex>> *vertices, int super_step) {
  for (int i = 0; i < (*vertices).size(); ++i){
    std::shared_ptr<vertex> vertex = (*vertices)[i];
    vertex->compute(super_step, iter, completion_vars, random_assignments);
  }
}

void recv_msgs(std::vector<std::shared_ptr<vertex>> *vertices, int super_step) {
  p_ops->all_to_all_v();
}

void process_recvd_msgs(std::vector<std::shared_ptr<vertex>> *vertices, int super_step) {
  for (int i = 0; i < (*vertices).size(); ++i){
    std::shared_ptr<vertex> vertex = (*vertices)[i];
    vertex->process_recvd(super_step);
  }
}

void send_msgs(std::vector<std::shared_ptr<vertex>> *vertices, int super_step) {
  for (int i = 0; i < (*vertices).size(); ++i){
    std::shared_ptr<vertex> vertex = (*vertices)[i];
    vertex->prepare_send(super_step);
  }
}

void finalize_iteration(std::vector<std::shared_ptr<vertex>> *vertices) {
  // TODO - introduce threads here
  for (const auto &vertex : (*vertices)){
    vertex->finalize_iteration();
  }
}

bool finalize_iterations(std::vector<std::shared_ptr<vertex>> *vertices) {
  // Note, we can't break the loop after finding on true
  // because the finalize_iterations() need to be called on all vertices
  bool found_k_path = false;
  for (const auto &v : (*vertices)) {
    if (v->finalize_iterations()) {
      found_k_path = true;
    }
  }
  return found_k_path;
}

void pretty_print_config(std::string &str){
  std::string params [] = {"Input File",
                           "Global Vertex Count",
                           "K",
                           "Epsilon",
                           "Delta",
                           "Alpha Max",
                           "Parallel Pattern"};
}

int log2(int x) {
  int result = 0;
  x >>= 1;
  while (x > 0){
    result++;
    x >>= 1;
  }
  return result;
}

void print_timing(
    const double duration_ms,
    const std::string &msg) {
  double avg_duration_ms, min_duration_ms, max_duration_ms;
  MPI_Reduce(&duration_ms, &min_duration_ms, 1, MPI_DOUBLE, MPI_MIN, 0, p_ops->MPI_COMM_INSTANCE);
  MPI_Reduce(&duration_ms, &max_duration_ms, 1, MPI_DOUBLE, MPI_MAX, 0, p_ops->MPI_COMM_INSTANCE);
  MPI_Reduce(&duration_ms, &avg_duration_ms, 1, MPI_DOUBLE, MPI_SUM, 0, p_ops->MPI_COMM_INSTANCE);
  if (is_print_rank){
    std::cout<<msg<<" [min max avg]ms: ["<< min_duration_ms
             << " " << max_duration_ms << " " << (avg_duration_ms / p_ops->instance_procs_count) << "]" <<std::endl;
  }
}

void print_timing_global(
    const double duration_ms,
    const std::string &msg) {
  double avg_duration_ms, min_duration_ms, max_duration_ms;
  MPI_Reduce(&duration_ms, &min_duration_ms, 1, MPI_DOUBLE, MPI_MIN, 0, MPI_COMM_WORLD);
  MPI_Reduce(&duration_ms, &max_duration_ms, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
  MPI_Reduce(&duration_ms, &avg_duration_ms, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
  if (is_print_rank) {
    std::cout << msg << " global[min max avg]ms: [" << min_duration_ms
              << " " << max_duration_ms << " " << (avg_duration_ms / p_ops->instance_procs_count) << "]" << std::endl;
  }
}


