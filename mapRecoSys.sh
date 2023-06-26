#!/bin/bash
./build/serverplatform >/dev/null 2>&1&

port_lst=(60001 60002 60003 60004 60005 60006 60007 60008 60009 60010 60011 60012 60013 60014 60015
          60016 60017 60018 60019 60020 60021 60022 60023 60024 60025 60026 60027 60028 60029 60030
          60031 60032 60033 60034 60035 60036 60037 60038 60039 60040 60041 60042 60043 60044 60045
          60046 60047 60048 60049 60050 60051 60052 60053 60054 60055 60056 60057 60058 60059 60060
          60061 60062 60063 60064 60065 60066 60067 60068 60069 60070 60071 60072 60073 60074 60075
          60076 60077 60078 60079 60080 60081 60082 60083 60084 60085 60086 60087 60088 60089 60090
          60091 60092 60093 60094 60095 60096 60097 60098 60099 60100 60101 60102 60103 60104 60105)

x0_cor=(41228 41165 42052 41306 41232 40944 41434 42449)
y0_cor=(3543 3315 3376 3165 3766 3303 3197 3450)

x1_cor=(41432 41085 41373 41366 41095 41015 41077 40690 41062 42008 41086 41052 40817 40653 40943)
y1_cor=(3429 3493 3473 3492 3301 3313 3351 3306 3154 3469 3676 3726 3756 4230 3239)

x2_cor=(41280 41309 41376 41505 41867 41828 42137 40970 41048 40905 40824 41424)
y2_cor=(3426 3396 3414 3429 3423 3360 3443 3330 3326 3319 3295 3444)

x3_cor=(41241 41351 40706 42125)
y3_cor=(3564 3438 3232 4016)

x4_cor=(41380 41327 41080 40908)
y4_cor=(3677 3936 3544 3751)

x5_cor=(41428 41335 41158 42258 42490 42533)
y5_cor=(3413 3383 3543 3310 3429 3422)

x6_cor=(41641 41205 41422 41589 41273 41468 41737)
y6_cor=(3330 3401 3513 3511 3512 3461 3421)

# echo "${port_lst[$port_index]}, ${x0_cor[$i]}, ${y0_cor[$i]}"
variety_cnt=0
port_index=$[$variety_cnt * 15]
for ((i=0;i<${#x0_cor[*]};i++)); do
    ./build/merchant -p ${port_lst[$port_index]} -x ${x0_cor[$i]} -y ${y0_cor[$i]} >/dev/null 2>&1&
    port_index=$[$port_index + 1]
done
variety_cnt=$[$variety_cnt + 1]

port_index=$[$variety_cnt * 15]
for ((i=0;i<${#x1_cor[*]};i++)); do
    ./build/merchant -p ${port_lst[$port_index]} -x ${x1_cor[$i]} -y ${y1_cor[$i]} >/dev/null 2>&1&
    port_index=$[$port_index + 1]
done
variety_cnt=$[$variety_cnt + 1]

port_index=$[$variety_cnt * 15]
for((i=0;i<${#x2_cor[*]};i++)); do
    ./build/merchant -p ${port_lst[$port_index]} -x ${x2_cor[$i]} -y ${y2_cor[$i]} >/dev/null 2>&1&
    port_index=$[$port_index + 1]
done
variety_cnt=$[$variety_cnt + 1]

port_index=$[$variety_cnt * 15]
for((i=0;i<${#x3_cor[*]};i++)); do
    ./build/merchant -p ${port_lst[$port_index]} -x ${x3_cor[$i]} -y ${y3_cor[$i]} >/dev/null 2>&1&
    port_index=$[$port_index + 1]
done
variety_cnt=$[$variety_cnt + 1]

port_index=$[$variety_cnt * 15]
for((i=0;i<${#x4_cor[*]};i++)); do
    ./build/merchant -p ${port_lst[$port_index]} -x ${x4_cor[$i]} -y ${y4_cor[$i]} >/dev/null 2>&1&
    port_index=$[$port_index + 1]
done
variety_cnt=$[$variety_cnt + 1]

port_index=$[$variety_cnt * 15]
for((i=0;i<${#x5_cor[*]};i++)); do
    ./build/merchant -p ${port_lst[$port_index]} -x ${x5_cor[$i]} -y ${y5_cor[$i]} >/dev/null 2>&1&
    port_index=$[$port_index + 1]
done
variety_cnt=$[$variety_cnt + 1]

port_index=$[$variety_cnt * 15]
for((i=0;i<${#x6_cor[*]};i++)); do
    ./build/merchant -p ${port_lst[$port_index]} -x ${x6_cor[$i]} -y ${y6_cor[$i]} >/dev/null 2>&1&
    port_index=$[$port_index + 1]
done

echo "请输入期望查询半径(米)..."
read radius

echo "请输入要查询的类别索引..."
echo "----------------------------------------------------------------------------------------------"
echo "|  类别  |  银行(0)  |  餐饮(1)  |  旅馆(2)  |  医疗(3)  |  教育(4)  |  商场(5)  |  其他(6)  |"
echo "----------------------------------------------------------------------------------------------"
read variety

./build/client -r $radius -v $variety -x 41556 -y 3407