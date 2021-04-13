#ifndef SUN_MOON_TIME_HPP_
#define SUN_MOON_TIME_HPP_

#include "delta_t.hpp"
#include "file.hpp"

#include <cmath>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

namespace sun_moon {

struct timespec jst2utc(struct timespec);   // 変換: JST -> UTC
std::string gen_time_str(struct timespec);  // 日時文字列生成

class Time {
  std::vector<std::vector<std::string>> l_ls;   // List of Leap Second
  std::vector<std::vector<std::string>> l_dut;  // List of DUT1
  float  dut1;     // UTC - TAI (協定世界時と国際原子時の差 = うるう秒の総和)
  int    utc_tai;  // UTC - TAI (協定世界時と国際原子時の差 = うるう秒の総和)
  float  dlt_t;    // ΔT (TT(地球時) と UT1(世界時1)の差)

public:
  Time();  // コンストラクタ
  int   get_utc_tai(struct timespec);  // UTC -> UTC - TAI
  float get_dut1(struct timespec);     // UTC -> DUT1
  float calc_dlt_t(struct timespec, int, float);  // 計算: ΔT  (TT(地球時) と UT1(世界時1)の差)

private:
};

}  // namespace sun_moon

#endif

