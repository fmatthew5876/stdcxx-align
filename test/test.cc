#include "memory.hh"
#include <utility>
#include <iostream>
#include <cstdint>
#include <climits>
#include <string>
#include <numeric_limits>

template <typename T>
class Test {
  public:

    void au(T x, size_t a, T exp) {
      auto got = std::align_up(x, a);
      std::cout << "Test: align_up<" << _tname << ">(" << +x << ", " << a << ") == " << +exp << " : ";
      if(got == exp) { std::cout << "OK"; } else { std::cout << "FAIL (" << +got << ")"; }
      std::cout << std::endl;
    }
    void ad(T x, size_t a, T exp) {
      auto got = std::align_down(x, a);
      std::cout << "Test: align_down<" << _tname << ">(" << +x << ", " << a << ") == " << +exp << " : ";
      if(got == exp) { std::cout << "OK"; } else { std::cout << "FAIL (" << +got << ")"; }
      std::cout << std::endl;
    }
    void ia(T x, size_t a, bool exp) {
      auto got = std::is_aligned(x, a);
      std::cout << "Test: is_aligned<" << _tname << ">(" << +x << ", " << a << ") == " << exp << " : ";
      if(got == exp) { std::cout << "OK"; } else { std::cout << "FAIL (" << got << ")"; }
      std::cout << std::endl;
    }

    Test() {
      _tname = std::is_unsigned<T>::value ? "uint" : "int";
      _tname += std::to_string(sizeof(T) * 8) + "_t";
    }

  private:
    std::string _tname;
};


template <typename T>
static void test() {
  Test<T> t;
  for(T i = 0; i < 16; ++i) {
    for(size_t j = 1; j <= 128; j*=2) {
      auto ru = (i % T(j)) == 0 ? i : (i + T(j)) - (i % T(j));
      auto rd = (i % T(j)) == 0 ? i : i - (i % T(j));
      auto ri = i % T(j) == 0;
      t.au(i, j, ru);
      t.ad(i, j, rd);
      t.ia(i, j, ri);
    }
  }
}

int main() {
  test<int8_t>();
  test<int16_t>();
  test<int32_t>();
  test<int64_t>();
  test<uint8_t>();
  test<uint16_t>();
  test<uint32_t>();
  test<uint64_t>();

  return 0;
}
