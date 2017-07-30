//
// Created by Saliya Ekanayake on 7/11/17.
//

#ifndef CLIONCPP_TEST_MAP_CLASS_HPP
#define CLIONCPP_TEST_MAP_CLASS_HPP


#include <map>
#include <vector>
#include <boost/shared_ptr.hpp>
#include <set>

typedef bool (*rev_comp_t)(const long&, const long&);
class test_map_class {

public:
  test_map_class();
  test_map_class(const int yy);

  static void add_and_print_element(int x, int y);
  void add_element_to_vector_and_print(int z);
  void test_object_creation_and_shared_ptr();
  int get_yy();
  static std::shared_ptr<std::set<long, rev_comp_t>> create_degrees_collection();
  static std::shared_ptr<test_map_class> create_object_and_return_shared_ptr();
  ~test_map_class(){}
private:
  test_map_class(std::shared_ptr<std::set<long, rev_comp_t>> dgrs);
  static std::map<int, int> static_map;
  const static int XX = 256;
  std::vector<int> private_vector;
  const int yy;
  static const std::shared_ptr<test_map_class>X;
  static const std::shared_ptr<test_map_class>Y;
  static test_map_class* create_object();
  static std::shared_ptr<test_map_class> create_object2();

  std::set<long, rev_comp_t> my_degrees;


};


#endif //CLIONCPP_TEST_MAP_CLASS_HPP
