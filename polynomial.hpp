//
// Created by Saliya Ekanayake on 7/12/17.
//

#ifndef CLIONCPP_POLYNOMIAL_HPP
#define CLIONCPP_POLYNOMIAL_HPP


#include <set>
#include <vector>
#include <memory>

typedef bool (*rev_comp_t)(const long&, const long&);

class polynomial : public std::enable_shared_from_this<polynomial>{
public:
  static const long Q = 2L;
  static const std::shared_ptr<polynomial> X;
  static const std::shared_ptr<polynomial> ONE;

  static std::shared_ptr<polynomial> create_irreducible(int degree);
  static std::shared_ptr<polynomial> create_irreducible(int degree, std::function<int()> &gen_rnd_byte);

  long to_long();

  ~polynomial();

  // Print the object
  friend std::ostream& operator <<(std::ostream& outputStream, const polynomial& p);

private:
  static std::shared_ptr<polynomial> create_from_long(long l);
  static std::shared_ptr<polynomial> create_from_bytes(std::vector<char> bytes, int degree);
  static std::shared_ptr<polynomial> create_random(int degree);
  static std::shared_ptr<polynomial> create_random(int degree, std::function<int()> &gen_rnd_byte);
  static std::shared_ptr<std::set<long, rev_comp_t>> create_degrees_collection();

  static bool test_bit(long v, int n);
  static int bit_count(long v);
  static int compare(long a, long b);

  std::shared_ptr<std::set<long, rev_comp_t>> degrees = nullptr;

  polynomial();
  polynomial(std::shared_ptr<polynomial> poly);
  polynomial(std::shared_ptr<std::set<long, rev_comp_t>> degrees);
  int compare(std::shared_ptr<polynomial> o);
  std::shared_ptr<polynomial> poly_xor(std::shared_ptr<polynomial> that);
  std::shared_ptr<polynomial> mod(std::shared_ptr<polynomial> that);
  std::shared_ptr<polynomial> shift_left(long shift);
  std::shared_ptr<polynomial> multiply(std::shared_ptr<polynomial> that);
  std::shared_ptr<polynomial> reduce_exponent(int p);
  std::shared_ptr<polynomial> gcd(std::shared_ptr<polynomial> that);
  std::shared_ptr<polynomial> mod_pow(long e, std::shared_ptr<polynomial> m);

  long get_degree();
  bool is_empty();
  bool has_degree(long k);
  bool is_reducible();
  bool get_reducibility_ben_or();
};


#endif //CLIONCPP_POLYNOMIAL_HPP
