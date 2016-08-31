/*
Tnx to Sprite_TM (source came from his esp8266ircbot)
*/

#include <c_types.h>
#include <user_interface.h>
#include <espconn.h>
#include <mem.h>
#include <osapi.h>
#include "user_network.h"
#include "user_config.h"
#include "connection.h"

static char lineBuf[1024];
static int lineBufPos;
LOCAL os_timer_t network_timer;

static void ICACHE_FLASH_ATTR networkParseLine(struct espconn *conn, char *line) {
  char buff[1024];
    uint8 page, y;
    page = line[0];
    y = line[1];
    char* data = line + 2;    
    os_printf("P-L: %x-%x: %s\n\r",page,y,data);
    display_data(page, y, data);
}

static void ICACHE_FLASH_ATTR networkParseChar(struct espconn *conn, char c) {
  lineBuf[lineBufPos++]=c;
  if (lineBufPos>=sizeof(lineBuf)) lineBufPos--;

  if (lineBufPos>2 && lineBuf[lineBufPos-1]=='\n') {
    lineBuf[lineBufPos-1]=0;
    networkParseLine(conn, lineBuf);
    lineBufPos=0;
  }
}

static void ICACHE_FLASH_ATTR networkRecvCb(void *arg, char *data, unsigned short len) {
  struct espconn *conn=(struct espconn *)arg;
  int x;
  for (x=0; x<len; x++) networkParseChar(conn, data[x]);
}

static void ICACHE_FLASH_ATTR networkConnectedCb(void *arg) {
  struct espconn *conn=(struct espconn *)arg;
  espconn_regist_recvcb(conn, networkRecvCb);
  lineBufPos=0;
  os_printf("connected\n\r");
}

static void ICACHE_FLASH_ATTR networkReconCb(void *arg, sint8 err) {
  os_printf("Reconnect\n\r");
  network_init();
}

static void ICACHE_FLASH_ATTR networkDisconCb(void *arg) {
  os_printf("Disconnect\n\r");
  network_init();
}


void ICACHE_FLASH_ATTR network_start() {
  serverInit();
}

void ICACHE_FLASH_ATTR network_check_ip(void)
{
  struct ip_info ipconfig;

  os_timer_disarm(&network_timer);

  wifi_get_ip_info(STATION_IF, &ipconfig);


  if (wifi_station_get_connect_status() == STATION_GOT_IP && ipconfig.ip.addr != 0) {
    char page_buffer[20];
    os_sprintf(page_buffer,"IP: %d.%d.%d.%d",IP2STR(&ipconfig.ip));
    network_start();
  }
  else 
  {
    os_printf("No ip found\n\r");
    os_timer_setfn(&network_timer, (os_timer_func_t *)network_check_ip, NULL);
    os_timer_arm(&network_timer, 1000, 0);
  } 
        
}

void ICACHE_FLASH_ATTR network_init()
{
    os_printf("Network init\n");
    os_timer_disarm(&network_timer);
    os_timer_setfn(&network_timer, (os_timer_func_t *)network_check_ip, NULL);
    os_timer_arm(&network_timer, 1000, 0);    
}
