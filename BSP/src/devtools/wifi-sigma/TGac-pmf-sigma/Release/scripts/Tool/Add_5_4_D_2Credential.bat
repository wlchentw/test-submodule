
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
REM -f_prefre [1:prefer; (defult)0:non-prefer]
REM ====== END         ======
adb shell wpa_cli -g@android:wpa_wlan0 IFNAME=wlan0 sta_add_cred -tuname_pwd -uwifi-user -ptest%11 -rmail.example.com -acas.pem -f0
adb shell wpa_cli -g@android:wpa_wlan0 IFNAME=wlan0 enable_sw_sim

echo Complete adding credential for test plan 5_4_D 2nd credential...
PAUSE