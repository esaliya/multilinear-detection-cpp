//
// Created by Saliya Ekanayake on 10/18/17.
//

#ifndef CLIONCPP_TEMPLATE_PARTITIONER_H
#define CLIONCPP_TEMPLATE_PARTITIONER_H


#include <vector>
#include <map>
#include <fstream>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <queue>
#include "graph.h"

class template_partitioner {
public:

  template_partitioner(const char *template_file, int template_vertex_count) {
    root_template = std::make_shared<graph>(id_count);
    sub_templates = std::make_shared<std::vector<std::shared_ptr<graph>>>();
    left_child = std::make_shared<std::map<int, std::shared_ptr<graph>>>();
    right_child = std::make_shared<std::map<int, std::shared_ptr<graph>>>();

    std::ifstream fs;
    std::string line;

    fs.open(template_file);
    std::vector<std::string> tokens;
    for (int i = 0; i < template_vertex_count; ++i) {
      getline(fs, line);
      boost::split(tokens, line, boost::is_any_of(" "), boost::token_compress_on);
      int node = std::stoi(tokens[0]);
      root_template->add_node(node);
      for (int j = 1; j < tokens.size(); ++j){
        root_template->add_edge(node, std::stoi(tokens[j]));
      }
    }
    fs.close();

    gen_sub_templates(root_template);
    std::sort(sub_templates->begin(), sub_templates->end(),
              [](std::shared_ptr<graph> g1, std::shared_ptr<graph> g2) ->bool{
                return g1->size < g2->size;
              });

    // Note verification code
    /*for (const std::shared_ptr<graph> &s : *sub_templates){
      std::cout<<s->id<<" "<<s->size<<"\n";
    }*/
  }

  void gen_sub_templates(std::shared_ptr<graph> sub_template){
    sub_templates->push_back(sub_template);
    if (sub_template->size <= 1){
      return;
    }

    std::shared_ptr<graph> copy = sub_template->deep_copy();
    int root = copy->get_root_id();
    int other_root = sub_template->get_first_nbr_id(root);
    copy->delete_edge(root, other_root);
    std::shared_ptr<std::vector<std::shared_ptr<std::set<int>>>> connected_comps
        = get_connected_components(copy);
    assert(connected_comps->size() == 2);
    std::shared_ptr<graph> left = sub_template->sub_graph((*connected_comps)[0], ++id_count);
    (*left_child)[sub_template->id] = left;
    std::shared_ptr<graph> right = sub_template->sub_graph((*connected_comps)[1], ++id_count);
    (*right_child)[sub_template->id] = right;

    gen_sub_templates(left);
    gen_sub_templates(right);
  }


  std::shared_ptr<std::vector<std::shared_ptr<graph>>> get_sub_templates(){
    return sub_templates;
  }

  int get_sub_template_count(){
    return (int) sub_templates->size();
  }

private:
  int id_count = 0;
  std::shared_ptr<graph> root_template = nullptr;
  std::shared_ptr<std::vector<std::shared_ptr<graph>>> sub_templates = nullptr;
  std::shared_ptr<std::map<int, std::shared_ptr<graph>>> left_child = nullptr;
  std::shared_ptr<std::map<int, std::shared_ptr<graph>>> right_child = nullptr;

  std::shared_ptr<std::vector<std::shared_ptr<std::set<int>>>> get_connected_components(std::shared_ptr<graph> g){
    std::shared_ptr<std::vector<std::shared_ptr<std::set<int>>>> comps = std::make_shared<std::vector<std::shared_ptr<std::set<int>>>>();
    std::set<int> seen;
    std::set<int> remaining = g->nodes;

    while(seen.size() < g->size){
      std::cout<<"seen.size"<<seen.size()<<" g.size"<<g->size<<"\n";
      std::shared_ptr<std::set<int>> comp = std::make_shared<std::set<int>>();
      int u = *remaining.begin();
      comp->insert(u);
      seen.insert(u);
      std::queue<int> q;
      q.push(u);
      while (!q.empty()){
        int node = q.front();
        q.pop();
        for (const int &nbr : *g->get_nbrs(node)){
          if (seen.find(nbr) == seen.end()){
            q.push(nbr);
            comp->insert(nbr);
            seen.insert(nbr);
          }
        }
      }
      comps->push_back(comp);
      std::set<int> diff;
      std::set_difference(remaining.begin(), remaining.end(),
                          seen.begin(), seen.end(),
                          std::inserter(diff, diff.begin()));
      remaining = diff;

    }
    return comps;
  }
};


#endif //CLIONCPP_TEMPLATE_PARTITIONER_H
