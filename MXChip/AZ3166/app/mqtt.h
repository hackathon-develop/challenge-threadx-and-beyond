#include <time.h>

#include "tx_api.h"
#include "nx_api.h"
#include "nxd_mqtt_client.h"
#include "tx_port.h"

/* IP Address of the server. */
#define  MQTT_SERVER_ADDRESS (IP_ADDRESS(5, 196, 78, 28)) //test.mosquitto.org

/*******************************************************/
/* IOT MQTT Client Example                             */
/*******************************************************/
#define DEMO_STACK_SIZE         2048
#define CLIENT_ID_STRING        "xxx_mqtt_client"
#define MQTT_CLIENT_STACK_SIZE  4096
#define STRLEN(p)               (sizeof(p) - 1)

/* Declare the MQTT thread stack space. */
static ULONG                    mqtt_client_stack[MQTT_CLIENT_STACK_SIZE / sizeof(ULONG)];

/* Declare the MQTT client control block. */
static NXD_MQTT_CLIENT          mqtt_client;

/* Define the test threads.  */
#define TOPIC_NAME                  "xxx_sdv24"
#define MESSAGE_STRING              "Hello, xxx!"

/* Define the priority of the MQTT internal thread. */
#define MQTT_THREAD_PRIORITY         2

/* Define the MQTT keep alive timer for 5 minutes */
#define MQTT_KEEP_ALIVE_TIMER       300

#define QOS0                        0
#define QOS1                        1

/* Declare event flag, which is used in this demo. */
TX_EVENT_FLAGS_GROUP                mqtt_app_flag;
#define DEMO_MESSAGE_EVENT          1
#define DEMO_ALL_EVENTS             3

/* Declare buffers to hold message and topic. */
static UCHAR message_buffer[NXD_MQTT_MAX_MESSAGE_LENGTH];
static UCHAR topic_buffer[NXD_MQTT_MAX_TOPIC_NAME_LENGTH];

/* Declare the disconnect notify function. */
static VOID my_disconnect_func(NXD_MQTT_CLIENT *client_ptr)
{
    NX_PARAMETER_NOT_USED(client_ptr);
    printf("client disconnected from server\n");
}


static VOID my_notify_func(NXD_MQTT_CLIENT* client_ptr, UINT number_of_messages)
{
    NX_PARAMETER_NOT_USED(client_ptr);
    NX_PARAMETER_NOT_USED(number_of_messages);
    tx_event_flags_set(&mqtt_app_flag, DEMO_MESSAGE_EVENT, TX_OR);
    return; 
}

static ULONG error_counter;
UINT mqtt_client_connect_subscribe(NX_IP *ip_ptr, NX_PACKET_POOL *pool_ptr)
{
    UINT status;
    NXD_ADDRESS server_ip;

    /* Create MQTT client instance. */
    status = nxd_mqtt_client_create(&mqtt_client, "xxx_client",
        CLIENT_ID_STRING, STRLEN(CLIENT_ID_STRING), ip_ptr, pool_ptr,
        (VOID*)mqtt_client_stack, sizeof(mqtt_client_stack),
        MQTT_THREAD_PRIORITY, NX_NULL, 0);

    if (status)
    {
        printf("Error in creating MQTT client: 0x%02x\n", status);
        error_counter++;
    }

    /* Register the disconnect notification function. */
    nxd_mqtt_client_disconnect_notify_set(&mqtt_client, my_disconnect_func);

    /* Create an event flag for this demo. */
    status = tx_event_flags_create(&mqtt_app_flag, "xxx app event");
    if(status)
        error_counter++;

    server_ip.nxd_ip_version = 4;
    server_ip.nxd_ip_address.v4 = MQTT_SERVER_ADDRESS;

    /* Start the connection to the server. */
    nxd_mqtt_client_connect(&mqtt_client, &server_ip, NXD_MQTT_PORT, 
        MQTT_KEEP_ALIVE_TIMER, 0, NX_WAIT_FOREVER);

    /* Subscribe to the topic with QoS level 0. */
    nxd_mqtt_client_subscribe(&mqtt_client, TOPIC_NAME, STRLEN(TOPIC_NAME),
        QOS0);

    /* Set the receive notify function. */
    nxd_mqtt_client_receive_notify_set(&mqtt_client, my_notify_func);

    return status;
}

void mqtt_client_publish(char* message_string)
{
    UINT status;
    ULONG events;
    UINT topic_length, message_length;

    printf("message to send= %s\r\n", message_string);

    /* Publish a message with QoS Level 1. */
    nxd_mqtt_client_publish(&mqtt_client, TOPIC_NAME,
        STRLEN(TOPIC_NAME), (CHAR*)message_string, 
        30, 0, QOS1, NX_WAIT_FOREVER);

    /* Now wait for the broker to publish the message. */
    tx_event_flags_get(&mqtt_app_flag, DEMO_ALL_EVENTS,
        TX_OR_CLEAR, &events, TX_WAIT_FOREVER);


    if(events & DEMO_MESSAGE_EVENT)
    {
        /* Retrieve the published message : Used for testing purpose */
        status = nxd_mqtt_client_message_get(&mqtt_client, topic_buffer,
            sizeof(topic_buffer), &topic_length, message_buffer,
            sizeof(message_buffer), &message_length);

        if(status == NXD_MQTT_SUCCESS)
        {
            topic_buffer[topic_length] = 0;
            message_buffer[message_length] = 0;
            printf("topic = %s, message = %s\r\n", topic_buffer, message_buffer);
        }
    }
}

void mqtt_client_disconnect()
{
    /* Now unsubscribe the topic. */
    nxd_mqtt_client_unsubscribe(&mqtt_client, TOPIC_NAME,
        STRLEN(TOPIC_NAME));

    /* Disconnect from the broker. */
    nxd_mqtt_client_disconnect(&mqtt_client);

    /* Delete the client instance, release all the resources. */
    nxd_mqtt_client_delete(&mqtt_client);
}
