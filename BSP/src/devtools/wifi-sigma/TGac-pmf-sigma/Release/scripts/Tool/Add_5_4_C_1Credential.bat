
REM ====== Description ======
REM -t_type
REM -u_username
REM -p_password
REM -i_imsi
REM -n_plmn_mnc
REM -c_plmn_mcc
REM -a_root_ca
REM -r_realm
REM -q_fqdn
REM -l_clientCA
REM -f_prefer [1:prefer; (defult)0:non-prefer]
REM ====== END         ======
adb shell wpa_cli -g@android:wpa_wlan0 IFNAME=wlan0 sta_add_cred -tsim -rmail.example.com -i234560000000000 -n56 -c234 -p90dca4eda45b53cf0f12d7c9c3bc6a89:cb9cccc4b9258e6dca4760379fb82581:000000000123 -f1
adb shell wpa_cli -g@android:wpa_wlan0 IFNAME=wlan0 enable_sw_sim

echo Complete adding credential for test plan 5_4_C 1st credential...
PAUSE