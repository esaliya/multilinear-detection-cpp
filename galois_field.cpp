//
// Created by Saliya Ekanayake on 7/11/17.
//

#include <map>
#include <iostream>
#include <memory>
#include "galois_field.hpp"

std::map<int, std::shared_ptr<galois_field>> galois_field::instances
    = std::map<int, std::shared_ptr<galois_field>>();

galois_field::galois_field(int field_size, int primitive_polynomial)
    : field_size(field_size),
      primitive_period(field_size-1),
      primitive_polynomial(primitive_polynomial),
      log_tbl((unsigned long) field_size, 0),
      pow_tbl((unsigned long) field_size, 0),
      mul_tbl((unsigned long)(field_size*field_size), 0)
{
  if (field_size <= 0){
    std::cout<<"ERROR: fieldSize must be a positive integer"<<std::endl;
  }

  if (primitive_polynomial <= 0){
    std::cout<<"ERROR: fieldSize must be a positive integer"<<std::endl;
  }

  int value = 1;
  for (int pow = 0; pow < field_size - 1; pow++) {
    pow_tbl[pow] = value;
    log_tbl[value] = pow;
    value = value * 2;
    if (value >= field_size) {
      value = value ^ primitive_polynomial;
    }
  }

  // building multiplication table
  for (int i = 0; i < field_size; ++i){
    for (int j = 0; j < field_size; ++j){
      if (i == 0 || j == 0){
        mul_tbl[i*field_size+j] = 0;
      } else {
        int z = log_tbl[i] + log_tbl[j];
        z = z >= primitive_period ? z - primitive_period : z;
        z = pow_tbl[z];
        mul_tbl[i*field_size+j] = z;
      }
    }
  }
}

galois_field::~galois_field() {

}

int galois_field::get_field_size() {
  return field_size;
}

int galois_field::add(int x, int y) {
  return x^y;
}

int galois_field::multiply(int x, int y) {
  return mul_tbl[x*field_size+y];
}

std::shared_ptr<galois_field> galois_field::getInstance(int field_size, int primitive_polynomial) {
  int key = ((field_size << 16) & 0xFFFF0000) + (primitive_polynomial & 0x0000FFFF);
  // The original code was thread safe here but we don't need that
  std::shared_ptr<galois_field> gf;
  if (instances.find(key) == instances.end()){
    gf  = std::make_shared<galois_field>(field_size, primitive_polynomial);
    instances[key] = gf;
  } else {
    gf = instances[key];
  }
  return gf;
}

std::shared_ptr<galois_field> galois_field::getInstance() {
  return getInstance(DEFAULT_FIELD_SIZE, DEFAULT_PRIMITIVE_POLYNOMIAL);
}





