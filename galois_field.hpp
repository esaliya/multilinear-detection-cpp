//
// Created by Saliya Ekanayake on 7/11/17.
//

#ifndef CLIONCPP_GALOIS_FIELD_HPP
#define CLIONCPP_GALOIS_FIELD_HPP


#include <vector>
#include <unordered_map>

/**
 * C++ version of the GaloisField in Java from Jose.
 * Implements Galois field arithmetic with 2^p elements.
 * Inputs must be unsigned integers.
 */
class galois_field {

public:
  galois_field(int field_size, int primitive_polynomial);
  ~galois_field();

  int get_field_size();

  int add(int x, int y);
  int multiply(int x, int y);
  static std::shared_ptr<galois_field> getInstance(int field_size, int primitive_polynomial);
  static std::shared_ptr<galois_field> getInstance();

private:
  std::vector<int> log_tbl;
  std::vector<int> pow_tbl;
  std::vector<int> mul_tbl;
  const int field_size;
  const int primitive_period;
  const int primitive_polynomial;

  // Field size 256 is good for byte based system
  static const int DEFAULT_FIELD_SIZE = 256;
  // primitive polynomial 1 + X^2 + X^3 + X^4 + X^8
  static const int DEFAULT_PRIMITIVE_POLYNOMIAL = 285;

  static std::map<int, std::shared_ptr<galois_field>> instances;

};


#endif //CLIONCPP_GALOIS_FIELD_HPP
