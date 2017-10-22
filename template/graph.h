//
// Created by Saliya Ekanayake on 10/17/17.
//

#ifndef CLIONCPP_GRAPH_H
#define CLIONCPP_GRAPH_H


#include <vector>
#include <map>
#include <boost/shared_ptr.hpp>
#include <set>

class graph {
public:
  std::shared_ptr<std::map<int,std::shared_ptr<std::set<int>>>> adj_list = nullptr;
  std::set<int> nodes;
  int id;
  int size;

  graph(int id){
    this->id = id;
    this->size = 0;
    adj_list = std::make_shared<std::map<int, std::shared_ptr<std::set<int>>>>();
  }



  std::shared_ptr<graph> sub_graph(std::shared_ptr<std::set<int>> s, int id){
    std::shared_ptr<graph> g = std::make_shared<graph>(id);
    for (const int &node : *s){
      assert(has_node(node));
      g->add_node(node);
    }

    for (const int &node : *s){
      for (const int &nbr : *get_nbrs(node)){
        if (g->has_node(nbr)){
          g->add_edge(node, nbr);
        }
      }
    }
    return g;
  }

  bool has_node(int u){
    return adj_list->find(u) != adj_list->end();
  }

  std::shared_ptr<std::set<int>> get_nbrs(int u){
    return (*adj_list)[u];
  }

  std::shared_ptr<graph> deep_copy(){
    std::shared_ptr<graph> to = std::make_shared<graph>(id);
    to->size = size;
    for (auto it = adj_list->begin(); it != adj_list->end(); ++it){
      (*to->adj_list)[it->first] = std::make_shared<std::set<int>>();
      for (auto nbr_it = it->second->begin(); nbr_it != it->second->end(); ++nbr_it){
        (*to->adj_list)[it->first]->insert(*nbr_it);
      }
    }

    for (const int &node : nodes){
      to->nodes.insert(node);
    }
    return to;
  }

  int get_root_id(){
    return adj_list->begin()->first;
  }

  int get_first_nbr_id(int id){
    return *((*adj_list)[id]->begin());
  }

  void add_edge(int u, int v){
    if (adj_list->find(u) == adj_list->end()){
      add_node(u);
    }

    if (adj_list->find(v) == adj_list->end()){
      add_node(v);
    }

    (*adj_list)[u]->insert(v);
    (*adj_list)[v]->insert(u);
  }

  void add_node(int n){
    if (adj_list->find(n) == adj_list->end()) {
      (*adj_list)[n] = std::make_shared<std::set<int>>();
      nodes.insert(n);
      ++size;
    }
  }

  void delete_edge(int u, int v) {
    auto it = (*adj_list)[u]->find(v);
    if (it != (*adj_list)[u]->end()) {
      (*adj_list)[u]->erase(it);
    }

    it = (*adj_list)[v]->find(u);
    if (it != (*adj_list)[v]->end()) {
      (*adj_list)[v]->erase(it);
    }
  }

};


#endif //CLIONCPP_GRAPH_H
