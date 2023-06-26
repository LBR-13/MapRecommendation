import math
import numpy as np
CONSTANTS_RADIUS_OF_EARTH = 6371000.     # meters (m)


def GPStoXY(lat, lon, ref_lat, ref_lon):
    # input GPS and Reference GPS in degrees
    # output XY in meters (m) X:North Y:East
    lat_rad = math.radians(lat)
    lon_rad = math.radians(lon)
    ref_lat_rad = math.radians(ref_lat)
    ref_lon_rad = math.radians(ref_lon)

    sin_lat = math.sin(lat_rad)
    cos_lat = math.cos(lat_rad)
    ref_sin_lat = math.sin(ref_lat_rad)
    ref_cos_lat = math.cos(ref_lat_rad)

    cos_d_lon = math.cos(lon_rad - ref_lon_rad)

    arg = np.clip(ref_sin_lat * sin_lat + ref_cos_lat *
                  cos_lat * cos_d_lon, -1.0, 1.0)
    c = math.acos(arg)

    k = 1.0
    if abs(c) > 0:
        k = (c / math.sin(c))

    x = float(k * (ref_cos_lat * sin_lat - ref_sin_lat *
              cos_lat * cos_d_lon) * CONSTANTS_RADIUS_OF_EARTH)
    y = float(k * cos_lat * math.sin(lon_rad - ref_lon_rad)
              * CONSTANTS_RADIUS_OF_EARTH)
    print(int(x),int(-y))
    return x, -y


# import math

# def distance(lat1, lon1, lat2, lon2):
#     R = 6371  # 地球半径，单位为公里
#     phi1 = math.radians(lat1)
#     phi2 = math.radians(lat2)
#     delta_phi = math.radians(lat2 - lat1)
#     delta_lambda = math.radians(lon2 - lon1)
#     a = math.sin(delta_phi / 2)**2 + math.cos(phi1) * math.cos(phi2) * math.sin(delta_lambda / 2)**2
#     c = 2 * math.atan2(math.sqrt(a), math.sqrt(1 - a))
#     d = R * c  # d 单位为公里
#     return d

# print(GPStoXY(120.3924340, 36.0773895, 120, 36))

# print(GPStoXY(120.3924340, 36.0773895, 120,36))


(GPStoXY(120.370793	,	36.063021,  120,36))
(GPStoXY(120.370225	,	36.058975,  120,36))
(GPStoXY(120.378205	,	36.060047,  120,36))
(GPStoXY(120.371494	,	36.056296,  120,36))
(GPStoXY(120.370834	,	36.066994,  120,36))
(GPStoXY(120.368237	,	36.058759,  120,36))
(GPStoXY(120.372642	,	36.056877,  120,36))
(GPStoXY(120.381772	,	36.061348,  120,36))
(GPStoXY(120.372627,    36.060994	,120,36))
(GPStoXY(120.36951,     36.062148   ,120,36))
(GPStoXY(120.372096,    36.061779	,120,36))
(GPStoXY(120.372031,    36.062126	,120,36))
(GPStoXY(120.369591,    36.058726	,120,36))
(GPStoXY(120.368872,    36.058939	,120,36))
(GPStoXY(120.369435,    36.059611	,120,36))
(GPStoXY(120.365954,    36.058822	,120,36))
(GPStoXY(120.369296,    36.056107	,120,36))
(GPStoXY(120.377804,    36.061705	,120,36))
(GPStoXY(120.369516,    36.065396	,120,36))
(GPStoXY(120.36921,     36.066293   ,120,36))
(GPStoXY(120.367096,    36.066831	,120,36))
(GPStoXY(120.365626,    36.075252	,120,36))
(GPStoXY(120.368228,    36.057625	,120,36))
(GPStoXY(120.371263,    36.060941	,120,36))
(GPStoXY(120.37152,     36.060412   ,120,36))
(GPStoXY(120.372121,    36.060741	,120,36))
(GPStoXY(120.37328,     36.060997   ,120,36))
(GPStoXY(120.376541,    36.06088   ,120,36))
(GPStoXY(120.376187,    36.05977   ,120,36))
(GPStoXY(120.378966,    36.061231	,120,36))
(GPStoXY(120.368468,    36.059245	,120,36))
(GPStoXY(120.369171,    36.059180   ,120,36))
(GPStoXY(120.367883,    36.059046,	120,36))
(GPStoXY(120.367154,    36.058621,	120,36))
(GPStoXY(120.370909	,	36.063405,  120,36))
(GPStoXY(120.371901	,	36.061154,  120,36))
(GPStoXY(120.366091	,	36.057511,  120,36))
(GPStoXY(120.378862	,	36.071431,  120,36))
(GPStoXY(120.372161	,	36.065419,  120,36))
(GPStoXY(120.371684	,	36.070017,  120,36))
(GPStoXY(120.369464	,	36.063053,  120,36))
(GPStoXY(120.367913	,	36.066735,  120,36))
(GPStoXY(120.372593	,	36.060720,   120,36))
(GPStoXY(120.371756	,	36.060177,  120,36))
(GPStoXY(120.370158	,	36.063022,  120,36))
(GPStoXY(120.38005	,	36.058863,  120,36))
(GPStoXY(120.382141	,	36.060982,  120,36))
(GPStoXY(120.382527	,	36.060848,  120,36))
(GPStoXY(120.374502	,	36.059230,   120,36))
(GPStoXY(120.37058	,	36.060496,  120,36))
(GPStoXY(120.372538	,	36.062487,  120,36))
(GPStoXY(120.374035	,	36.062448,  120,36))
(GPStoXY(120.371197	,	36.062478,  120,36))
(GPStoXY(120.372952	,	36.061559,  120,36))
(GPStoXY(120.375366	,	36.060846,  120,36))
(GPStoXY(120.373743, 36.060597,  120,36))

(GPStoXY(120.372552,36.061264,  120,36))
