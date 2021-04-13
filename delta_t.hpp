#ifndef SUN_MOON_DELTA_T_HPP_
#define SUN_MOON_DELTA_T_HPP_

namespace sun_moon {

float calc_dlt_t_bf_m500(double);  // ΔT (                year <  -500)
float calc_dlt_t_bf_0500(double);  // ΔT (-500 <= year && year <   500)
float calc_dlt_t_bf_1600(double);  // ΔT ( 500 <= year && year <  1600)
float calc_dlt_t_bf_1700(double);  // ΔT (1600 <= year && year <  1700)
float calc_dlt_t_bf_1800(double);  // ΔT (1700 <= year && year <  1800)
float calc_dlt_t_bf_1860(double);  // ΔT (1800 <= year && year <  1860)
float calc_dlt_t_bf_1900(double);  // ΔT (1860 <= year && year <  1900)
float calc_dlt_t_bf_1920(double);  // ΔT (1900 <= year && year <  1920)
float calc_dlt_t_bf_1941(double);  // ΔT (1920 <= year && year <  1941)
float calc_dlt_t_bf_1961(double);  // ΔT (1941 <= year && year <  1961)
float calc_dlt_t_bf_1986(double);  // ΔT (1961 <= year && year <  1986)
float calc_dlt_t_bf_2005(double);  // ΔT (1986 <= year && year <  2005)
float calc_dlt_t_bf_2050(double);  // ΔT (2005 <= year && year <  2050)
float calc_dlt_t_to_2150(double);  // ΔT (2050 <= year && year <= 2150)
float calc_dlt_t_af_2150(double);  // ΔT (2150 <  year                )

}  // namespace sun_moon

#endif

