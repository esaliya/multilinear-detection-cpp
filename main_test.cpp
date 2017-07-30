#include <omp.h>
#include <stdio.h>
#include <chrono>
#include <fstream>
#include <iostream>
#include <map>
#include <vector>
#include <random>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

#include <boost/foreach.hpp>
#include <set>
#include <bitset>
#include "test_map_class.hpp"
#include "map_constructor.hpp"
#include "polynomial.hpp"

typedef std::chrono::duration<double, std::milli> ms_t;
typedef std::chrono::duration<double, std::micro> micros_t;
typedef std::chrono::time_point<std::chrono::high_resolution_clock> ticks_t;

typedef std::chrono::high_resolution_clock hrc_t;

void openmp_parfor_test(){
  int size = 16777216;
  std::vector<double> *vec = new std::vector<double>((unsigned long) size);

  ticks_t start_ticks = hrc_t::now();

//  omp_set_num_threads(1);
#pragma omp parallel
{
  int num_threads = omp_get_num_threads();
  std::cout<<"num threads: "<<num_threads;
#pragma omp for
  for (int i = 0; i < size; ++i) {
    (*vec)[i] = sqrt(i);
  }
}

  ticks_t end_ticks = hrc_t::now();

  std::cout<<"Loop initialization took (ms) "<<(ms_t(end_ticks - start_ticks).count())<<std::endl;

  delete vec;
}

void rnd_pointer_test(){
  std::default_random_engine *re = new std::default_random_engine(345678L);
  std::uniform_int_distribution<int> *uni_int_dist = new std::uniform_int_distribution<int>(0,10);
//  std::uniform_int_distribution<int> uni_int_dist(0,10);
  std::cout<<(*uni_int_dist)(*re)<<std::endl;
}


void pass_bind_test_helper(
    std::function<int()> &bind) {
  std::cout<<bind()<<std::endl;
}

void pass_bind_test(){
  std::default_random_engine re2(100);
  std::uniform_int_distribution<int> unif(-128,127);
  std::function<int()> gen = std::bind(unif, re2);
  for (int i = 0; i < 5; ++i) {
    pass_bind_test_helper(
        gen);
  }
}

void pass_by_ref_2_internal_internal(int &z){
  z = 30;
}

void pass_by_ref_test_2_internal(int &y){
  y = 20;
  pass_by_ref_2_internal_internal(y);
}

void pass_by_ref_test_2(){
  int x = 10;
  std::cout<<x<<std::endl;
  pass_by_ref_test_2_internal(x);
  std::cout<<x<<std::endl;

}


void test_set_remove(){
  std::set<long> s;
  s.insert(10);
  s.insert(10);
  s.insert(23);
  s.insert(11);
  s.insert(10);
  s.insert(23);
  s.insert(33);

  std::set<long>::iterator it = s.find(23);
  if (it != s.end()){
    s.erase(it);
  }

  for (const long &v : s){
    std::cout<<v<<std::endl;
  }
}

void int_bitcount(){
  int x = 7;
  int y = -7;
  std::bitset<sizeof(int)*8> bs1((unsigned long long int) x);
  std::bitset<sizeof(int)*8> bs2((unsigned long long int) y);
  std::cout<<bs1<<std::endl;
  std::cout<<bs2<<std::endl;

}


int bit_count(long v){
  std::bitset<sizeof(long)*8> bs((unsigned long)v);
  int count = (int) bs.count();
  int total_length = sizeof(long)*8;
  return bs[total_length-1] == 1 ? (total_length - count) : count;
}

void test_bitset(){
  long x = 65529;
  std::bitset<sizeof(long)> bs((unsigned long long int) x);
  std::cout<<bs.to_string()<<" "<<bs.count()<<"  "<<bs[0]<<"  "<<bs[1]<<std::endl;
  std::cout<<"size of long "<< sizeof(long);

  short y = -7L;
  unsigned short u_y = (unsigned short)y;
  std::cout<<std::endl<<y<<" "<<u_y<<std::endl;

  std::cout<<bit_count(-7L)<<std::endl;
  std::cout<<bit_count(7L)<<std::endl;
}

void test_map_construct(){
  std::shared_ptr<map_constructor> mc = std::make_shared<map_constructor>();
  std::shared_ptr<std::set<int>> mc_set = mc->get_set();
  mc_set->insert(10);
  mc_set->insert(37);
  mc_set->insert(4);

  std::shared_ptr<map_constructor> copy_mc = std::make_shared<map_constructor>(mc_set);
  std::shared_ptr<std::set<int>> copy_mc_set = copy_mc->get_set();
  copy_mc_set->insert(90);

  for(const int &mc_val : (*mc_set)){
    std::cout<<mc_val<<" ";
  }
  std::cout<<std::endl;

  for(const int &copy_mc_val : (*copy_mc_set)){
    std::cout<<copy_mc_val<<" ";
  }
  std::cout<<std::endl;

}

void long_rand_test(){
  unsigned int seed = (unsigned int) std::chrono::high_resolution_clock::now().time_since_epoch().count();
  std::default_random_engine re1(seed);
  std::default_random_engine re2(seed);
  std::uniform_int_distribution<long> unid1;
  std::uniform_int_distribution<int> unid2;
  auto gen1 = std::bind(unid1, re1);
  auto gen2 = std::bind(unid2, re2);

  int x = 5;
  for (int i = 0; i < x; ++i) {
    long v1 = gen1();
    long v2 = gen2();
    std::cout << v1 << "  " << v2 << " " << v1 - v2 << std::endl;
  }

  std::cout<<"=========\n";

  std::default_random_engine re3(seed);
  std::uniform_int_distribution<int> unid3;
  auto gen3 = std::bind(unid1, re3);
  for (int i = 0; i < x; ++i) {
    gen3();
  }

  auto gen3_1 = std::bind(unid3, re3);
  for (int i = 0; i < x; ++i) {
    long v1 = gen1();
    long v3_1 = gen3_1();
    std::cout << v1 << "  " << v3_1 << " " << v1 - v3_1 << std::endl;
  }
}

void rand_pass_test(){
  unsigned int seed = (unsigned int) std::chrono::high_resolution_clock::now().time_since_epoch().count();
  std::default_random_engine re_1(seed);
  std::uniform_int_distribution<long> unid_1;

  auto gen_1 = std::bind(unid_1, re_1);
  int x = 5;
  for (int i = 0; i < x; ++i) {
//    long v = unid_1(re_1);
    long v = gen_1(re_1);
    std::cout << v << std::endl;
  }

  std::default_random_engine re_2(seed);
  auto gen_2 = std::bind(unid_1, re_2);
  x = 5;
  for (int i = 0; i < x; ++i) {
//    long v = unid_1(re_2);
    long v = gen_2();
    std::cout << v << std::endl;
  }

}



void rand_byte_test(){
  std::uniform_int_distribution<int> unif(-128,127);
  std::default_random_engine re((unsigned int) std::chrono::high_resolution_clock::now().time_since_epoch().count());
  std::vector<char> bytes(5);
  auto gen = std::bind(unif, re);
  std::generate(std::begin(bytes), std::end(bytes), gen);

  for (const int &x : bytes){
    std::cout<<(int)x<<std::endl;
  }

  char c = 127;
  std::cout<<(int)c;
}

void time_test(){
  std::chrono::time_point<std::chrono::high_resolution_clock > start_prog = std::chrono::system_clock::now();
  std::time_t start_prog_time = std::chrono::system_clock::to_time_t(start_prog);
  std::string print_str = "\nINFO: Graph computation started on ";
  print_str.append(std::ctime(&start_prog_time));
  std::cout<<print_str;
}

void math_tests(){
  double epsilon = 0.1;
  double prob_success = 0.2;
  int iter = (int) round(log(epsilon) / log(1 - prob_success));
  std::cout<<iter<<std::endl;
}

void shared_ptr_creation_test(){
  std::shared_ptr<int> shared_int_ptr = std::make_shared<int>();
  (*shared_int_ptr) = 10;
}

void shared_ptr_array_copy(){
  int size = 10;
  std::shared_ptr<short> b = std::shared_ptr<short>(new short[size](), std::default_delete<short[]>());
  std::shared_ptr<short> rb = std::shared_ptr<short>(new short[size](), std::default_delete<short[]>());
  for (int i = 0; i < 10; ++i){
    b.get()[i] = (short) (size - i);
  }
  
  std::copy(b.get(), b.get()+size, rb.get());
  
  for (int i = 0; i < 10; ++i){
    std::cout<<rb.get()[i]<<std::endl;
  }
}

void map_increment(){
  std::map<int,int> *map = new std::map<int,int>();
  (*map)[2] = 234;
  if (map->find(2) != map->end()){
    ++(*map)[2];
  }

  for (const auto &kv : *map){
    std::cout<<kv.first<<" "<<kv.second<<std::endl;
  }

  delete map;
}


void shared_ptr_to_array(){
  std::shared_ptr<short> b = std::shared_ptr<short>(new short[10](), std::default_delete<short[]>());
  b.get()[3] = 72;
  for (int i = 0; i < 10; ++i){
    std::cout<<b.get()[i]<<std::endl;
  }
}

void int_to_short_test(){
  int original = 70345588;

  short firstHalf = (short) (original >> 16);
  short secondHalf = (short) (original & 0xffff);

  int reconstituted = (firstHalf << 16) | (secondHalf & 0xffff);
  std::cout<<original<<"  "<<reconstituted<<std::endl;
}

void vector_test(){
  std::shared_ptr<std::vector<int>> v = std::make_shared<std::vector<int>>();
  v->push_back(10);
  if (v->size() > 0){
    ++(*v)[0];
  }
  std::cout<<(*v)[0]<<std::endl;

  std::shared_ptr<std::vector<int>> w = std::make_shared<std::vector<int>>();
  w->push_back(23);
  w->push_back(45);

  std::copy(w->begin(), w->end(), std::back_inserter(*v));

  for (const auto &val : (*v)){
    std::cout<<val<<std::endl;
  }

}

void sizeof_test(){
  int x = 1000000000;
  std::cout<< sizeof(x)<<" "<<x<<std::endl;
}


void assign_to_point_test2_internal_internal(std::vector<std::shared_ptr<test_map_class>> *&vec){
  vec = new std::vector<std::shared_ptr<test_map_class>>(5);
  (*vec)[0] = std::make_shared<test_map_class>(999);
}

void assign_to_pointer_test2_internal(std::vector<std::shared_ptr<test_map_class>> *&vec){
  assign_to_point_test2_internal_internal(vec);
  (*vec)[1] = std::make_shared<test_map_class>(888);
}

void assign_to_pointer_test2(){
  std::vector<std::shared_ptr<test_map_class>> *vec = nullptr;
  assign_to_pointer_test2_internal(vec);
  std::cout<<((*vec)[0].get())->get_yy()<<std::endl;
  std::cout<<((*vec)[1].get())->get_yy()<<std::endl;
  delete vec;
}

void assign_to_pointer_test_internal(std::vector<int> **v){
  (*v) = new std::vector<int>();
  (*v)->push_back(10);
}

void assign_to_pointer_test(){
  std::vector<int> *WECK = nullptr;
  std::vector<int> **vec = &WECK;
  assign_to_pointer_test_internal(vec);
  std::cout<<(**vec)[0]<<std::endl;
  delete *vec;
}


void pass_by_ref_helper(std::vector<int> &vec){
  vec[0] = 23;
  vec[1] = 43;
}

void pass_by_ref_test(){
  std::vector<int> *vec = new std::vector<int>(4);
  pass_by_ref_helper((*vec));
  for (std::vector<int>::iterator it = vec->begin(); it != vec->end(); ++it){
    std::cout<<(*it)<<std::endl;
  }
}

void test_set_clear(){
  std::set<int> myset;
  std::set<int> myset2;
  std::set<int> diff;
  for (int i=1; i<=10; i++) {
    myset.insert(i*10);
    if (i < 4) {
      myset2.insert((i + 3) * 10);
    }
  }
  myset2.insert(90);

  std::cout<<"myset 1 size "<<myset.size()<<" [ ";
  for (const int &s1v : myset){
    std::cout<<s1v<<" ";
  }
  std::cout<<" ]"<<std::endl;

  std::cout<<"myset 2 size "<<myset2.size()<<" [ ";
  for (const int &s2v : myset2){
    std::cout<<s2v<<" ";
  }
  std::cout<<" ]"<<std::endl;

//  myset.erase(myset2.begin(), myset2.end());

  std::set_difference(myset.begin(), myset.end(),
                      myset2.begin(), myset2.end(),
                      std::inserter(diff, diff.begin()));

  std::cout<<"diff (myset - myset2) size "<<diff.size()<<" [ ";
  for (const int &dv : diff){
    std::cout<<dv<<" ";
  }
  std::cout<<" ]"<<std::endl;

}

void test_object_creation_and_shared_ptr(){
  std::shared_ptr<test_map_class> shared_ptr_to_obj = test_map_class::create_object_and_return_shared_ptr();
}

void test_shared_ptr_for_set(){
  std::shared_ptr<std::set<long, rev_comp_t>> sp_set = test_map_class::create_degrees_collection();
  sp_set->insert(37);
  sp_set->insert(68);
  sp_set->insert(22);
  for (std::set<long, rev_comp_t>::iterator it = sp_set->begin(); it != sp_set->end(); ++it){
    std::cout<<(*it)<<std::endl;
  }
}

void test_comparator(){
  auto reverse_comparator = [](const int& lhs, const int& rhs) -> bool{return lhs > rhs;};
  std::set<int, decltype(reverse_comparator)> myset(reverse_comparator);
  myset.insert(10);
  myset.insert(20);
  for (std::set<int>::iterator it = myset.begin(); it != myset.end(); ++it){
    std::cout<<(*it);
  }

  for (std::set<int>::iterator it = myset.begin(); it != myset.end(); ++it){
    std::cout<<(*it);
  }
}

void test_loop(){
  int field_size = 10;
  for (int i = 0; i < field_size; ++i) {
    for (int j = 0; j < field_size; ++j) {
      if (i == 0 || j == 0) {
        std::cout<<i << " " << j << std::endl;
        continue;
      }
      int z = 100;
      std::cout<<z<<std::endl;
    }
  }
}

void test_method_for_map(){
  test_map_class tmc(43);
  /* Should print 22 and 57 (not 43 or any other number) */
  tmc.test_object_creation_and_shared_ptr();
//  test_map_class::add_and_print_element(10,20);
//  test_map_class tmc;
//  tmc.add_element_to_vector_and_print(100);
}

void test_string(){
  std::string split_me( "hello world  how are   you" );

  std::vector<std::string> tokens;
  boost::split( tokens, split_me, boost::is_any_of(" "), boost::token_compress_on);

  std::string split_me_too( "yes yes i'm ready" );
  boost::split( tokens, split_me_too, boost::is_any_of(" "), boost::token_compress_on);

  std::cout << tokens.size() << " tokens" << std::endl;
//  BOOST_FOREACH( const std::string& i, tokens ) {
//    std::cout << "'" << i << "'" << std::endl;
//  }

  for (std::string v : tokens){
    std::cout << "'" << v << "'" << std::endl;
  }

  std::string number = "124";
  std::cout<<std::stoi(number);

}

void test_rand_time_seed(){
  std::chrono::time_point<std::chrono::high_resolution_clock> start = std::chrono::high_resolution_clock::now();
  std::default_random_engine re((unsigned int) start.time_since_epoch().count());
  std::uniform_real_distribution<double> unif;
  auto bind = std::bind(unif, re);
  std::cout<<bind()<<std::endl;
  std::cout<<bind()<<std::endl;
  std::cout<<bind()<<std::endl;
}


void test_rand(){
  std::uniform_real_distribution<double> unif;
  std::default_random_engine re;
  std::default_random_engine re2;
  re.seed(10088388);
  re2.seed(100883883);
  for (int i = 0; i < 10; ++i) {
    std::cout << unif(re) << " " << unif(re2) << std::endl;
  }
}

void test_array(){
  int* arr = new int[10];
  arr[1] = 23;
  std::cout<<arr[1]<<std::endl;
  delete[] arr;

  std::vector<int> vint(2);
  vint[1] =13;
  std::cout<<vint[1]<<std::endl;

  std::vector<int>* vint2 = new std::vector<int>(2);
  (*vint2)[1] =33;
  std::cout<<(*vint2)[1]<<std::endl;

  // Seems it's not necessary to do this shared pointers for vectors
  std::shared_ptr<std::vector<int>> shared_vector = std::make_shared<std::vector<int>>(2);


}


void test_shard_ptr(std::shared_ptr<std::map<int,int>> x){
  (*x)[2] = 40;
}

void test_map(){
  std::map<int, int> *number_map = new std::map<int,int>();
  std::shared_ptr<std::map<int,int>> smart_map(new std::map<int,int>());
  std::shared_ptr<std::map<int,int>> smart_map2 = std::make_shared<std::map<int,int>>();

  (*smart_map)[2] = 30;
  test_shard_ptr(smart_map);
  (*number_map)[2] = 20;
  int* arr = new int[4];
  arr[1] = 10;
//  number_map[2] = 20;
//  number_map[4] = 40;
  std::cout<<(*number_map)[2]<<" "<<arr[1]<<" "<<(*smart_map)[1]<<std::endl;
  delete[] arr;
}

void test(){
  short* a = (short*) malloc(sizeof(short)*3);
  short* b = new short[5];

  for (int i = 0; i < 3; ++i){
    a[i] = (short) i;
  }

  for (int i = 0; i < 5; ++i){
    b[i] = (short) (i+5);
  }


  std::copy(a, a+3, &b[2]);

  for (int i = 0; i < 5; ++i){
    std::cout<<b[i]<<" ";
  }
}

int main() {
  openmp_parfor_test();
//  rnd_pointer_test();
//  int_bitcount();
//  pass_bind_test();
//  poly_test();
//  pass_by_ref_test_2();
//  rand_pass_test();
//  long_rand_test();
//  test_set_remove();
//  test_bitset();
//  test_map_construct();
//  rand_byte_test();
//  test_rand_time_seed();
//  time_test();
//  math_tests();
//  shared_ptr_creation_test();
//  shared_ptr_array_copy();
//  map_increment();
//  shared_ptr_to_array();
//  int_to_short_test();
//  vector_test();
//  sizeof_test();
//  assign_to_pointer_test2();
//  assign_to_pointer_test();
//  pass_by_ref_test();
//  test_set_clear();
//  test_object_creation_and_shared_ptr();
//  test_shared_ptr_for_set();
//  test_comparator();
//  test_loop();
//  test_method_for_map();
//  test_string();
//  test_rand();
//  test_array();
//  test_map();
//  test();

  /*
#pragma omp parallel
  printf("Hello from thread %d, nthreads %d\n",
         omp_get_thread_num(), omp_get_num_threads());

  const char *graph_file = "/Users/esaliya/sali/projects/graphs/data/snap/fascia_com-orkut.ungraph.txt";
  std::ifstream file_g;
  std::string line;
  int* srcs_g;
  int* dsts_g;
  int n_g;
  unsigned m_g;

  int rank = 0;

  std::chrono::time_point<std::chrono::system_clock> start, end;
  start = std::chrono::system_clock::now();
  file_g.open(graph_file);

  // Read in labels and vertices for graph
  getline(file_g, line);
  n_g = atoi(line.c_str());
  getline(file_g, line);
  m_g = (unsigned int) strtoul(line.c_str(), NULL, 10);

  srcs_g = new int[m_g];
  dsts_g = new int[m_g];


  for (unsigned i = 0; i < m_g; ++i)
  {
    getline(file_g, line, ' ');
    srcs_g[i] = atoi(line.c_str());
    getline(file_g, line);
    dsts_g[i] = atoi(line.c_str());
  }

  file_g.close();
  end = std::chrono::system_clock::now();
  std::chrono::duration<double> elapsed_seconds = end-start;
  std::cout << "*****Rank: 0 finished reading the whole graph in " << elapsed_seconds.count() << " seconds";

   */

}
