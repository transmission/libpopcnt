///
/// @file  benchmark.cpp
/// @brief Simple benchmark program for libpopcnt.h, repeatedly
///        counts the 1 bits inside a vector.
///
/// Usage: ./benchmark [array bytes] [iters]
///
/// Copyright (C) 2019 Kim Walisch, <kim.walisch@gmail.com>
///
/// This file is distributed under the BSD License. See the LICENSE
/// file in the top level directory.
///

#include <libpopcnt.h>

#include <iostream>
#include <iomanip>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <stdint.h>
#include <string>

double get_seconds()
{
  return (double) std::clock() / CLOCKS_PER_SEC;
}

// init vector with random data
void init(std::vector<uint8_t>& vect)
{
  std::srand((unsigned) std::time(0));

  for (size_t i = 0; i < vect.size(); i++)
    vect[i] = (uint8_t) std::rand();
}

// count 1 bits inside vector
uint64_t benchmark(const std::vector<uint8_t>& vect, int iters)
{
  uint64_t total = 0;
  int old = - 1;

  for (int i = 0; i < iters; i++)
  {
    int percent = (int)(100.0 * i / iters);
    if (percent > old)
    {
      std::cout << "\rStatus: " << percent << "%" << std::flush;
      old = percent;
    }
    total += popcnt(&vect[0], vect.size());
  }

  return total;
}

void verify(uint64_t cnt, uint64_t total, int iters)
{
  if (cnt != total / iters)
  {
    std::cerr << "libpopcnt verification failed!" << std::endl;
    std::exit(1);
  }
}

int main(int argc, char* argv[])
{
  int bytes = (1 << 10) * 16;
  int iters = 10000000;

  if (argc > 1)
    bytes = std::atoi(argv[1]);
  if (argc > 2)
    iters = std::atoi(argv[2]);

  uint64_t cnt = 0;
  std::vector<uint8_t> vect(bytes);
  std::string algo;
  init(vect);

  std::cout << "Iters: " << iters << std::endl;

  if (bytes < 1024)
    std::cout << "Array size: " << bytes << " bytes" << std::endl;
  else if (bytes < 1024 * 1024)
    std::cout << "Array size: " << std::fixed << std::setprecision(2) << bytes / 1024.0 << " KB" << std::endl;
  else
    std::cout << "Array size: " << std::fixed << std::setprecision(2) << bytes / (1024.0 * 1024.0) << " MB" << std::endl;

#if defined(LIBPOPCNT_X86_OR_X64)

  #if defined(LIBPOPCNT_HAVE_CPUID)
    int cpuid = get_cpuid();
    if ((cpuid & LIBPOPCNT_BIT_AVX512_VPOPCNTDQ) && bytes >= 40)
      algo = "AVX512";
    else if ((cpuid & LIBPOPCNT_BIT_AVX2) && bytes >= 512)
      algo = "AVX2";
    else if (cpuid & LIBPOPCNT_BIT_POPCNT)
      algo = "POPCNT";
  #else
    #if defined(LIBPOPCNT_HAVE_AVX512) && (defined(__AVX512__) || \
                                          (defined(__AVX512F__) && \
                                           defined(__AVX512BW__) && \
                                           defined(__AVX512VPOPCNTDQ__)))
      if (algo.empty() && bytes >= 40)
        algo = "AVX512";
    #endif
    #if defined(LIBPOPCNT_HAVE_AVX2) && defined(__AVX2__)
      if (algo.empty() && bytes >= 512)
        algo = "AVX2";
    #endif
    #if defined(LIBPOPCNT_HAVE_POPCNT) && defined(__POPCNT__)
      if (algo.empty())
        algo = "POPCNT";
    #endif
  #endif

#elif defined(__ARM_FEATURE_SVE) && \
      __has_include(<arm_sve.h>)
  algo = "ARM SVE";
#elif (defined(__ARM_NEON) || \
       defined(__aarch64__)) && \
      __has_include(<arm_neon.h>)
  algo = "ARM NEON";
#elif defined(__PPC64__)
  algo = "POPCNTD";
#endif

  if (algo.empty())
    algo = "integer popcount";

  std::cout << "Algorithm: " << algo << std::endl;

  for (size_t i = 0; i < vect.size(); i++)
    cnt += popcnt64_bitwise(vect[i]);

  double seconds = get_seconds();
  uint64_t total = benchmark(vect, iters);
  seconds = get_seconds() - seconds;

  std::cout << "\rStatus: 100%" << std::endl;
  std::cout << "Seconds: " << std::fixed << std::setprecision(2) << seconds << std::endl;

  double total_bytes = (double) bytes * (double) iters;
  double GB = total_bytes / 1e9;
  double GBs = GB / seconds;

  std::cout << std::fixed << std::setprecision(1) << GBs << " GB/s" << std::endl;
  verify(cnt, total, iters);

  return 0;
}
