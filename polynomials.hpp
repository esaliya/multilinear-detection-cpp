//
// Created by Saliya Ekanayake on 7/13/17.
//

#include <vector>

#ifndef CLIONCPP_POLYNOMIALS_HPP
#define CLIONCPP_POLYNOMIALS_HPP

#endif //CLIONCPP_POLYNOMIALS_HPP

class polynomials{
public:
  /**
	 * Returns the index of the maximum set bit. If no bits are set, returns -1.
	 */
  static int get_max_bit(long l) {
    for (int i = 64 - 1; i >= 0; i--) {
      if (get_bit(l, i))
        return i;
    }
    return -1;
  }
  /**
	 * Returns the value of the bit at index of the long. The right most bit is
	 * at index 0.
	 */
  static bool get_bit(long l, int index) {
    return (((l >> index) & 1) == 1);
  }

  /**
   * Returns the value of the bit at index of the byte. The right most bit is
   * at index 0.
   */
  static bool get_bit(char b, int index) {
    return (((b >> index) & 1) == 1);
  }

  /**
   * Returns the value of the bit at index of the byte. The right most bit is
   * at index 0 of the last byte in the array.
   */
  static bool get_bit(std::vector<char> bytes, int index) {
    // byte array index
    int aidx = (int)((bytes.size() - 1) - (index / 8));
    // bit index
    int bidx = index % 8;
    // byte
    char b = bytes[aidx];
    // bit
    return get_bit(b, bidx);
  }

};