#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <netpacket/packet.h>
#include <arpa/inet.h>
#include <inttypes.h>
#include <stdarg.h>
#include <unistd.h>
#include <linux/wireless.h>
//#include <sys/un.h>
#include <assert.h>
#include <sys/time.h>
#include <sys/stat.h>

#include "wfa_wpa.h"
#include "wfa_portall.h"

#if 1 //reference wfa's debug
#include "wfa_debug.h"
extern short wfa_defined_debug;
#endif

int wpa_connect(wpa_private_t* P, const char* if_name);


int checksum_ok
    ( wpa_private_t* P ///< [in] pointer to private wpa_supplicant context
    )
{
    if ( !P ) return 0;
    if ( P->checksum != WPA_CHECKSUM ) return 0;
    return 1;
}


void wpa_close
    ( int sock
    , struct sockaddr_un *local
    )
{
	if ( sock < 0 ) {
		return;
    }

	unlink(local->sun_path);
	close(sock);
}

int wpa_open
    ( const char *ctrl_path
    , struct sockaddr_un *local
    )
{
	int sock;
	int delete_old = 0;
	struct sockaddr_un dest;
	//int pid = (int) getpid();
	static int sub_index = 0;
	int rc;

	sock= socket(PF_UNIX, SOCK_DGRAM, 0);
	if ( sock < 0 ) {
		return -1;
    }

	local->sun_family = AF_UNIX;
	sub_index++;

#if 1 //fix file name!
	rc = snprintf(local->sun_path, sizeof(local->sun_path),
				"/tmp/wfa_%d", sub_index);
#else
	rc = snprintf(local->sun_path, sizeof(local->sun_path),
				"/tmp/wfa_%d-%d", pid, sub_index);
#endif	
	if ( rc < 0 || (size_t) rc >= sizeof(local->sun_path) ) {
		close(sock);
		return -1;
	}

open_sock:
	if (bind(sock, (struct sockaddr *) local,
			sizeof(struct sockaddr_un)) < 0) {
		if (!delete_old && errno == EADDRINUSE) {
			unlink(local->sun_path);
			delete_old = 1;
			goto open_sock;
		}
                  DPRINT_ERR(WFA_ERR,"%s(%d): bind socket fial: %s\n", __FILE__, __LINE__, strerror(errno) );
		close(sock);
		return -1;
	}

	chmod(local->sun_path, S_IRWXU|S_IRWXG|S_IRWXO);
	dest.sun_family = AF_UNIX;

	strcpy(dest.sun_path, ctrl_path);
	if (connect(sock, (struct sockaddr *) &dest, sizeof(dest)) < 0) {
                  DPRINT_ERR(WFA_ERR,"%s(%d): connect socket [%s] failed : %s\n", __FILE__, __LINE__, ctrl_path ,strerror(errno) ); 
		close(sock);
		unlink(local->sun_path);
		return -1;
	}

	return sock;
}

int wpa_send_cmd
    ( int sock
    , const char *cmd
    , size_t cmd_len
    , char *reply
    , size_t reply_len
    )
{
	struct timeval tv;
	fd_set rfds;
	int ret;

	if ( send(sock, cmd, cmd_len, 0) < 0 ) {
		return -1;
    }

    while ( 1 ) {
		tv.tv_sec = 12;
		tv.tv_usec = 0;

		FD_ZERO(&rfds);
		FD_SET(sock, &rfds);
		ret = select(sock + 1, &rfds, NULL, NULL, &tv);
		if ( ret < 0 ) {
			return WPA_CMD_UNKNOWN_ERROR;
        }

		if ( FD_ISSET(sock, &rfds) ) {
			ret = recv(sock, reply, reply_len, 0);
			if ( ret < 0 ) {
				return WPA_CMD_UNKNOWN_ERROR;
            }

			if ( ret > 0 && reply[0] == '<' ) {
				/* ignore unsolicited message */
				continue;
			}

			reply_len = ret;
			break;
		} else
			return WPA_CMD_TIMEOUT;
	}

	return 0;
}

int wpa_start_cmd
    (
     char *if_name    
    , int sock
    , char *cmd
    , char *buffer
    , size_t len
    , uint8_t disp
    )
{
	int ret;

	if (sock < 0) {
		DPRINT_ERR(WFA_ERR,"Need to connected to wpa_supplicant\n");
		return -1;
	}

    len = len - 1;

	if (disp > 0)
		DPRINT_INFO(WFA_OUT,"%s : if_name=[%s] cmd=[%s] len:%zu\r\n", __FUNCTION__, if_name, cmd, len);

	ret = wpa_send_cmd(sock, cmd, strlen(cmd), buffer, len);
	if (ret == -1) {
		DPRINT_ERR(WFA_ERR,"'%s' command did not complete.\n", cmd);
		return ret;
	} else if (ret == WPA_CMD_TIMEOUT) {
		DPRINT_ERR(WFA_ERR,"'%s' command taking for ever return error.\n", cmd);
		return ret;
	} else if (ret < 0) {
		DPRINT_ERR(WFA_ERR,"'%s' Unknown error.\n", cmd);
		return -1;
	}

	buffer[len] = '\0';

	if (disp == 1) {
		DPRINT_INFO(WFA_OUT,"Resp:%s \n", buffer);
    }

	return 0;
}

int wpa_detach
    ( int sock
    )
{
	char buffer[50];
	size_t len = 50;
	int ret;

	ret = wpa_send_cmd(sock, "DETACH", 6, buffer, len);
	if ( ret < 0 ) {
		return ret;
    }

	if ( len == 3 && memcmp(buffer, "OK\n", 3) == 0 ) {
		return 0;
    }

	return -1;
}

void wpa_close_connection
    ( wpa_private_t* P
    , int call_detach
    )
{
	DPRINT_ERR(WFA_ERR,"wpa_close_connection");

	if ( call_detach ) {
		wpa_detach(P->notif_sock);
    }

	if ( P->notif_sock >= 0 ) {
		wpa_close(P->notif_sock, &P->notif_local);
    }
	P->notif_sock = -1;

	if ( P->cmd_sock >= 0 ) {
		wpa_close(P->cmd_sock, &P->cmd_local);
    }
	P->cmd_sock = -1;

	if ( P->p2p_cmd_sock >= 0 ) {
                wpa_close(P->p2p_cmd_sock, &P->p2p_cmd_local);
    }
        P->p2p_cmd_sock = -1;

	#ifdef MTK_P2P_SIGMA
	if ( P->wlan_cmd_sock >= 0 ) {
            wpa_close(P->wlan_cmd_sock, &P->wlan_cmd_local);
    }
        P->wlan_cmd_sock = -1;
	#endif
}

int wpa_reconnect
    ( wpa_private_t* P
    )
{
	wpa_close_connection(P, 0);

	return wpa_connect(P, P->if_name);
}

void wpa_ping
    ( wpa_private_t* P
    )
{
	char buff[50];
	size_t len = sizeof(buff) - 1;
	int ret;

	if ( P->cmd_sock >= 0 ) {
        ret = wpa_start_cmd(P->if_name, P->cmd_sock, "PING", buff, len, 0);
		if (ret != 0) {
            		DPRINT_ERR(WFA_ERR,"Lost connection to wpa_supplicantreconnect again\n");
            		wpa_close_connection(P, 0);
		}
    }

    if ( P->cmd_sock < 0 ) {
        wpa_reconnect(P);
    }
}

int wpa_attach
    ( int sock
    )
{
	char buffer[50];
    size_t len = 50;
	int ret;

	ret = wpa_send_cmd(sock, "ATTACH", 6, buffer, len);
	if (ret < 0) {
		return ret;
    }

	if (memcmp(buffer, "OK\n", 3) == 0) {
		return 0;
    }

	return -1;
}

void* wpa_if_notify_thread
    (wpa_private_t* P
    )
{
   	fd_set rfds;
	uint8_t buf[1024];
	int len;
	int ping_counter = 0;
	int ret;

	if ( P == NULL )
	{
		DPRINT_ERR(WFA_ERR, "invalid priv pointer\n");		
		return NULL;
	}

	//ATTACH the notification
	if (wpa_attach(P->notif_sock) == 0)
	{
		
	}
	else
	{
		DPRINT_ERR(WFA_ERR,"wpa_attach() fail!!!\n");
	}

	while ( P->notif_thread_cancel_request != 1 ) 
	{
	        if (P->cmd_sock < 0) 
		{
				DPRINT_ERR(WFA_ERR,"connection lost try reconnect\n");

				if (wpa_reconnect(P) == 0)
				{
					wpa_attach(P->notif_sock);

				}
				else
				{
					DPRINT_ERR(WFA_ERR,"wpa_reconnect() fail!!! sleep....and try again!\n");				
					sleep( 1 );
				}

				continue;
		}

	/*PC: MAKE HELPER FUNCTIONS TO GET/SET STATE*/
	struct timeval tv;

#if 1
		tv.tv_sec = 1;
#else
	tv.tv_usec = 0;
	if (local_state == IDLE)
	   	tv.tv_sec = 1;
	else if (local_state == ASSOCIATED)
		tv.tv_sec = 4;
	else
		tv.tv_sec = 1;
#endif

        FD_ZERO( &rfds );
        FD_SET( P->notif_sock, &rfds );
        ret = select( P->notif_sock + 1, &rfds, NULL, NULL, &tv );

       if (ret < 0 && errno != EINTR && errno != 0)
	{
            DPRINT_ERR(WFA_ERR,"select failed %s\n", strerror(errno));
            continue;
        }

	 if ( ! FD_ISSET( P->notif_sock, &rfds ) )
	 {

	 	if (++ping_counter >= 6)
		{
			wpa_ping(P);
			ping_counter = 0;
		}

		continue;
	  }

	  len = recv( P->notif_sock, buf, sizeof(buf), 0 );
	  if ( len < 0 )
	  {
	      DPRINT_ERR(WFA_ERR," %s\n",strerror(errno));
	      continue;
	  } 
	  else if ( len == 0 )
	  {
	      DPRINT_ERR(WFA_ERR,"Connection Closed\n");
	      wpa_close_connection(P, 1);
             continue;
	  }

	  buf[len] = '\0';

#if 1
	  DPRINT_INFO(WFA_OUT,"\r%s\n", buf);
#endif

         if (strncmp((char *)buf+3, "CTRL-EVENT-TERMINATING",
		strlen("CTRL-EVENT-TERMINATING")) == 0)
	  {
		DPRINT_ERR(WFA_ERR,"wpa_supplicant terminate");
		wpa_close_connection(P, 0);
		continue;
	   }

#if 1
		if (strncmp((char *)buf+3, "P2P-INVITATION-RESULT",
										strlen("P2P-INVITATION-RESULT")) == 0)
		{
		#ifdef MTK_P2P_SIGMA
		/*
			Format:
			P2P-INVITATION-RESULT status=0 ff:ff:ff:ff:ff:ff
		*/
		DPRINT_INFO(WFA_OUT,"P2P-INVITATION-RESULT!!\n");

		char *tmp = NULL;

				tmp = strtok(buf," ");
				tmp = strtok(NULL," ");

		/* Handle status=0 */
		tmp = strtok(tmp,"=");
				tmp = strtok(NULL,"=");

		P->recvInviteRspStatus = atoi(tmp);

		/* turn this flag to break polling */
		P->isRecvInviteRsp = 1;
		#endif
	}
	else
#endif
#if 1

        if (strncmp((char *)buf+3, "P2P-INVITATION-RECEIVED",
				strlen("P2P-INVITATION-RECEIVED")) == 0)
	{
		#ifdef MTK_P2P_SIGMA
		/* 
			Format:
			P2P-INVITATION-RECEIVED sa=02:10:18:96:2d:17 
			go_dev_addr=02:10:18:96:2d:17 bssid=02:10:18:96:ad:17 unknown-network
		*/

		DPRINT_INFO(WFA_OUT,"P2P-INVITATION-RECEIVED!!\n");
		DPRINT_INFO(WFA_OUT,"wps method: %d\n", P->confMethod);
		DPRINT_INFO(WFA_OUT,"pin: %s\n", P->keypad);

		char cmdbuf[WPS_BUF_LENGTH];
		size_t cmdbuflen = sizeof(cmdbuf) - 1;
		char cmd[WPS_CMD_LENGTH];
		char dev_addr[20]; //store go_dev_addr device address
		
		char *tmp = NULL;

		tmp = strtok(buf," ");
                tmp = strtok(NULL," ");
		/* Handle sa=02:10:18:96:2d:17 */

		tmp = strtok(NULL," ");
		/* Handle go_dev_addr=02:10:18:96:2d:17*/
		tmp = strtok(tmp,"=");
                tmp = strtok(NULL,"=");
                DPRINT_INFO(WFA_OUT,"%s\n", tmp);

                //store go_dev_addr
                dev_addr[0] = '\0';
                strcpy(dev_addr,tmp);
		
		memset(P->inviteDevAddr, '\0', sizeof(P->inviteDevAddr));
		strcpy(P->inviteDevAddr, tmp);

		/* For Invite retry connect mechanism */
		P->isOnInviteSession = 1;
			
		if (P->confMethod == KEY_PAD)
		{
			snprintf(cmd, sizeof(cmd), "P2P_CONNECT %s %s keypad join", dev_addr, P->keypad);
		}
		else
		{
			DPRINT_ERR(WFA_ERR,"recv P2P-INVITATION-RECEIVED!! TODO!\n");
			continue;
		}

		wpa_start_cmd(P->if_name, P->cmd_sock, cmd, cmdbuf, cmdbuflen, 1);
		#endif

       }else
#endif
	#ifdef MTK_P2P_SIGMA
	//P2P-GROUP-FORMATION-FAILURE
	 if(strncmp((char *)buf+3, "P2P-GROUP-FORMATION-FAILURE",
							  strlen("P2P-GROUP-FORMATION-FAILURE")) == 0)
	{
	  P->role = DEVICE;
	  P->bNegoDone = 1;

	  if (P->isOnInviteSession == 1)
	  {
			  char cmdbuf[WPS_BUF_LENGTH];
			  size_t cmdbuflen = sizeof(cmdbuf) - 1;
			  char cmd[WPS_CMD_LENGTH];

			  DPRINT_INFO(WFA_OUT,"P2P-INVITATION-RECEIVED!!\n");

			  if (P->confMethod == KEY_PAD)
			  {
					  snprintf(cmd, sizeof(cmd), "P2P_CONNECT %s %s keypad join", P->inviteDevAddr, P->keypad);
			  }
			  else
			  {
					  DPRINT_ERR(WFA_ERR,"retry P2P-INVITATION TODO!\n");
			  }

			  wpa_start_cmd(P->if_name, P->cmd_sock, cmd, cmdbuf, cmdbuflen, 1);
	  }
	} 
	#endif

#if 1
       //P2P-GO-NEG-FAILURE
	if(strncmp((char *)buf+3, "P2P-GO-NEG-FAILURE",
                                strlen("P2P-GO-NEG-FAILURE")) == 0)
	{
		P->role = DEVICE;
		P->bNegoDone = 1;
	}
	else
#endif	
       if(strncmp((char *)buf+3, "P2P-GROUP-STARTED",
				strlen("P2P-GROUP-STARTED")) == 0)
	{
		char *tmp = NULL;
		char *tmp2 = NULL;
		char tmp_buff[64];
		
		char ssid_buf[32+1]; // MAX SSID leng is 32
		char dev_add[20]; //store go_dev_addr device address
		char int_add[20]; //store go_int_addr device address
		char cmdStr[128]; //store go_int_addr device address
		char tmp_if_name[ IFNAMSIZ ];
		FILE *tmpfile = NULL;

                char *pStr = NULL;
                char *pSave = NULL;
/*
                 case 1: GO
		   P2P-GROUP-STARTED p2p0 GO ssid="DIRECT-L5" freq=2462 passphrase="EnIEEE6B" go_dev_addr=02:1f:e2:c5:3d:26

		   case 2: GC
		   P2P-GROUP-STARTED p2p0 client ssid="DIRECT-ia" freq=0 psk=2ff59eff2217c760f1616cc26a2bb5775dedc50d4e146c4603bde83b9d218d5a go_dev_addr=02:1f:e2:c5:3d:26'
*/
		tmp = strtok_r(buf," ", &pSave);
                pStr = tmp;
                if (pStr != NULL)
                    pStr += strlen(pStr) + 1;

		tmp = strtok_r(NULL," ", &pSave);
                pStr = tmp;
                if (pStr != NULL)
                    pStr += strlen(pStr) + 1;

		//interface
		//YF
	  	DPRINT_INFO(WFA_OUT,"Interface :%s\n", tmp);
		memset(P->p2p_if_name, '\0', IFNAMSIZ);
		strcpy(P->p2p_if_name, tmp);
		DPRINT_INFO(WFA_OUT,"Interface :%s[%d]\n", P->p2p_if_name, strlen(P->p2p_if_name));
		
		tmp = strtok_r(NULL," ", &pSave);
	  	DPRINT_INFO(WFA_OUT,"Role : %s\n", tmp);
                pStr = tmp;
                if (pStr != NULL)
                    pStr += strlen(pStr) + 1;

		//set the role flag to sigma DUT check
		if (!strcmp(tmp,"GO"))
		{
			P->role = GO;
		}
		else if (!strcmp(tmp,"client"))
		{
			P->role = GC;
		}
		else
		{
			P->role = DEVICE;
		}
		
		#ifdef MTK_P2P_SIGMA
		// if P2P group is formatted by invitation flow, we should start dhcp server/client here
		// to obtain/distribute ip
		if (P->isOnInviteSession == 1 || P->isInviteReinvoke == 1) {
			printf("isOnInviteSession %d, isInviteReinvoke %d!!\n", P->isOnInviteSession,
				P->isInviteReinvoke);
			if (P->role == GO) {
				printf("<GO> Starting DHCP server\n");

				sprintf(cmdStr, "ifconfig %s %s", P->p2p_if_name, DHCPD_DEF_STATIC_IP_ADDR);
				system(cmdStr);
				printf("  Executing cmd: [%s]\n", cmdStr);
				printf("%s :  enable dhcp server\n", __FUNCTION__);
				sprintf(cmdStr, "mtk_dhcp_server.sh %s", P->p2p_if_name);
				system(cmdStr);
				printf("  Executing cmd: [%s]\n", cmdStr);
				system("sleep 2");
			}
			else if (P->role == GC) {
				printf("<GC> Starting DHCP client\n");
				sprintf(cmdStr, "mtk_dhcp_client.sh %s", P->p2p_if_name);
				system(cmdStr);
				printf("  Executing cmd: [%s]\n", cmdStr);
				sprintf(cmdStr, "sleep 5");
				system(cmdStr);
				printf("  Executing cmd: [%s]\n", cmdStr);
				system("sleep 2");
			}
		}

		P->isOnInviteSession = 0;
		P->isInviteReinvoke = 0;
		#endif 
		
		tmp = strtok_r(NULL, " ", &pSave);
                if (tmp != NULL)
	  	    DPRINT_INFO(WFA_OUT,"SSID : %s\n", tmp);
                else
                    DPRINT_INFO(WFA_OUT,"get NULL pointer, failed to parse SSID\n");

		tmp_buff[0] = '\0';
		strcpy(tmp_buff,tmp); //store tmp ssid
		
		tmp = strtok_r(NULL," ", &pSave);
	  	DPRINT_INFO(WFA_OUT,"Freq : %s\n", tmp);
		
		tmp = strtok_r(NULL," ", &pSave);
	  	DPRINT_INFO(WFA_OUT,"PSK : %s\n", tmp);
		
		tmp = strtok_r(NULL," ", &pSave);
	  	DPRINT_INFO(WFA_OUT,"GO_ADDR : %s\n", tmp);

		tmp = strtok_r(tmp,"=", &pSave);
		tmp = strtok_r(NULL,"=", &pSave);		
	  	DPRINT_INFO(WFA_OUT,"%s\n", tmp);	

		//store go_dev_addr
		dev_add[0] = '\0';
		strcpy(dev_add,tmp);

		tmp2 = strtok_r(tmp_buff,"\"", &pSave);
		tmp2 = strtok_r(NULL,"\"", &pSave);
		//store SSID
		ssid_buf[0] = '\0';
		strcpy(ssid_buf,tmp2);

		P->ssid[0]='\0';
		strcpy(P->ssid,tmp2);
		
		//set the group id for sigma DUT check
		sprintf(P->group_id,"%s %s",dev_add,ssid_buf);
		
		//set to flag for sigma DUT check
		P->bNegoDone = 1;
		/*
		memset(tmp_if_name, '\0', IFNAMSIZ);
		wpa_get_if_by_role(P, &tmp_if_name);
		sprintf(cmdStr, "busybox ifconfig %s | grep HWaddr | busybox awk '{print $5}' > /tmp/sigma_mac.txt", tmp_if_name);
		printf("get mac cmd: %s\n", cmdStr);
		system(cmdStr);

		tmpfile = fopen("/tmp/sigma_mac.txt", "r+");
		if(tmpfile == NULL)
		{	
		  DPRINT_ERR(WFA_ERR, "file open failed\n");
		  return FALSE;
		}

		if(fscanf(tmpfile, "%s", int_add) == EOF)
		{
			DPRINT_ERR(WFA_ERR, "GET interface MAC failed\n");
		}
		else
		{
			sprintf(P->group_id,"%s %s",int_add,ssid_buf);
		}
		fclose(tmpfile);
		*/
       }
       else if(strncmp((char *)buf+3, "P2P-GROUP-REMOVED",
                                strlen("P2P-GROUP-REMOVED")) == 0)
       {
		//YF: close the p2p_cmd sock
		if ( P->p2p_cmd_sock > 0 ) {
			printf("===> close the P2P CMD Sock\n");
                	wpa_close(P->p2p_cmd_sock, &P->p2p_cmd_local);
    		}

        	P->p2p_cmd_sock = -1;
		memset(P->p2p_if_name, '\0', IFNAMSIZ);
		P->role = DEVICE;
#ifdef MTK_BDP_SIGMA
                char cmdStr[64] = {'\0'};
                sprintf(cmdStr, "ifconfig %s 0.0.0.0", WFA_STAUT_IF_P2P);
                system(cmdStr);
                printf("  system: %s \n",cmdStr);
                sleep(1);
#endif
	   }
    }

	wpa_close_connection(P, 1);

    return NULL;
}


int wpa_shutdown
    ( wpa_private_t *P
    )
{
	DPRINT_ERR(WFA_ERR,"wpa_shutdown\n");
	if ( P == NULL ) {
		return 1;
	}

	if ( P->notif_thread ) {
		P->notif_thread_cancel_request = 1;
        	pthread_join(P->notif_thread, NULL);
	}

	wpa_close_connection(P, 1);

    	P->checksum = 0;

	return 0;
}

int wpa_start_notifications
    ( wpa_private_t* P
    )
{
    // Create a thread to watch for notifications from the supplicant
    // driver.  Thread will run forever until it's cancelled by
    // the _shutdown() function.
    int rc = pthread_create( &P->notif_thread, NULL, wpa_if_notify_thread, P);
    if ( rc != 0 ) {
        DPRINT_ERR(WFA_ERR,"%s(%d): %s", __FILE__, __LINE__, strerror(errno) );
        wpa_close_connection(P, 0);
        return 1;
    }

    return 0;
}

#ifdef MTK_P2P_SIGMA
int wpa_wlan_cmd_connect
    ( wpa_private_t* P
    , const char* if_name
    )
{
        char sock_file[100];
#ifdef ANDROID_AOSP
	const char *supplicat_file = WPA_SUPPLICANT_CTRL_IFACE_PATH;
#else
	const char *supplicat_file = "/var/run/wpa_supplicant";
#endif

        int cmd_sock;
        int ret;
        if (P == NULL || if_name == NULL)
                return -1;

        ret = snprintf(sock_file, sizeof(sock_file), "%s/%s", supplicat_file, if_name);
        if (ret < 0 || ret > 100)
                return -1;
		printf("%s/%s", supplicat_file, if_name);
        cmd_sock = wpa_open(sock_file, &P->wlan_cmd_local);
        if (cmd_sock < 0) {
                return -1;
        }

        P->wlan_cmd_sock = cmd_sock;
        printf("%s: %s ===> %d\n", __FUNCTION__, if_name, P->wlan_cmd_sock);
        return 0;
}
#endif

wpa_private_t * wpa_init
    (char* if_name)
{
    int rc;
	
    static wpa_private_t static_priv;
    static wpa_private_t* P = &static_priv;
	
    memset( P, 0, sizeof(wpa_private_t) );
    snprintf( P->if_name, IFNAMSIZ, "%s", if_name );

    rc = wpa_connect(P, if_name);
    if (rc != 0) {
        DPRINT_ERR(WFA_ERR,"%s Can not connect to wpa_supplicant", if_name );
        return NULL;
    }
	
    #ifdef MTK_P2P_SIGMA
	// for concurrent 
	snprintf( P->wlan_if_name, IFNAMSIZ, "%s", WFA_STAUT_IF); 
	rc = wpa_wlan_cmd_connect(P, P->wlan_if_name);	
	if (rc != 0) {
		DPRINT_ERR(WFA_ERR,"%s Can not connect to wpa_supplicant" );
	//		return NULL;
	}
	#endif
	
    // Set up private variables
    P->notif_thread_cancel_request = 0;
    P->checksum = WPA_CHECKSUM;
    P->confMethod= UN_SET;
    P->keypad[0] = '\0';
    P->go_intent = 7; //default to 7
    P->bNegoDone = 0; //default to 0 , not yet!
    P->role = DEVICE; //default to DEVICE
    P->ssid[0] = '\0'; //detault to empty
    P->group_id[0] = '\0'; //detault to empty
    wpa_start_notifications(P);
    return P;
}

int wpa_p2p_cmd_connect
    ( wpa_private_t* P
    , const char* if_name
    )
{
        char sock_file[100];
#ifdef ANDROID_AOSP
	const char *supplicat_file = WPA_SUPPLICANT_CTRL_IFACE_PATH;
#else
	const char *supplicat_file = "/var/run/wpa_supplicant";
#endif
        int cmd_sock;
        int ret;
        if (P == NULL || if_name == NULL)
                return -1;
        ret = snprintf(sock_file, sizeof(sock_file), "%s/%s", supplicat_file, if_name);
        if (ret < 0 || ret > 100)
                return -1;

        cmd_sock = wpa_open(sock_file, &P->p2p_cmd_local);
        if (cmd_sock < 0) {
                return -1;
        }

        P->p2p_cmd_sock = cmd_sock;
	printf("%s: ===> %d\n", __FUNCTION__, P->p2p_cmd_sock);
        return 0;
}

int wpa_connect
    ( wpa_private_t* P
    , const char* if_name
    )
{
	char sock_file[100];
#ifdef ANDROID_AOSP
	const char *supplicat_file = WPA_SUPPLICANT_CTRL_IFACE_PATH;
#else
	const char *supplicat_file = "/var/run/wpa_supplicant";
#endif
	int sock_notif;
	int cmd_sock;
	int ret;

	if (P == NULL || if_name == NULL)
		return -1;

	ret = snprintf(sock_file, sizeof(sock_file), "%s/%s", supplicat_file, if_name);
	if (ret < 0 || ret > 100)
		return -1;

	sock_notif = wpa_open(sock_file, &P->notif_local);
	if (sock_notif < 0)
		return -1;

    	cmd_sock = wpa_open(sock_file, &P->cmd_local);
	if (cmd_sock < 0) {
    		wpa_close(sock_notif, &P->notif_local);
		return -1;
	}

	P->cmd_sock = cmd_sock;
	P->notif_sock = sock_notif;

	return 0;
}

int get_mac_address
    ( const char* if_name ///< [in] interface name to obtain the MAC address for
    , uint8_t* mac       ///< [out] buffer to store the binary MAC address in (6 bytes)
    )
{
    assert( if_name );
    assert( mac );

    struct ifreq ifr;
    int sock;
    int rc;

	DPRINT_INFO(WFA_OUT,"entering %s \n", __func__ );
    sock = socket( PF_INET, SOCK_STREAM, 0 );
    if ( sock < 0 ) {
         DPRINT_ERR(WFA_ERR,"%d: %s",  __LINE__, strerror(errno) );
        return 1;
    }

    snprintf( ifr.ifr_name, IFNAMSIZ, "%s", if_name );

    rc = ioctl( sock, SIOCGIFHWADDR, &ifr );
    if ( rc == -1 ) {
         DPRINT_ERR(WFA_ERR,"%d: %s", __LINE__, strerror(errno) );
        close( sock );
        return 1;
    }

    memcpy( mac, &ifr.ifr_hwaddr.sa_data, ETH_ALEN );
    close( sock );

    return 0;
}

int wpa_channel_to_freq
    ( int channel
    )
{
    int ret_ch = 0;
    DPRINT_INFO(WFA_OUT, "entering %s\n", __func__ );
    if (channel <=14)
    {
    	ret_ch = (2407 + (5*channel));
    }
    else if ((channel >=36) && (channel <=173))
    {
	ret_ch = (5000 + (5*channel));
    }
    else
    {
         DPRINT_ERR(WFA_ERR,"illegal channel=%d\n", channel);
    }

    return ret_ch;    
}

//YF
int wpa_get_if_by_role
    ( wpa_private_t * P
    , uint8_t* macbuf
    )
{
	if ((P->role == GO) || (P->role == GC))
      	memcpy(macbuf, P->p2p_if_name,IFNAMSIZ );
    else
	 memcpy(macbuf, P->if_name,IFNAMSIZ );

	return 0;
}

int wpa_get_mac
    ( wpa_private_t * P
    , uint8_t* macbuf
    )
{
    DPRINT_INFO(WFA_OUT,"entering %s ==> [%d][%s][%s]\n",
		 __func__, P->role, P->if_name, P->p2p_if_name);
	
    if ((P->role == GO) || (P->role == GC))
      return get_mac_address( P->p2p_if_name, macbuf );
    else		
    return get_mac_address( P->if_name, macbuf );
}

int wpa_set_debug_level
    ( wpa_private_t * h
    , const int level
    )
{
    return 0;
}
