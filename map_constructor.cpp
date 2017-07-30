//
// Created by Saliya Ekanayake on 7/24/17.
//

#include "map_constructor.hpp"

map_constructor::map_constructor() {
  set = std::make_shared<std::set<int>>();

}

map_constructor::map_constructor(std::shared_ptr<std::set<int>> set) : set(std::make_shared<std::set<int>>(*set)) {

}

std::shared_ptr<std::set<int>> map_constructor::get_set() {
  return set;
}