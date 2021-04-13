#include "time.hpp"

namespace sun_moon {

// 定数
static constexpr unsigned int kJstOffset = 9;       // JST - UTC (hours)
static constexpr unsigned int kSecHour   = 3600;    // Seconds in an hour
static constexpr double       kTtTai     = 32.184;  // TT - TAI

/*
 * @brief      変換: JST -> UTC
 *
 * @param[in]  JST (timespec)
 * @return     UTC (timespec)
 */
struct timespec jst2utc(struct timespec ts_jst) {
  struct timespec ts;

  try {
    ts.tv_sec  = ts_jst.tv_sec - kJstOffset * kSecHour;
    ts.tv_nsec = ts_jst.tv_nsec;
  } catch (...) {
    throw;
  }

  return ts;
}

/*
 * @brief      日時文字列生成
 *
 * @param[in]  日時 (timespec)
 * @return     日時文字列 (string)
 */
std::string gen_time_str(struct timespec ts) {
  struct tm t;
  std::stringstream ss;
  std::string str_tm;

  try {
    localtime_r(&ts.tv_sec, &t);
    ss << std::setfill('0')
       << std::setw(4) << t.tm_year + 1900 << "-"
       << std::setw(2) << t.tm_mon + 1     << "-"
       << std::setw(2) << t.tm_mday        << " "
       << std::setw(2) << t.tm_hour        << ":"
       << std::setw(2) << t.tm_min         << ":"
       << std::setw(2) << t.tm_sec         << "."
       << std::setw(3) << ts.tv_nsec / 1000000;
    return ss.str();
  } catch (...) {
    throw;
  }
}

/*
 * @brief  コンストラクタ
 *
 * @param  none
 */
Time::Time() {
  try {
    // うるう秒, DUT1 一覧取得
    l_ls.reserve(50);    // 予めメモリ確保
    l_dut.reserve(250);  // 予めメモリ確保
    File o_f;
    if (!o_f.get_leap_sec_list(l_ls)) throw;
    if (!o_f.get_dut1_list(l_dut))    throw;
    dlt_t = 0.0;
  } catch (...) {
    throw;
  }
}

/*
 * @brief       UTC - TAI (協定世界時と国際原子時の差 = うるう秒の総和) 取得
 *
 * @param[in]   UTC (timespec)
 * @return      UTC - TAI (int)
 */
int Time::get_utc_tai(struct timespec ts) {
  struct tm t;
  std::stringstream ss;      // 対象年月日算出用
  std::string dt_t;          // 対象年月日
  std::string buf;           // 1行分バッファ
  int i;                     // ループインデックス
  utc_tai = 0;               // 初期化

  try {
    // 対象年月日
    localtime_r(&ts.tv_sec, &t);
    ss << std::setw(4) << std::setfill('0') << std::right
       << t.tm_year + 1900
       << std::setw(2) << std::setfill('0') << std::right
       << t.tm_mon + 1
       << std::setw(2) << std::setfill('0') << std::right
       << t.tm_mday;
    dt_t = ss.str();

    // うるう秒取得
    for (i = l_ls.size() - 1; i >= 0; --i) {
      if (l_ls[i][0] <= dt_t) {
        utc_tai = stoi(l_ls[i][1]);
        break;
      }
    }

    return utc_tai;
  } catch (...) {
    throw;
  }
}

/*
 * @brief       DUT1 (UT1(世界時1) と UTC(協定世界時)の差) 取得
 *
 * @param[in]   UTC (timespec)
 * @return      DUT1 (float)
 */
float Time::get_dut1(struct timespec ts) {
  struct tm t;
  std::stringstream ss;    // 対象年月日算出用
  std::string dt_t;        // 対象年月日
  std::string buf;         // 1行分バッファ
  int i;                   // ループインデックス
  dut1 = 0.0;              // 初期化

  try {
    // 対象年月日
    localtime_r(&ts.tv_sec, &t);
    ss << std::setw(4) << std::setfill('0') << std::right
       << t.tm_year + 1900
       << std::setw(2) << std::setfill('0') << std::right
       << t.tm_mon + 1
       << std::setw(2) << std::setfill('0') << std::right
       << t.tm_mday;
    dt_t = ss.str();

    // DUT1 取得
    for (i = l_dut.size() - 1; i >= 0; --i) {
      if (l_dut[i][0] <= dt_t) {
        dut1 = stod(l_dut[i][1]);
        break;
      }
    }

    return dut1;
  } catch (...) {
    throw;
  }
}

/*
 * @brief   ΔT (TT(地球時) と UT1(世界時1)の差) 計算
 *
 * @param   時刻 (timespec)
 * @param   UTC - TAI (int)
 * @param   DUT1 (float)
 * @return  ΔT (float)
 */
float Time::calc_dlt_t(struct timespec ts, int utc_tai, float dut1) {
  struct tm t;
  int    year;  // 西暦年（対象年）
  double y;     // 西暦年（計算用）

  try {
    if (dlt_t != 0.0) return dlt_t;
    if (utc_tai != 0) return kTtTai - utc_tai - dut1;
    localtime_r(&ts.tv_sec, &t);
    year = t.tm_year + 1900;
    y = year + (t.tm_mon + 1 - 0.5) / 12;

    if        (                 year <  -500) {
      dlt_t = calc_dlt_t_bf_m500(y);
    } else if ( -500 <= year && year <   500) {
      dlt_t = calc_dlt_t_bf_0500(y);
    } else if (  500 <= year && year <  1600) {
      dlt_t = calc_dlt_t_bf_1600(y);
    } else if ( 1600 <= year && year <  1700) {
      dlt_t = calc_dlt_t_bf_1700(y);
    } else if ( 1700 <= year && year <  1800) {
      dlt_t = calc_dlt_t_bf_1800(y);
    } else if ( 1800 <= year && year <  1860) {
      dlt_t = calc_dlt_t_bf_1860(y);
    } else if ( 1860 <= year && year <  1900) {
      dlt_t = calc_dlt_t_bf_1900(y);
    } else if ( 1900 <= year && year <  1920) {
      dlt_t = calc_dlt_t_bf_1920(y);
    } else if ( 1920 <= year && year <  1941) {
      dlt_t = calc_dlt_t_bf_1941(y);
    } else if ( 1941 <= year && year <  1961) {
      dlt_t = calc_dlt_t_bf_1961(y);
    } else if ( 1961 <= year && year <  1986) {
      dlt_t = calc_dlt_t_bf_1986(y);
    } else if ( 1986 <= year && year <  2005) {
      dlt_t = calc_dlt_t_bf_2005(y);
    } else if ( 2005 <= year && year <  2050) {
      dlt_t = calc_dlt_t_bf_2050(y);
    } else if ( 2050 <= year && year <= 2150) {
      dlt_t = calc_dlt_t_to_2150(y);
    } else if ( 2150 <  year                ) {
      dlt_t = calc_dlt_t_af_2150(y);
    }
  } catch (...) {
    throw;
  }

  return dlt_t;
}

}  // namespace sun_moon

