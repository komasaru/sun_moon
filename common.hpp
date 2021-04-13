#ifndef CONV_TIME_COMMON_HPP_
#define CONV_TIME_COMMON_HPP_

#include <ctime>
#include <iomanip>
#include <string>

namespace conv_time {

// -------------------------------------
//   Functions
// -------------------------------------
struct timespec jst2utc(struct timespec);   // 変換: JST -> UTC
std::string gen_time_str(struct timespec);  // 日時文字列生成

}  // namespace conv_time

#endif

