/*
  Copyright (c) 2016, Marko Viitanen (Fador)

  Permission to use, copy, modify, and/or distribute this software for any purpose 
  with or without fee is hereby granted, provided that the above copyright notice 
  and this permission notice appear in all copies.

  THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH 
  REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY 
  AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, 
  INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM 
  LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE 
  OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR 
  PERFORMANCE OF THIS SOFTWARE.

*/

#include <c_types.h>
#include <user_interface.h>
#include <espconn.h>
#include <mem.h>
#include <osapi.h>
#include "connection.h"
#include "configure.h"


LOCAL ip_addr_t ip;
LOCAL struct espconn conn1;
LOCAL esp_udp udp1;

LOCAL struct espconn conn2;
LOCAL esp_tcp tcp1;


static void ICACHE_FLASH_ATTR recvCb(void *arg, char *data, unsigned short len) {
  struct espconn *conn=(struct espconn *)arg;
  int x;
  remot_info *premot = NULL;
  
  if(len >= BUFFERLEN) return;
  
  struct ip_info ipconfig;
  char buffer[BUFFERLEN];
  

  wifi_get_ip_info(STATION_IF, &ipconfig);
   
   if(os_strstr(data,"M-SEARCH")) {
     if (espconn_get_connection_info(&conn1, &premot, 0) != ESPCONN_OK)
          return;
      os_memcpy(conn1.proto.udp->remote_ip, premot->remote_ip, 4);
      conn1.proto.udp->remote_port = premot->remote_port;
        
      for (x=0; x<len; x++) os_printf("%c", data[x]);
      os_sprintf(buffer,  "HTTP/1.1 200 OK\r\n"
                          "CACHE-CONTROL: max-age=86400\r\n"
                          "DATE: Wed, 31 Aug 2016 00:13:46 GMT\r\n"
                          "EXT:\r\n"
                          "LOCATION: http://%d.%d.%d.%d:8000/setup.xml\r\n"
                          "OPT: \"http://schemas.upnp.org/upnp/1/0/\"; ns=01\r\n"
                          "01-NLS: %s\r\n"
                          "SERVER: Unspecified, UPnP/1.0, Unspecified\r\n"
                          "ST: urn:Belkin:device:**\r\n"
                          "USN: uuid:Socket-1_0-%s\r\n\r\n", IP2STR(&ipconfig.ip), UUID, SERIAL_NUMBER);
      
      espconn_send(&conn1, buffer, strlen(buffer));
   }
  
}

LOCAL void ICACHE_FLASH_ATTR webserver_recv(void *arg, char *data, unsigned short length)
{
    struct espconn *ptrespconn = arg;
    char buffer[BUFFERLEN];
    if(os_strstr(data,"setup.xml")) {
      
      os_sprintf(buffer, "HTTP/1.1 200 OK\r\n"
                         "Content-Type: text/xml\r\n"
                         "SERVER: Unspecified, UPnP/1.0, Unspecified\r\n"
                         "connection: close\r\n"
                         "LAST-MODIFIED: Sat, 01 Jan 2000 00:00:00 GMT\r\n"
                         "\r\n"
                        "<?xml version=\"1.0\"?>\r\n"
                        "<root>\r\n"
                        "  <device>\r\n"
                        "    <deviceType>urn:FadorTest:device:controllee:1</deviceType>\r\n"
                        "    <friendlyName>%s</friendlyName>\r\n"
                        "    <manufacturer>Belkin International Inc.</manufacturer>\r\n"
                        "    <modelName>Emulated Socket</modelName>\r\n"
                        "    <modelNumber>3.1415</modelNumber>\r\n"
                        "    <UDN>uuid:Socket-1_0-%s</UDN>\r\n"
                        "  </device>\r\n"
                        "</root>", 
                        DEVICE_NAME, SERIAL_NUMBER);
      espconn_sent(ptrespconn, buffer, os_strlen(buffer));
      return;
    } else if(os_strstr(data, "/upnp/control/basicevent1")) {
      
      char message[100];
      if(os_strstr(data, "<BinaryState>1</BinaryState>")) {
        os_printf(message, "Warp drive turn on");
      } else {
        os_printf(message, "Warp drive turn off");
      }
      
      
      os_sprintf(buffer, "HTTP/1.1 200 OK\r\n"
                         "CONTENT-TYPE: text/xml charset=\"utf-8\"\r\n"
                         "SERVER: Unspecified, UPnP/1.0, Unspecified\r\n"
                         "connection: close\r\n"
                         "LAST-MODIFIED: Sat, 01 Jan 2000 00:00:00 GMT\r\n"
                         "\r\n"
                         "%s", message);                        
      espconn_sent(ptrespconn, buffer, os_strlen(buffer));
      
    } else {
      os_sprintf(buffer, "HTTP/1.1 200 OK\r\n"
                         "Content-Type: text/xml\r\n"
                         "SERVER: Unspecified, UPnP/1.0, Unspecified\r\n"
                         "connection: close\r\n"
                         "LAST-MODIFIED: Sat, 01 Jan 2000 00:00:00 GMT\r\n"
                         "\r\n"
                         "ok");                        
      espconn_sent(ptrespconn, buffer, os_strlen(buffer));
    }
    



    
}

LOCAL ICACHE_FLASH_ATTR void webserver_recon(void *arg, sint8 err)
{
    struct espconn *pesp_conn = arg;

}

/******************************************************************************
 * FunctionName : webserver_recon
 * Description  : the connection has been err, reconnection
 * Parameters   : arg -- Additional argument to pass to the callback function
 * Returns      : none
*******************************************************************************/
LOCAL ICACHE_FLASH_ATTR void webserver_discon(void *arg)
{
    struct espconn *pesp_conn = arg;

}

LOCAL void ICACHE_FLASH_ATTR webserver_listen(void *arg)
{
    struct espconn *pesp_conn = arg;

    espconn_regist_recvcb(pesp_conn, webserver_recv);
    espconn_regist_reconcb(pesp_conn, webserver_recon);
    espconn_regist_disconcb(pesp_conn, webserver_discon);
}

void ICACHE_FLASH_ATTR serverInit() {
  conn1.type = ESPCONN_UDP;
  conn1.state = ESPCONN_NONE;
  conn1.proto.udp = (esp_udp *)os_zalloc(sizeof(esp_udp));
  udp1.local_port = 1900;
  conn1.proto.udp = &udp1;
  espconn_create(&conn1);
  espconn_regist_recvcb(&conn1, recvCb);
  
  ip_addr_t ipgroup;
  int ret;
  
  ipaddr_aton("239.255.255.250", &ipgroup);
  
  espconn_igmp_join( (ip_addr_t *) &ip, &ipgroup);

  os_printf("Free heap: %d\n", system_get_free_heap_size());
  
  
  tcp1.local_port = 8000;
  conn2.type = ESPCONN_TCP;
  conn2.state = ESPCONN_NONE;
  conn2.proto.tcp = &tcp1;
  
  
  espconn_regist_connectcb(&conn2, webserver_listen);
  espconn_accept(&conn2);
}
