#include <stdio.h>
#include <tx_api.h>

#include "sensor.h"
#include "ssd1306.h"
#include "stm32f412rx.h"
#include "string.h"
#include "tx_port.h"
#include "wwd_networking.h"


#define ECLIPSETX_THREAD_PRIORITY   4

//
// Network Thread
//
#define NETWORK_THREAD_STACK_SIZE 4096
TX_THREAD xxx_network_thread;
ULONG xxx_network_thread_stack[NETWORK_THREAD_STACK_SIZE / sizeof(ULONG)];

static volatile UCHAR xxx_network_ready = 0;
// Entry function to setup the network
static void xxx_network_thread_entry(ULONG parameter)
{
    xxx_network_ready = 0;
    UINT status;
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
    printf("INFO: Network initialization complete");
    xxx_network_ready = 1;
}


//
// Sensor Thread
//
#define SENSOR_THREAD_STACK_SIZE 4096
TX_THREAD xxx_sensor_thread;
ULONG xxx_sensor_thread_stack[SENSOR_THREAD_STACK_SIZE / sizeof(ULONG)];

static volatile lsm6dsl_data_t xxx_imu_data;
static volatile UCHAR xxx_imu_data_ready = 0;


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
