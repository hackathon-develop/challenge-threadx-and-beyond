#include <stdio.h>
#include <tx_api.h>
#include <nx_api.h>
#include <tx_byte_pool.h>

#include "sensor.h"
#include "ssd1306.h"
#include "stm32f412rx.h"
#include "string.h"
#include "tx_port.h"
#include "wwd_networking.h"
#include "mqtt.h"
#define ECLIPSETX_THREAD_PRIORITY   4

static volatile lsm6dsl_data_t xxx_imu_data;
static volatile UCHAR xxx_imu_data_ready = 0;

//
// Network Thread
//
#define NETWORK_THREAD_STACK_SIZE 4096
TX_THREAD xxx_network_thread;
ULONG xxx_network_thread_stack[NETWORK_THREAD_STACK_SIZE / sizeof(ULONG)];
static NXD_MQTT_CLIENT          mqtt_client;

static volatile UCHAR xxx_network_ready = 0;
// Entry function to setup the network
static void xxx_network_thread_entry(ULONG parameter)
{
    xxx_network_ready = 0;
    UINT status;

    // buffer for string formatting
    CHAR buffer[64] = "";
    size_t buffer_len = sizeof(buffer);

    printf("INFO: Initializing the network");
    
    // Initialize the network
    if ((status = wwd_network_init(WIFI_SSID, WIFI_PASSWORD, WIFI_MODE)))
    {
        printf("ERROR: Failed to initialize the network (0x%08x)\r\n", status);
        return;
    }

    if ((status = wwd_network_connect()))
    {
        printf("ERROR: failled to connect to connect to network");
        return;
    }
    printf("INFO: Network initialization complete\n");
    xxx_network_ready = 1;

    // TODO: set up hacky pico zenoh send config here?

    if ((status = mqtt_client_connect_subscribe(&nx_ip, nx_pool)))
    {
        printf("ERROR: failed to connect to an mqtt broker");
        return;
    }
    printf("INFO: Connected to MQTT broker\n");

    while (1){
        if (xxx_imu_data_ready){
            snprintf(buffer, buffer_len, "a:%03d, b:%03d ,c:%03d",
                (int16_t) (xxx_imu_data.angular_rate_mdps[0]/(float) 100.0), 
                (int16_t) (xxx_imu_data.angular_rate_mdps[1]/(float) 100.0),
                (int16_t) (xxx_imu_data.angular_rate_mdps[2]/(float) 100.0));

            mqtt_client_publish(buffer);
            // TODO: trigger zenoh send here?
        }

        tx_thread_sleep(TX_TIMER_TICKS_PER_SECOND*1);
    }

    mqtt_client_disconnect();
}


//
// Sensor Thread
//
#define SENSOR_THREAD_STACK_SIZE 4096
TX_THREAD xxx_sensor_thread;
ULONG xxx_sensor_thread_stack[SENSOR_THREAD_STACK_SIZE / sizeof(ULONG)];

// Entry function for the sensor thread
static void xxx_sensor_thread_entry(ULONG parameter)
{
    xxx_imu_data_ready = 0;
    UINT status;

    printf("INFO: Initializing the IMU");
    if ((status = lsm6dsl_config()))
    {
        printf("ERROR: failled to configure IMU");
        return;
    }

    while (1){
        xxx_imu_data = lsm6dsl_data_read();
        xxx_imu_data_ready = 1;
        tx_thread_sleep(TX_TIMER_TICKS_PER_SECOND/5);
    }
}


//
// Display Thread
//
#define DISPLAY_THREAD_STACK_SIZE 4096
TX_THREAD xxx_display_thread;
ULONG xxx_display_thread_stack[DISPLAY_THREAD_STACK_SIZE / sizeof(ULONG)];

#define DISPLAY_ORANGE_SECTION_Y_MIN 0
#define DISPLAY_ORANGE_SECTION_Y_MAX 14

#define DISPLAY_BLUE_SECTION_Y_MIN 16
#define DISPLAY_BLUE_SECTION_Y_MAX 63

#define DISPLAY_Y_MIN DISPLAY_ORANGE_SECTION_Y_MIN
#define DISPLAY_Y_MAX DISPLAY_BLUE_SECTION_Y_MAX

#define DISPLAY_X_MIN 0
#define DISPLAY_X_MAX 127

extern NX_PACKET      *packet;
extern NX_UDP_SOCKET  socket;
extern TX_THREAD      thread;
extern NX_PACKET_POOL pool;
extern NX_IP          ip;
// Entry function for the display thread
//
// The display itself is 128 by 64 pixels.
// The pixels are thus addressable are
//   0 <= x < 128
//   0 <= y < 64
// These pixels are orange: y: 0 <= y < 15
// These pixels are blue:   y: 16 <= y < 64
// This means that for y = 15, there are no pixels!
static void xxx_display_thread_entry(ULONG parameter)
{
    ssd1306_SetDisplayOn(1);

    ssd1306_Fill(Black);

    ssd1306_DrawRectangle(DISPLAY_X_MIN,
                          DISPLAY_ORANGE_SECTION_Y_MIN,
                          DISPLAY_X_MAX,
                          DISPLAY_ORANGE_SECTION_Y_MAX,
                          White);

    ssd1306_DrawRectangle(DISPLAY_X_MIN,
                          DISPLAY_BLUE_SECTION_Y_MIN,
                          DISPLAY_X_MAX,
                          DISPLAY_BLUE_SECTION_Y_MAX,
                          White);
    ssd1306_UpdateScreen();

    // buffer for string formatting
    CHAR buffer[64] = "";
    size_t buffer_len = sizeof(buffer);

    while(1) {
        { // write IP address
            ULONG ip_address;
            ULONG network_mask;
            if (xxx_network_ready){
                nx_ip_address_get(&nx_ip, &ip_address, &network_mask);
                snprintf(buffer, buffer_len , "IP=%3d.%3d.%3d.%3d", 
                        (uint8_t) (ip_address>>24),
                        (uint8_t) ((ip_address>>16) & 0xff),
                        (uint8_t) ((ip_address >>8) & 0xff),
                        (uint8_t) (ip_address & 0xff));
            } else {
                strlcpy(buffer,"IP=xxx.xxx.xxx.xxx", buffer_len);
            }

            ssd1306_DrawRectangle(
                                  DISPLAY_X_MIN + 1,
                                  DISPLAY_ORANGE_SECTION_Y_MIN + 1,
                                  DISPLAY_X_MAX - 1,
                                  DISPLAY_ORANGE_SECTION_Y_MAX - 1,
                                  Black);
            ssd1306_SetCursor(DISPLAY_X_MIN +1, DISPLAY_ORANGE_SECTION_Y_MIN + 1);
            ssd1306_WriteString(buffer, Font_7x10, White);
        }


        { // write IMU acceleration
            if (xxx_imu_data_ready){
                snprintf(buffer, buffer_len, "x=%3d y=%3d z=%3d",
                    (int16_t) (xxx_imu_data.acceleration_mg[0]/(float) 20.0), 
                    (int16_t) (xxx_imu_data.acceleration_mg[1]/(float) 20.0),
                    (int16_t) (xxx_imu_data.acceleration_mg[2]/(float) 20.0));
            }else{
                strlcpy(buffer,"no IMU", buffer_len );
            }

            ssd1306_DrawRectangle(
                                  DISPLAY_X_MIN +1,
                                  DISPLAY_BLUE_SECTION_Y_MIN + 1,
                                  DISPLAY_X_MAX -1,
                                  DISPLAY_Y_MAX -1,
                                  Black);

            ssd1306_SetCursor(1, DISPLAY_BLUE_SECTION_Y_MIN + 1);
            ssd1306_WriteString(buffer, Font_7x10, White);
        }

        
        { // write IMU gyro
            if (xxx_imu_data_ready){
                snprintf(buffer, buffer_len, "a=%3d b=%3d c=%3d",
                    (int16_t) (xxx_imu_data.angular_rate_mdps[0]/(float) 100.0), 
                    (int16_t) (xxx_imu_data.angular_rate_mdps[1]/(float) 100.0),
                    (int16_t) (xxx_imu_data.angular_rate_mdps[2]/(float) 100.0));
            }else{
                strlcpy(buffer,"no IMU", buffer_len );
            }

            ssd1306_DrawRectangle(
                                  DISPLAY_X_MIN +1,
                                  DISPLAY_BLUE_SECTION_Y_MIN + 1 + 11 ,
                                  DISPLAY_X_MAX -1,
                                  DISPLAY_Y_MAX -1,
                                  Black);

            ssd1306_SetCursor(1, DISPLAY_BLUE_SECTION_Y_MIN + 1 + 11);
            ssd1306_WriteString(buffer, Font_7x10, White);
        }


        ssd1306_UpdateScreen();
        
        tx_thread_sleep(TX_TIMER_TICKS_PER_SECOND/3);
    }
}

//
// Zenohpico Thread
//
#define ZENOHPICO_THREAD_STACK_SIZE 4096
TX_THREAD xxx_zenohpico_thread;
ULONG xxx_zenohpico_thread_stack[ZENOHPICO_THREAD_STACK_SIZE / sizeof(ULONG)];

// Entry function for the zenohpico thread
static void xxx_zenohpico_thread_entry(ULONG parameter)
{
    xxx_imu_data_ready = 0;
    UINT status;

    printf("INFO: ZENOHPICO");
    if ((status = 0))
    {
        printf("ERROR: failled setup ZENOHPICO");
        return;
    }

    while (1){
        // TODO: trigger ZENOHPICO send ?
        printf("%lx ", (unsigned long) packet);
        printf("%lx ", (unsigned long) &socket);
        printf("%lx ", (unsigned long) &thread);
        printf("%lx ", (unsigned long) &pool);
        printf("%lx \n", (unsigned long) &ip);
        tx_thread_sleep(TX_TIMER_TICKS_PER_SECOND/100);
    }
}
