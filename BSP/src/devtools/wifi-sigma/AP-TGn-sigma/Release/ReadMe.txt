0. Introduction
    This tool is designed for AP mode, including the following programs:
    ‧	WMMPS
    ‧	TGn
    ‧	PMF
    ‧	TGac

    Before starting test, need to make sure the following devices are ready.
    ‧	One notebook(CA), with Windows OS to connect with UCC and test device.
    ‧	One test phone(APUT), connecting to the notebook with USB cable.

1. Environment Setup
    Before starting sigma test, the environment and device should be configured
    first.
    ‧	Configure init file:
        a.	Setup DUT’s MAC address in init file.
            Take init_wmmps.txt for example, DutMacAddress_24G,
            DutMacAddress_5G should be set with APUT's mac address.
            And TGMacAddress should also be set the same if exist this
            attribute.

        b.	Setup PC Endpoint:
            The traffic generator’s functionality is totally embedded in APUT.
            We set the PC Endpoint’s IP address and port to be the same as
            APUT.
            For example:
            Control agent part:
                wfa_control_agent_dut!ipaddr=192.168.250.82,port=9000!
                wfa_console_ctrl!ipaddr=192.168.250.82,port=9000!
            Also remember to set the port in "ENV_SETUP.bat” for program
            wfa_ca.
                SET CA_PORT=9000

            Wireless IP part, for example:
                dut_wireless_ip!192.165.100.82!
                wfa_console_tg!192.165.100.82!
            Also remember to set this ip in "ENV_SETUP.bat”.
                SET PCENDPOINT_IP_ADDRESS=192.165.100.82
                SET PCENDPOINT_IP_NETMASK=255.255.0.0

        c. Setup bridge
            The PC Endpoint’s functionality is totally embedded in APUT. Other
            testbed APs in environment may need PCE's traffic generator
            functionality such as overlapping bss testcases. Since only one PCE
            is allowed in testing environment, bridge the APUT's network with
            the testing data network to enable traffic between APUT and other
            APs.

            Use another cable to connect the CA notebook to the testing data
            network.

            Connect APUT to CA notebook with usb cable, and enable usb
            tethering from APUT. Setup a bridge between APUT's rndis interface
            and testing data network interface on CA notebook, and set the
            bridge's ip & netmask as the same domain as that of PC Endpoint.
            For example: ip: 192.165.100.10, netmask: 255.255.0.0

2. Starting sigma
    1) Ensure "Verified Boot" is disabled if the Android's version is greater
       than P.
    2) Turn off Wi-Fi & Hotspot from GUI.
    3) Click "STEP_01_install_sigma.bat" to install essential materials to
       setup the environment.
    4) Click "STEP_02_start_sigma.bat" to run wfa_ca and wfa_dut.
    5) Start sigma testing.
    6) Click "STEP_03_stop_sigma.bat" to end sigma testing.
