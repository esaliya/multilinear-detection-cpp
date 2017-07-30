//
// Created by Saliya Ekanayake on 7/11/17.
//

#include <iostream>
#include "test_map_class.hpp"

const std::shared_ptr<test_map_class> test_map_class::X(create_object());
const std::shared_ptr<test_map_class> test_map_class::Y(create_object2());

test_map_class::test_map_class():yy(12344) {}

test_map_class::test_map_class(const int yy) : yy(yy) {}

std::map<int,int> test_map_class::static_map = std::map<int,int>();

void test_map_class::add_and_print_element(int x, int y) {
  static_map[x] = y;
  std::cout<<static_map[x]<<std::endl;
  std::cout<<XX<<std::endl;
}

void test_map_class::add_element_to_vector_and_print(int z) {
  private_vector[0] = z;
  std::cout<<private_vector[0]<<std::endl;

}

test_map_class *test_map_class::create_object() {
  return new test_map_class(22);
}

void test_map_class::test_object_creation_and_shared_ptr() {
  std::cout<<X->yy<<std::endl;
  std::cout<<Y->yy<<std::endl;
}

std::shared_ptr<test_map_class> test_map_class::create_object2() {
  return std::shared_ptr<test_map_class>(new test_map_class(57));
}


std::shared_ptr<std::set<long, rev_comp_t>> test_map_class::create_degrees_collection() {
  rev_comp_t rev_comparator = [](const long& lhs, const long& rhs) -> bool{return lhs > rhs;};
  return std::shared_ptr<std::set<long, rev_comp_t>>(new std::set<long, rev_comp_t>(rev_comparator));
}

std::shared_ptr<test_map_class> test_map_class::create_object_and_return_shared_ptr() {
  std::shared_ptr<std::set<long, rev_comp_t>> dgrs = create_degrees_collection();
  dgrs->insert(23);
  dgrs->insert(98);
  dgrs->insert(67);
  std::shared_ptr<test_map_class> obj(new test_map_class(dgrs));
  std::cout<<"original dgrs"<<std::endl;
  for (std::set<long, rev_comp_t>::iterator it = dgrs->begin(); it != dgrs->end(); ++it){
    std::cout<<(*it)<<std::endl;
  }
  return obj;
}

test_map_class::test_map_class(std::shared_ptr<std::set<long, rev_comp_t>> dgrs) : my_degrees(*dgrs.get()), yy(1376) {
  my_degrees.insert(76);
  my_degrees.insert(83);
  my_degrees.insert(31);
  std::cout<<"my_degrees in the object"<<std::endl;
  for (std::set<long, rev_comp_t>::iterator it = my_degrees.begin(); it != my_degrees.end(); ++it){
    std::cout<<(*it)<<std::endl;
  }
}

int test_map_class::get_yy() {
  return yy;
}

//std::shared_ptr<std::set<long, rev_comp_t)>> test_map_class::create_degrees_collection() {
//  auto reverse_comparator = [](const int& lhs, const int& rhs) -> bool{return lhs > rhs;};
//  std::shared_ptr<std::set<long, bool (*) (const int&, const int&)>> ptr = std::shared_ptr<std::set<long, bool (*) (const int&, const int&)>>(new std::set<long, bool (*) (const int&, const int&)>(reverse_comparator));
//  std::set<long, bool (*)(const long&, const long&)>* myset = new std::set<long, bool (*)(const long&, const long&)>(cmp);
//  return myset;
//}
