//
// Created by Saliya Ekanayake on 8/5/17.
//
#include <iostream>
#include <fstream>
#include <boost/shared_ptr.hpp>
#include <vector>
#include <map>
#include <ratio>
#include <chrono>
#include <set>

typedef std::chrono::duration<double, std::milli> ms_t;
typedef std::chrono::time_point<std::chrono::high_resolution_clock> ticks_t;

typedef std::chrono::high_resolution_clock hrc_t;

int main(int argc, char *argv[]) {
  char *f = argv[1];
  int vc = std::stoi(argv[2]);
  int ec = std::stoi(argv[3]);
  char *of_metis = argv[4];
  char *of_graph = argv[5];

  int out_freq = std::stoi(argv[6]);

  std::cout<<f<<" "<<vc<<" "<<ec<<" "<<of_metis<<std::endl;

  std::ifstream ifs(f);
  std::ofstream ofs_graph(of_graph);
  std::ofstream ofs_metis(of_metis);

  std::map<int, std::shared_ptr<std::set<int>>> *g = new std::map<int, std::shared_ptr<std::set<int>>>();
  int count = 0;
  std::string line;
  ticks_t start = hrc_t::now();
  ticks_t end;
  int uniq_vc = 0;
  int missing_vc = 0;
  while (true){
    if (count == ec) break;
    getline(ifs, line, '\t');
    if (line.c_str()[0] == '#') {
      getline(ifs, line);
      continue;
    }

    int src_v = std::stoi(line.c_str());
    getline(ifs, line);
    int dest_v = std::stoi(line.c_str());

    // TODO - debug
    /* if (src_v == 2){
       std::cout<<"should be present in g now "<< (g->find(src_v) == g->end())<<" "<<(*g)[src_v]->size()<<std::endl;
       break;
     }*/
    if (g->find(src_v) == g->end()){
      (*g)[src_v] = std::make_shared<std::set<int>>();
      ++uniq_vc;
    }
    (*g)[src_v].get()->insert(dest_v);

    if (g->find(dest_v) == g->end()){
      (*g)[dest_v] = std::make_shared<std::set<int>>();
      ++uniq_vc;
    }
    (*g)[dest_v].get()->insert(src_v);

    ++count;
    if (count%out_freq == 0 || count == ec ){
      end = hrc_t::now();
      std::cout<<"Read "<< count << " lines in "<< (ms_t(end - start)).count() << std::endl;
    }
  }
  std::cout<<"Total count "<<count<<std::endl;

  std::map<int,int> *key_updates = new std::map<int,int>();
  int idx = 0;
  for (const auto &kv : *g){
    (*key_updates)[kv.first] = idx++;
  }

  std::cout<<"Graph line count "<< g->size() << std::endl;
  std::cout<<"Uniq vc "<< uniq_vc << std::endl;

  start = hrc_t::now();
  std::cout<<"Writing zero-idx based adj list graph ... ";
  for (const auto &kv : *g){
    ofs_graph << (*key_updates)[kv.first] << " " << 1; // 1 for dummy weight
    for (const auto &nbr : *kv.second){
      ofs_graph << " " << (*key_updates)[nbr];
    }
    ofs_graph << std::endl;
  }
  end = hrc_t::now();
  std::cout << "duration (ms) " << (ms_t(end - start)).count();


  start = hrc_t::now();
  std::cout<<"Writing metis graph ... ";
  ofs_metis<<vc<<" "<<ec<<std::endl;
  for (const auto &kv : *g){
    for (const auto &nbr : *kv.second){
      ofs_metis << (*key_updates)[nbr]+1 << " "; // change to 1 based idx
    }
    ofs_metis << std::endl;
  }
  end = hrc_t::now();
  std::cout << "duration (ms) " << (ms_t(end - start)).count();

  delete key_updates;
  delete g;



  ifs.close();
  ofs_graph.close();
  ofs_metis.close();

  return 0;
}