//
// Created by Saliya Ekanayake on 7/12/17.
//

#include <random>
#include <bitset>
#include <map>
#include <iostream>
#include <algorithm>
#include "polynomial.hpp"
const std::shared_ptr<polynomial> polynomial::X(create_from_long(2L));
const std::shared_ptr<polynomial> polynomial::ONE(create_from_long(1L));

std::shared_ptr<polynomial> polynomial::create_irreducible(int degree) {
  while (true){
    std::shared_ptr<polynomial> p = create_random(degree);
    if (!p->is_reducible()){
      return p;
    }
  }
}

std::shared_ptr<polynomial> polynomial::create_irreducible(int degree, std::function<int()> &gen_rnd_byte) {
  while (true){
    std::shared_ptr<polynomial> p = create_random(degree, gen_rnd_byte);
    if (!p->is_reducible()){
      return p;
    }
  }
}

long polynomial::to_long() {
  long b = 0L;
  for (const long &v : (*degrees)){
    b = b | (1 << v);
  }
  return  b;
}

polynomial::~polynomial() {
}

std::shared_ptr<polynomial> polynomial::create_from_long(long l) {
  std::shared_ptr<std::set<long, rev_comp_t>> dgrs = create_degrees_collection();
  for (int i = 0; i < 64; i++) {
    if (((l >> i) & 1) == 1)
      dgrs->insert((long)i);
  }
  return std::shared_ptr<polynomial>(new polynomial(dgrs));
}

std::shared_ptr<polynomial> polynomial::create_from_bytes(std::vector<char> bytes, int degree) {
  std::shared_ptr<std::set<long, rev_comp_t>> dgrs = create_degrees_collection();
  for (int i = 0; i < degree; i++) {
    if (polynomials::get_bit(bytes, i)) {
      dgrs->insert((long) i);
    }
  }
  dgrs->insert((long)degree);
  return std::shared_ptr<polynomial>(new polynomial(dgrs));
}

std::shared_ptr<polynomial> polynomial::create_random(int degree){
  std::default_random_engine re;
  std::uniform_int_distribution<int> unif(-128,127);

  std::vector<char> bytes((unsigned long) ((degree / 8) + 1));
  auto gen = std::bind(unif, re);
  std::generate(std::begin(bytes), std::end(bytes), gen);

  return create_from_bytes(bytes, degree);
}

std::shared_ptr<polynomial> polynomial::create_random(int degree, std::function<int()> &gen_rnd_byte) {
  int len =  ((degree / 8) + 1);
  std::vector<char> bytes((unsigned long) len);
  for (int i = 0; i < len; ++i){
    // It's OK to cast this to char because our rnd generator
    // generates only within [-128,127] range
    bytes.push_back((char &&) gen_rnd_byte());
  }
  return create_from_bytes(bytes, degree);
}

std::shared_ptr<std::set<long, rev_comp_t>> polynomial::create_degrees_collection() {
  rev_comp_t rev_comparator = [](const long& lhs, const long& rhs) -> bool{return lhs > rhs;};
  return std::make_shared<std::set<long, rev_comp_t>>(rev_comparator);
}

bool polynomial::test_bit(long v, int n) {
  return ((v & (1<<n)) != 0);
}

int polynomial::bit_count(long v) {
  std::bitset<sizeof(long)*8> bs((unsigned long)v);
  int count = (int) bs.count();
  int total_length = sizeof(long)*8;
  return bs[total_length-1] == 1 ? (total_length - count) : count;
}

int polynomial::compare(long a, long b) {
  return (a < b ? -1 : (a == b ? 0 : 1));
}

polynomial::polynomial() {
  degrees = create_degrees_collection();
}

polynomial::polynomial(std::shared_ptr<polynomial> poly)
    : polynomial(poly->degrees){
  // Nothing to do here
}

/* Check with Jose, if we really need this copying(cloning) of degrees.
 * Could have stored the shared pointer itself. But I am doing the cloning too for now*/
polynomial::polynomial(std::shared_ptr<std::set<long, rev_comp_t>> degrees)
    : degrees(std::make_shared<std::set<long, rev_comp_t>>(*degrees)){
  // nothing to do here
}

int polynomial::compare(std::shared_ptr<polynomial> o) {
  long deg = get_degree();
  long odeg = o->get_degree();
  int cmp = compare(deg, odeg);
  if (cmp != 0) {
    return cmp;
  }
  std::shared_ptr<polynomial> x = poly_xor(o);
  if (x->is_empty()) return 0;
  return has_degree(x->get_degree()) ? 1 : -1;
}

std::shared_ptr<polynomial> polynomial::poly_xor(std::shared_ptr<polynomial> that) {
  rev_comp_t rev_comparator = [](const long& lhs, const long& rhs) -> bool{return lhs > rhs;};
  std::set<long, rev_comp_t> dgrs0 = *degrees;
  std::set<long, rev_comp_t> diff0(rev_comparator);
  std::set_difference(dgrs0.begin(), dgrs0.end(), (*that->degrees).begin(), (*that->degrees).end(),
                      std::inserter(diff0, diff0.begin()));
  dgrs0 = diff0;

  // The copy constructor copies content of "*(that->degrees)"
  std::shared_ptr<std::set<long, rev_comp_t>> dgrs1 = std::make_shared<std::set<long, rev_comp_t>>(*(that->degrees));
  std::set<long, rev_comp_t> diff1(rev_comparator);
  std::set_difference((*dgrs1).begin(), (*dgrs1).end(), (*degrees).begin(), (*degrees).end(),
                      std::inserter(diff1, diff1.begin()));
  *dgrs1 = diff1;
  (*dgrs1).insert(dgrs0.begin(), dgrs0.end());

  // Can't call make_shared because polynomical constructor is private, so can be used only within polynomial class
  return std::shared_ptr<polynomial>(new polynomial(dgrs1));
}

std::shared_ptr<polynomial> polynomial::mod(std::shared_ptr<polynomial> that) {
  long da = get_degree();
  long db = that->get_degree();
  std::shared_ptr<polynomial> reg = std::shared_ptr<polynomial>(new polynomial(degrees));
  for (long i = (da - db); compare(i, 0) >= 0; i = i - 1){
    if (reg->has_degree(i+db)){
      std::shared_ptr<polynomial> shifted = that->shift_left(i);
      reg = reg->poly_xor(shifted);
    }
  }
  return reg;
}

std::shared_ptr<polynomial> polynomial::shift_left(long shift) {
  std::shared_ptr<std::set<long, rev_comp_t>> dgrs = create_degrees_collection();
  for (const long &degree : (*degrees)){
    long shifted = degree + shift;
    dgrs->insert(shifted);
  }
  return std::shared_ptr<polynomial>(new polynomial(dgrs));
}

std::shared_ptr<polynomial> polynomial::multiply(std::shared_ptr<polynomial> that) {
  std::shared_ptr<std::set<long, rev_comp_t>> dgrs = create_degrees_collection();
  for (const long &pa : (*degrees)){
    for (const long &pb : *(that->degrees)){
      long sum = pa+pb;
      // xor the result
      std::set<long,rev_comp_t>::iterator it = dgrs->find(sum);
      if (it != dgrs->end()){
        // dgrs set contains sum
        dgrs->erase(it);
      } else {
        dgrs->insert(sum);
      }
    }
  }
  return std::shared_ptr<polynomial>(new polynomial(dgrs));
}

std::shared_ptr<polynomial> polynomial::reduce_exponent(int p) {
  // compute (x^q^p mod f)
  long q_to_p = (long) std::pow(Q, p);
  std::shared_ptr<polynomial> x_to_q_to_p
      = X->mod_pow(q_to_p, shared_from_this());

  // subtract (x mod f)
  return x_to_q_to_p->poly_xor(X)->mod(shared_from_this());
}

std::shared_ptr<polynomial> polynomial::gcd(std::shared_ptr<polynomial> that) {
  std::shared_ptr<polynomial> a = std::shared_ptr<polynomial>(new polynomial(shared_from_this()));
  while (!that->is_empty()) {
    std::shared_ptr<polynomial> t = std::shared_ptr<polynomial>(new polynomial(that));
    that = a->mod(that);
    a = t;
  }
  return a;
}

std::shared_ptr<polynomial> polynomial::mod_pow(long e, std::shared_ptr<polynomial> m) {
  std::shared_ptr<polynomial> result = polynomial::ONE;
  std::shared_ptr<polynomial> b = std::shared_ptr<polynomial>(new polynomial(shared_from_this()));
  while (bit_count(e) != 0){
    if (test_bit(e, 0)){
      result = result->multiply(b)->mod(m);
    }
    e = e >> 1;
    b = b->multiply(b)->mod(m);
  }
  return result;
}

long polynomial::get_degree() {
  if (degrees->empty()){
    return -1L;
  }
  return *(degrees->begin());
}

bool polynomial::is_empty() {
  return degrees->empty();
}

bool polynomial::has_degree(long k) {
  return (degrees->find(k) != degrees->end());
}

/* Returns true for reducible and false for irreducible*/
bool polynomial::is_reducible() {
  if (compare(polynomial::ONE) == 0) return true;
  if (compare(polynomial::X) == 0) return true;
  // do full-on reducibility test
  return get_reducibility_ben_or();
}

/**
	 * BenOr Reducibility Test
	 *
	 * Tests and Constructions of Irreducible Polynomials over Finite Fields
	 * (1997) Shuhong Gao, Daniel Panario
	 *
	 * http://citeseer.ist.psu.edu/cache/papers/cs/27167/http:zSzzSzwww.math.clemson.eduzSzfacultyzSzGaozSzpaperszSzGP97a.pdf/gao97tests.pdf
	 */
bool polynomial::get_reducibility_ben_or() {
  long degree = get_degree();
  for (int i = 1; i <= (int) (degree / 2); i++) {
    std::shared_ptr<polynomial> b = reduce_exponent(i);
    std::shared_ptr<polynomial> g = gcd(b);
    if (g->compare(ONE) != 0) {
      return true;
    }
  }
  return false;
}

std::ostream &operator<<(std::ostream &out_strm, const polynomial &p) {
  out_strm << "size: " << p.degrees->size() << " [ ";
  for (const long &v : (*p.degrees)){
     out_strm << v << " ";
  }
  out_strm << " ]\n";

  return out_strm;
}

















