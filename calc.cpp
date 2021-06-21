#include "calc.hpp"

namespace sun_moon {

// 定数
static constexpr unsigned int kJstUtc    = 9;                // JST - UTC (hours)
static constexpr unsigned int kJstOffset = 32400;            // JST - UTC (secs)
static constexpr unsigned int kSecDay    = 86400;            // Seconds inf a day (secs)
static constexpr double       kDipCoef   = 0.0353333;        // 地平線伏角計算用係数
static constexpr double       kEps       = 0.5e-4;           // ループ処理閾値
static constexpr double       kPi        = atan(1.0) * 4.0;  // 円周率
static constexpr double       kPi180     = kPi / 180.0;      // 円周率 / 180
static constexpr double       kAstrRef   = 0.585556;         // 大気差(astro refract)

/*
 * @brief  コンストラクタ
 *
 * @param  none
 */
Calc::Calc(struct timespec jst, double lat, double lng, double ht) {
  struct timespec utc;  // UTC
  double dut1;          // DUT1
  int    utc_tai;       // UTC - TAI (協定世界時と国際原子時の差 = うるう秒の総和)
  double dlt_t;         // ΔT (TT(地球時) と UT1(世界時1)の差)

  try {
    this->jst = jst;
    this->lat_o = lat;
    this->lng_o = lng;
    this->ht_o  = ht;
    // JST -> UTC
    utc.tv_sec  = jst.tv_sec - kJstOffset;
    utc.tv_nsec = 0;
    // 初期処理
    Time o_tm;
    utc_tai = o_tm.get_utc_tai(utc);
    dut1    = o_tm.get_dut1(utc);
    dlt_t   = o_tm.calc_dlt_t(utc, utc_tai, dut1);
    this->dlt_t_d = dlt_t / kSecDay;
    this->dip     = kDipCoef * std::sqrt(ht_o);
    this->day_p   = calc_day_progress();
  } catch (...) {
    throw;
  }
}

/*
 * @brief   計算: 2000年1月1日力学時正午からの経過日数
 *
 * @param   none
 * @return  経過日数 (double)
 */
double Calc::calc_day_progress() {
  struct tm    t;
  unsigned int y;
  unsigned int m;
  unsigned int d;
  double       day_p;

  try {
    localtime_r(&jst.tv_sec, &t);
    y = t.tm_year + 1900 - 2000;
    m = t.tm_mon + 1;
    d = t.tm_mday;
    // 1月,2月は前年の13月,14月とする
    if (m < 3) {
      --y;
      m += 12;
    }
    day_p = 365.0 * y + 30.0 * m + d
          - 33.5 - kJstUtc / 24.0
          + int(3 * (m + 1) / 5.0)
          + int(y / 4.0);
  } catch (...) {
    throw;
  }

  return day_p;
}

/*
 * @brief      計算: 日の出／入
 *
 * @param[in]  区分(0: 日の出, 1: 日の入, 2: 日の南中) (unsigned int)
 * @return     日の出／入／南中の時刻と方位角／南中高度 (struct TmAh)
 */
struct TmAh Calc::calc_sun(unsigned int kbn) {
  double      dd;    // 時刻(単位: 日)
  double      dd_s;  // 時刻(単位: 秒)
  double      jy;    // 経過ユリウス年
  Coord       cd_k;  // 黄道座標
  struct TmAh sun;

  try {
    dd = calc_time_sun(kbn);
    dd_s = dd * kSecDay;
    sun.time.tv_sec  = jst.tv_sec + int(dd_s);
    sun.time.tv_nsec = jst.tv_nsec + (dd_s - int(dd_s)) * 1.0e9;
    if (sun.time.tv_nsec >= 1.0e9) {
      sun.time.tv_nsec -= 1.0e9;
      ++sun.time.tv_sec;
    };
    jy   = (day_p + dd + dlt_t_d) / 365.25;
    cd_k = {0.0, calc_lmd_sun(jy)};
    if (kbn == 2) {
      sun.ah = calc_height(cd_k, dd, jy);
    } else {
      sun.ah = calc_angle(cd_k, dd, jy);
    }
  } catch (...) {
    throw;
  }

  return sun;
}

/*
 * @brief      計算: 月の出／入
 *
 * @param[in]  区分(0: 月の出, 1: 月の入, 2: 月の南中) (unsigned int)
 * @return     月の出／入／南中の時刻と方位角／南中高度 (struct TmAh)
 */
struct TmAh Calc::calc_moon(unsigned int kbn) {
  double      dd;    // 時刻(単位: 日)
  double      dd_s;  // 時刻(単位: 秒)
  double      jy;    // 経過ユリウス年
  Coord       cd_k;  // 黄道座標
  struct TmAh moon;

  try {
    dd = calc_time_moon(kbn);
    dd_s = dd * kSecDay;
    if (dd < 0.0) {
      // 月の出／入がない場合
      moon.time = {0, 0};
      moon.ah = -1.0;
    } else {
      moon.time.tv_sec  = jst.tv_sec + int(dd_s);
      moon.time.tv_nsec = jst.tv_nsec + (dd_s - int(dd_s)) * 1.0e9;
      if (moon.time.tv_nsec >= 1.0e9) {
        moon.time.tv_nsec -= 1.0e9;
        ++moon.time.tv_sec;
      };
      jy   = (day_p + dd + dlt_t_d) / 365.25;
      cd_k = {calc_bet_moon(jy), calc_lmd_moon(jy)};
      if (kbn == 2) {
        moon.ah = calc_height(cd_k, dd, jy);
      } else {
        moon.ah = calc_angle(cd_k, dd, jy);
      }
    }
  } catch (...) {
    throw;
  }

  return moon;
}

/*
 * @brief      計算: 日の出・入・南中時刻
 *
 * @param[in]  区分(0: 出, 1: 入, 2: 南中) (unsigned int)
 * @return     時刻(日) (double)
 */
double Calc::calc_time_sun(unsigned int kbn) {
  double jy;                // 経過ユリウス年
  double rev = 1.0;         // 補正値
  double tm  = 0.5;         // 時刻(日)
  double tm_sd;             // 恒星時(日)
  double dist;              // 太陽: 距離
  double r;                 // 太陽: 視半径
  double diff;              // 太陽: 視差
  double ht;                // 太陽: 出入高度
  double hang_diff;         // 時角差
  Coord cd_k = {0.0, 0.0};  // 黄道座標
  Coord cd_s = {0.0, 0.0};  // 赤道座標

  try {
    while (std::abs(rev) > kEps) {
      jy        = (day_p + tm + dlt_t_d) / 365.25;      // tm の経過ユリウス年
      cd_k.lng  = calc_lmd_sun(jy);                     // 太陽の黄経
      dist      = calc_dist_sun(jy);                    // 太陽の距離
      cd_s      = ko2se(jy, cd_k);                      // 黄道 -> 赤道変換
      r         = 0.266994 / dist;                      // 太陽の視半径
      diff      = 0.0024428 / dist;                     // 太陽の視差
      ht        = -r - kAstrRef - dip + diff;           // 太陽の出入高度
      tm_sd     = tm_sidereal(jy, tm);                  // 恒星時
      hang_diff = hour_ang_diff(cd_s, tm_sd, ht, kbn);  // 時角差
      rev       = hang_diff / 360.0;                    // 仮定時刻に対する補正値
      tm       += rev;
    }
  } catch (...) {
    throw;
  }

  return tm;
}

/*
 * @brief      計算: 月の出・入・南中時刻
 *
 * @param[in]  区分(0: 出, 1: 入, 2: 南中) (unsigned int)
 * @return     時刻(日) (double)
 */
double Calc::calc_time_moon(unsigned int kbn) {
  double jy;                // 経過ユリウス年
  double rev = 1.0;         // 補正値
  double tm  = 0.5;         // 時刻(日)
  double tm_sd;             // 恒星時(日)
  double diff;              // 月: 視差
  double ht = 0.0;          // 月: 出入高度
  double hang_diff;         // 時角差
  Coord cd_k = {0.0, 0.0};  // 黄道座標
  Coord cd_s = {0.0, 0.0};  // 赤道座標

  try {
    while (std::abs(rev) > kEps) {
      jy = (day_p + tm + dlt_t_d) / 365.25;             // tm の経過ユリウス年
      cd_k.lng = calc_lmd_moon(jy);                     // 月の黄経
      cd_k.lat = calc_bet_moon(jy);                     // 月の黄緯
      cd_s = ko2se(jy, cd_k);                           // 黄道 -> 赤道変換
      // 南中の時は計算しない
      if (kbn != 2) {
        diff = calc_diff_moon(jy);                      // 月の視差
        ht   = diff - dip - kAstrRef;                   // 月の出入高度
      }
      tm_sd     = tm_sidereal(jy, tm);                  // 恒星時
      hang_diff = hour_ang_diff(cd_s, tm_sd, ht, kbn);  // 時角差
      rev       = hang_diff / 347.8;                    // 仮定時刻に対する補正値
      tm       += rev;
    }
    // 月の出／入がない場合は -1.0 とする
    if (tm < 0.0 || tm >= 1.0) { tm = -1.0; }
  } catch (...) {
    throw;
  }

  return tm;
}

/*
 * @brief      計算: 太陽の黄経
 *
 * @param[in]  経過ユリウス年 (double)
 * @return     黄経 (double)
 */
double Calc::calc_lmd_sun(double jy) {
  double lmd;

  try {
    lmd = 0.0003 * std::sin(kPi180 * norm_ang(329.7  +   44.43  * jy))
        + 0.0003 * std::sin(kPi180 * norm_ang(352.5  + 1079.97  * jy))
        + 0.0004 * std::sin(kPi180 * norm_ang( 21.1  +  720.02  * jy))
        + 0.0004 * std::sin(kPi180 * norm_ang(157.3  +  299.30  * jy))
        + 0.0004 * std::sin(kPi180 * norm_ang(234.9  +  315.56  * jy))
        + 0.0005 * std::sin(kPi180 * norm_ang(291.2  +   22.81  * jy))
        + 0.0005 * std::sin(kPi180 * norm_ang(207.4  +    1.50  * jy))
        + 0.0006 * std::sin(kPi180 * norm_ang( 29.8  +  337.18  * jy))
        + 0.0007 * std::sin(kPi180 * norm_ang(206.8  +   30.35  * jy))
        + 0.0007 * std::sin(kPi180 * norm_ang(153.3  +   90.38  * jy))
        + 0.0008 * std::sin(kPi180 * norm_ang(132.5  +  659.29  * jy))
        + 0.0013 * std::sin(kPi180 * norm_ang( 81.4  +  225.18  * jy))
        + 0.0015 * std::sin(kPi180 * norm_ang(343.2  +  450.37  * jy))
        + 0.0018 * std::sin(kPi180 * norm_ang(251.3  +    0.20  * jy))
        + 0.0018 * std::sin(kPi180 * norm_ang(297.8  + 4452.67  * jy))
        + 0.0020 * std::sin(kPi180 * norm_ang(247.1  +  329.64  * jy))
        + 0.0048 * std::sin(kPi180 * norm_ang(234.95 +   19.341 * jy))
        + 0.0200 * std::sin(kPi180 * norm_ang(355.05 +  719.981 * jy))
        + (1.9146 - 0.00005 * jy)
        * std::sin(kPi180 * norm_ang(357.538 + 359.991 * jy))
        + norm_ang(280.4603 + 360.00769 * jy);
  } catch (...) {
    throw;
  }

  return lmd;
}

/*
 * @brief      計算: 太陽の距離
 *
 * @param[in]  経過ユリウス年 (double)
 * @return     距離 (double)
 */
double Calc::calc_dist_sun(double jy) {
  double dist;

  try {
    dist = 0.000007 * std::sin(kPi180 * norm_ang(156.0 +  329.6  * jy))
         + 0.000007 * std::sin(kPi180 * norm_ang(254.0 +  450.4  * jy))
         + 0.000013 * std::sin(kPi180 * norm_ang( 27.8 + 4452.67 * jy))
         + 0.000030 * std::sin(kPi180 * norm_ang( 90.0))
         + 0.000091 * std::sin(kPi180 * norm_ang(265.1 +  719.98 * jy))
         + (0.007256 - 0.0000002 * jy)
         * std::sin(kPi180 * norm_ang(267.54 + 359.991 * jy));
    dist = std::pow(10.0, dist);
  } catch (...) {
    throw;
  }

  return dist;
}

/*
 * @brief      計算: 月の黄緯
 *
 * @param[in]  経過ユリウス年 (double)
 * @return     黄経 (double)
 */
double Calc::calc_bet_moon(double jy) {
  double bm;
  double bet;

  try {
    bm  = 0.0005 * std::sin(kPi180 * norm_ang(307.0   + 19.4       * jy))
        + 0.0026 * std::sin(kPi180 * norm_ang( 55.0   + 19.34      * jy))
        + 0.0040 * std::sin(kPi180 * norm_ang(119.5   +  1.33      * jy))
        + 0.0043 * std::sin(kPi180 * norm_ang(322.1   + 19.36      * jy))
        + 0.0267 * std::sin(kPi180 * norm_ang(234.95  + 19.341     * jy));
    bet = 0.0003 * std::sin(kPi180 * norm_ang(234.0   + 19268.0    * jy))
        + 0.0003 * std::sin(kPi180 * norm_ang(146.0   +  3353.3    * jy))
        + 0.0003 * std::sin(kPi180 * norm_ang(107.0   + 18149.4    * jy))
        + 0.0003 * std::sin(kPi180 * norm_ang(205.0   + 22642.7    * jy))
        + 0.0004 * std::sin(kPi180 * norm_ang(147.0   + 14097.4    * jy))
        + 0.0004 * std::sin(kPi180 * norm_ang( 13.0   +  9325.4    * jy))
        + 0.0004 * std::sin(kPi180 * norm_ang( 81.0   + 10242.6    * jy))
        + 0.0004 * std::sin(kPi180 * norm_ang(238.0   + 23281.3    * jy))
        + 0.0004 * std::sin(kPi180 * norm_ang(311.0   +  9483.9    * jy))
        + 0.0005 * std::sin(kPi180 * norm_ang(239.0   +  4193.4    * jy))
        + 0.0005 * std::sin(kPi180 * norm_ang(280.0   +  8485.3    * jy))
        + 0.0006 * std::sin(kPi180 * norm_ang( 52.0   + 13617.3    * jy))
        + 0.0006 * std::sin(kPi180 * norm_ang(224.0   +  5590.7    * jy))
        + 0.0007 * std::sin(kPi180 * norm_ang(294.0   + 13098.7    * jy))
        + 0.0008 * std::sin(kPi180 * norm_ang(326.0   +  9724.1    * jy))
        + 0.0008 * std::sin(kPi180 * norm_ang( 70.0   + 17870.7    * jy))
        + 0.0010 * std::sin(kPi180 * norm_ang( 18.0   + 12978.66   * jy))
        + 0.0011 * std::sin(kPi180 * norm_ang(138.3   + 19147.99   * jy))
        + 0.0012 * std::sin(kPi180 * norm_ang(148.2   +  4851.36   * jy))
        + 0.0012 * std::sin(kPi180 * norm_ang( 38.4   +  4812.68   * jy))
        + 0.0013 * std::sin(kPi180 * norm_ang(155.4   +   379.35   * jy))
        + 0.0013 * std::sin(kPi180 * norm_ang( 95.8   +  4472.03   * jy))
        + 0.0014 * std::sin(kPi180 * norm_ang(219.2   +   299.96   * jy))
        + 0.0015 * std::sin(kPi180 * norm_ang( 45.8   +  9964.00   * jy))
        + 0.0015 * std::sin(kPi180 * norm_ang(211.1   +  9284.69   * jy))
        + 0.0016 * std::sin(kPi180 * norm_ang(135.7   +   420.02   * jy))
        + 0.0017 * std::sin(kPi180 * norm_ang( 99.8   + 14496.06   * jy))
        + 0.0018 * std::sin(kPi180 * norm_ang(270.8   +  5192.01   * jy))
        + 0.0018 * std::sin(kPi180 * norm_ang(243.3   +  8206.68   * jy))
        + 0.0019 * std::sin(kPi180 * norm_ang(230.7   +  9244.02   * jy))
        + 0.0021 * std::sin(kPi180 * norm_ang(170.1   +  1058.66   * jy))
        + 0.0022 * std::sin(kPi180 * norm_ang(331.4   + 13377.37   * jy))
        + 0.0025 * std::sin(kPi180 * norm_ang(196.5   +  8605.38   * jy))
        + 0.0034 * std::sin(kPi180 * norm_ang(319.9   +  4433.31   * jy))
        + 0.0042 * std::sin(kPi180 * norm_ang(103.9   + 18509.35   * jy))
        + 0.0043 * std::sin(kPi180 * norm_ang(307.6   +  5470.66   * jy))
        + 0.0082 * std::sin(kPi180 * norm_ang(144.9   +  3713.33   * jy))
        + 0.0088 * std::sin(kPi180 * norm_ang(176.7   +  4711.96   * jy))
        + 0.0093 * std::sin(kPi180 * norm_ang(277.4   +  8845.31   * jy))
        + 0.0172 * std::sin(kPi180 * norm_ang(  3.18  + 14375.997  * jy))
        + 0.0326 * std::sin(kPi180 * norm_ang(328.96  + 13737.362  * jy))
        + 0.0463 * std::sin(kPi180 * norm_ang(172.55  +   698.667  * jy))
        + 0.0554 * std::sin(kPi180 * norm_ang(194.01  +  8965.374  * jy))
        + 0.1732 * std::sin(kPi180 * norm_ang(142.427 +  4073.3220 * jy))
        + 0.2777 * std::sin(kPi180 * norm_ang(138.311 +    60.0316 * jy))
        + 0.2806 * std::sin(kPi180 * norm_ang(228.235 +  9604.0088 * jy))
        + 5.1282 * std::sin(kPi180 * norm_ang( 93.273 +  4832.0202 * jy + bm));
  } catch (...) {
    throw;
  }

  return bet;
}

/*
 * @brief      計算: 月の黄経
 *
 * @param[in]  経過ユリウス年 (double)
 * @return     黄経 (double)
 */
double Calc::calc_lmd_moon(double jy) {
  double am;
  double lmd;

  try {
    am  = 0.0006 * std::sin(kPi180 * norm_ang( 54.0   + 19.3       * jy))
        + 0.0006 * std::sin(kPi180 * norm_ang( 71.0   +  0.2       * jy))
        + 0.0020 * std::sin(kPi180 * norm_ang( 55.0   + 19.34      * jy))
        + 0.0040 * std::sin(kPi180 * norm_ang(119.5   +  1.33      * jy));
    lmd = 0.0003 * std::sin(kPi180 * norm_ang(280.0   + 23221.3    * jy))
        + 0.0003 * std::sin(kPi180 * norm_ang(161.0   +    40.7    * jy))
        + 0.0003 * std::sin(kPi180 * norm_ang(311.0   +  5492.0    * jy))
        + 0.0003 * std::sin(kPi180 * norm_ang(147.0   + 18089.3    * jy))
        + 0.0003 * std::sin(kPi180 * norm_ang( 66.0   +  3494.7    * jy))
        + 0.0003 * std::sin(kPi180 * norm_ang( 83.0   +  3814.0    * jy))
        + 0.0004 * std::sin(kPi180 * norm_ang( 20.0   +   720.0    * jy))
        + 0.0004 * std::sin(kPi180 * norm_ang( 71.0   +  9584.7    * jy))
        + 0.0004 * std::sin(kPi180 * norm_ang(278.0   +   120.1    * jy))
        + 0.0004 * std::sin(kPi180 * norm_ang(313.0   +   398.7    * jy))
        + 0.0005 * std::sin(kPi180 * norm_ang(332.0   +  5091.3    * jy))
        + 0.0005 * std::sin(kPi180 * norm_ang(114.0   + 17450.7    * jy))
        + 0.0005 * std::sin(kPi180 * norm_ang(181.0   + 19088.0    * jy))
        + 0.0005 * std::sin(kPi180 * norm_ang(247.0   + 22582.7    * jy))
        + 0.0006 * std::sin(kPi180 * norm_ang(128.0   +  1118.7    * jy))
        + 0.0007 * std::sin(kPi180 * norm_ang(216.0   +   278.6    * jy))
        + 0.0007 * std::sin(kPi180 * norm_ang(275.0   +  4853.3    * jy))
        + 0.0007 * std::sin(kPi180 * norm_ang(140.0   +  4052.0    * jy))
        + 0.0008 * std::sin(kPi180 * norm_ang(204.0   +  7906.7    * jy))
        + 0.0008 * std::sin(kPi180 * norm_ang(188.0   + 14037.3    * jy))
        + 0.0009 * std::sin(kPi180 * norm_ang(218.0   +  8586.0    * jy))
        + 0.0011 * std::sin(kPi180 * norm_ang(276.5   + 19208.02   * jy))
        + 0.0012 * std::sin(kPi180 * norm_ang(339.0   + 12678.71   * jy))
        + 0.0016 * std::sin(kPi180 * norm_ang(242.2   + 18569.38   * jy))
        + 0.0018 * std::sin(kPi180 * norm_ang(  4.1   +  4013.29   * jy))
        + 0.0020 * std::sin(kPi180 * norm_ang( 55.0   +    19.34   * jy))
        + 0.0021 * std::sin(kPi180 * norm_ang(105.6   +  3413.37   * jy))
        + 0.0021 * std::sin(kPi180 * norm_ang(175.1   +   719.98   * jy))
        + 0.0021 * std::sin(kPi180 * norm_ang( 87.5   +  9903.97   * jy))
        + 0.0022 * std::sin(kPi180 * norm_ang(240.6   +  8185.36   * jy))
        + 0.0024 * std::sin(kPi180 * norm_ang(252.8   +  9224.66   * jy))
        + 0.0024 * std::sin(kPi180 * norm_ang(211.9   +   988.63   * jy))
        + 0.0026 * std::sin(kPi180 * norm_ang(107.2   + 13797.39   * jy))
        + 0.0027 * std::sin(kPi180 * norm_ang(272.5   +  9183.99   * jy))
        + 0.0037 * std::sin(kPi180 * norm_ang(349.1   +  5410.62   * jy))
        + 0.0039 * std::sin(kPi180 * norm_ang(111.3   + 17810.68   * jy))
        + 0.0040 * std::sin(kPi180 * norm_ang(119.5   +     1.33   * jy))
        + 0.0040 * std::sin(kPi180 * norm_ang(145.6   + 18449.32   * jy))
        + 0.0040 * std::sin(kPi180 * norm_ang( 13.2   + 13317.34   * jy))
        + 0.0048 * std::sin(kPi180 * norm_ang(235.0   +    19.34   * jy))
        + 0.0050 * std::sin(kPi180 * norm_ang(295.4   +  4812.66   * jy))
        + 0.0052 * std::sin(kPi180 * norm_ang(197.2   +   319.32   * jy))
        + 0.0068 * std::sin(kPi180 * norm_ang( 53.2   +  9265.33   * jy))
        + 0.0079 * std::sin(kPi180 * norm_ang(278.2   +  4493.34   * jy))
        + 0.0085 * std::sin(kPi180 * norm_ang(201.5   +  8266.71   * jy))
        + 0.0100 * std::sin(kPi180 * norm_ang( 44.89  + 14315.966  * jy))
        + 0.0107 * std::sin(kPi180 * norm_ang(336.44  + 13038.696  * jy))
        + 0.0110 * std::sin(kPi180 * norm_ang(231.59  +  4892.052  * jy))
        + 0.0125 * std::sin(kPi180 * norm_ang(141.51  + 14436.029  * jy))
        + 0.0153 * std::sin(kPi180 * norm_ang(130.84  +   758.698  * jy))
        + 0.0305 * std::sin(kPi180 * norm_ang(312.49  +  5131.979  * jy))
        + 0.0348 * std::sin(kPi180 * norm_ang(117.84  +  4452.671  * jy))
        + 0.0410 * std::sin(kPi180 * norm_ang(137.43  +  4411.998  * jy))
        + 0.0459 * std::sin(kPi180 * norm_ang(238.18  +  8545.352  * jy))
        + 0.0533 * std::sin(kPi180 * norm_ang( 10.66  + 13677.331  * jy))
        + 0.0572 * std::sin(kPi180 * norm_ang(103.21  +  3773.363  * jy))
        + 0.0588 * std::sin(kPi180 * norm_ang(214.22  +   638.635  * jy))
        + 0.1143 * std::sin(kPi180 * norm_ang(  6.546 +  9664.0404 * jy))
        + 0.1856 * std::sin(kPi180 * norm_ang(177.525 +   359.9905 * jy))
        + 0.2136 * std::sin(kPi180 * norm_ang(269.926 +  9543.9773 * jy))
        + 0.6583 * std::sin(kPi180 * norm_ang(235.700 +  8905.3422 * jy))
        + 1.2740 * std::sin(kPi180 * norm_ang(100.738 +  4133.3536 * jy))
        + 6.2887 * std::sin(kPi180 * norm_ang(134.961 +  4771.9886 * jy + am))
        + norm_ang(218.3161 + 4812.67881 * jy);
  } catch (...) {
    throw;
  }

  return lmd;
}

/*
 * @brief      計算: 月の視差
 *
 * @param[in]  経過ユリウス年 (double)
 * @return     視差 (double)
 */
double Calc::calc_diff_moon(double jy) {
  double diff;

  try {
    diff = 0.0003 * std::sin(kPi180 * norm_ang(227.0  +  4412.0   * jy))
         + 0.0004 * std::sin(kPi180 * norm_ang(194.0  +  3773.4   * jy))
         + 0.0005 * std::sin(kPi180 * norm_ang(329.0  +  8545.4   * jy))
         + 0.0009 * std::sin(kPi180 * norm_ang(100.0  + 13677.3   * jy))
         + 0.0028 * std::sin(kPi180 * norm_ang(  0.0  +  9543.98  * jy))
         + 0.0078 * std::sin(kPi180 * norm_ang(325.7  +  8905.34  * jy))
         + 0.0095 * std::sin(kPi180 * norm_ang(190.7  +  4133.35  * jy))
         + 0.0518 * std::sin(kPi180 * norm_ang(224.98 +  4771.989 * jy))
         + 0.9507 * std::sin(kPi180 * norm_ang(90.0));
  } catch (...) {
    throw;
  }

  return diff;
}

/*
 * @brief      黄道座標 -> 赤道座標 変換
 *
 * @param[in]  経過ユリウス年 (double)
 * @param[in]  黄道座標 (Coord)
 * @return     赤道座標 (Coord)
 */
Coord Calc::ko2se(double jy, Coord cd_k) {
  double ang_k;  // 黄道傾角
  double lmd;    // 黄経(rad)
  double bet;    // 黄緯(rad)
  double a;
  double b;
  double c;
  Coord  cd_s = {0.0, 0.0};

  try {
    ang_k = (23.439291 - 0.000130042 * jy) * kPi180;
    lmd = cd_k.lng * kPi180;  // 黄経
    bet = cd_k.lat * kPi180;  // 黄緯
    a =  cos(bet) * cos(lmd);
    b = -sin(bet) * sin(ang_k)
      +  cos(bet) * sin(lmd) * cos(ang_k);
    c =  sin(bet) * cos(ang_k)
      +  cos(bet) * sin(lmd) * sin(ang_k);
    cd_s.lng = std::atan(b / a) / kPi180;
    // aがマイナスのときは 90°< α < 270° → 180°加算する
    //if (a < 0.0) { cd_s.lat += 180.0; }
    if (a < 0.0) { cd_s.lng += 180.0; }
    cd_s.lat = std::asin(c) / kPi180;
  } catch (...) {
    throw;
  }

  return cd_s;
}

/*
 * @brief      計算: 観測地点の恒星時Θ(度)
 *
 * @param[in]  経過ユリウス年 (double)
 * @param[in]  時刻 (double)
 * @return     恒星時Θ(度) (double)
 */
double Calc::tm_sidereal(double jy, double tm) {
  double tm_sd;

  try {
    tm_sd = norm_ang(325.4606
                   + 360.007700536 * jy
                   + 0.00000003879 * jy * jy
                   + 360.0 * tm + lng_o);
  } catch (...) {
    throw;
  }

  return tm_sd;
}

/*
 * @brief      計算: 出入点(k)の時角(tk)と天体の時角(t)との差(dt=tk-t)
 *
 * @param[in]  天体の赤緯・赤経 (Coord)
 * @param[in]  恒星時Θ(度) (double)
 * @param[in]  観測地点の出没高度(度) (double)
 * @param[in]  区分(0: 出, 1: 入, 2: 南中) (double)
 * @return     時角差 (double)
 */
double Calc:: hour_ang_diff(
    Coord cd_s, double tm_sd, double ht, unsigned int kbn) {
  double dt;
  double tk;

  try {
    // 南中の場合は天体の時角を返す
    if (kbn == 2) {
      tk = 0.0;
    } else {
      tk  = std::sin(kPi180 * ht)
          - std::sin(kPi180 * cd_s.lat) * std::sin(kPi180 * lat_o);
      tk /= std::cos(kPi180 * cd_s.lat) * std::cos(kPi180 * lat_o);
      // 出没点の時角
      tk = std::acos(tk) / kPi180;
      // tkは出のときマイナス、入のときプラス
      if (kbn == 0 && tk > 0.0) { tk = std::abs(tk) * -1.0; }
      if (kbn == 1 && tk < 0.0) { tk = std::abs(tk); }
    }
    // 天体の時角
    dt = tk - tm_sd + cd_s.lng;
    // dtの絶対値を180°以下に調整
    if (dt >  180.0) {
      while (dt > 180.0) { dt -= 360.0; }
    }
    if (dt < -180.0) {
      while (dt < -180.0) { dt += 360.0; }
    }
  } catch (...) {
    throw;
  }

  return dt;
}

/*
 * @brief      計算: 時刻(t)における黄経、黄緯(λ(jy),β(jy))の天体の方位角(ang)
 *
 * @param[in]  天体の座標 (Coord)
 * @param[in]  経過ユリウス年 (double)
 * @param[in]  時刻 (0.xxxx日) (double)
 * @return     角度(xx.x度) (double)
 */
double Calc::calc_angle(Coord cd_k, double dd, double jy) {
  Coord  cd_s;   // 赤道座標
  double tm_sd;  // 恒星時(日)
  double hang;   // 天体の時角
  double a_0;
  double a_1;
  double ang;    // 角度

  try {
    cd_s  = ko2se(jy, cd_k);            // 黄道 -> 赤道変換
    tm_sd = tm_sidereal(jy, dd);        // 恒星時
    hang  = tm_sd - cd_s.lng;           // 天体の時角
    // 天体の方位角
    a_0 = -std::cos(kPi180 * cd_s.lat) * std::sin(kPi180 * hang);
    a_1 =  std::sin(kPi180 * cd_s.lat) * std::cos(kPi180 * lat_o)
        -  std::cos(kPi180 * cd_s.lat) * std::sin(kPi180 * lat_o)
         * std::cos(kPi180 * hang);
    ang = std::atan(a_0 / a_1) / kPi180;
    // 分母がプラスのときは -90°< ang < 90°
    if (a_1 > 0.0 && ang < 0.0) { ang += 360.0; }
    // 分母がマイナスのときは 90°< ang < 270° → 180°加算する
    if (a_1 < 0.0) { ang += 180.0; }
  } catch (...) {
    throw;
  }

  return ang;
}

/*
 * @brief      計算: 時刻(t)における黄経、黄緯(λ(jy),β(jy))の天体の高度(height)
 *
 * @param[in]  天体の座標 (Coord)
 * @param[in]  経過ユリウス年 (double)
 * @param[in]  時刻 (0.xxxx日) (double)
 * @return     高度(xx.x度) (double)
 */
double Calc::calc_height(Coord cd_k, double dd, double jy) {
  Coord  cd_s;    // 赤道座標
  double tm_sd;   // 恒星時(日)
  double hang;    // 天体の時角
  double tan_ht;  // 計算用
  double h;       // 計算用
  double ht;      // 高度

  try {
    cd_s  = ko2se(jy, cd_k);            // 黄道 -> 赤道変換
    tm_sd = tm_sidereal(jy, dd);        // 恒星時
    hang  = tm_sd - cd_s.lng;           // 天体の時角
    // 天体の高度
    ht  = std::sin(kPi180 * cd_s.lat) * sin(kPi180 * lat_o)
        + std::cos(kPi180 * cd_s.lat) * cos(kPi180 * lat_o)
        * std::cos(kPi180 * hang);
    ht  = std::asin(ht) / kPi180;
    // フランスの天文学者ラドー(R.Radau)の計算式
    // * 平均大気差と1秒程度の差で大気差を求めることが可能
    //   (標準的大気(気温10ﾟC，気圧1013.25hPa)の場合)
    //   (視高度 4ﾟ以上)
    tan_ht = std::tan(kPi180 * (90.0 - ht));
    h   = (58.76 - (0.406 - 0.0192 * tan_ht) * tan_ht) * tan_ht / 3600.0;
    ht += h;
  } catch (...) {
    throw;
  }

  return ht;
}

/*
 * @brief   計算: 角度の正規化
 *                (角度を 0 以上 360 未満にする)
 *
 * @param[in]  角度（正規化前） (double)
 * @return     角度（正規化後） (double)
 */
double Calc::norm_ang(double ang_src) {
  double ang;

  try {
    ang = ang_src - 360.0 * int(ang_src / 360.0);
  } catch (...) {
    throw;
  }

  return ang;
}

}  // namespace sun_moon

