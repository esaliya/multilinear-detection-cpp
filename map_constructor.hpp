//
// Created by Saliya Ekanayake on 7/24/17.
//

#ifndef CLIONCPP_MAP_CONSTRUCTOR_HPP
#define CLIONCPP_MAP_CONSTRUCTOR_HPP


#include <boost/shared_ptr.hpp>
#include <map>
#include <set>

class map_constructor {
public:
  map_constructor();
  map_constructor(std::shared_ptr<std::set<int>> set);
  
  std::shared_ptr<std::set<int>> get_set();
private:
  std::shared_ptr<std::set<int>> set;
};


#endif //CLIONCPP_MAP_CONSTRUCTOR_HPP
