/****************************************************************************************************
 *  BTS SNIR2 2024 - Author: Flamiingooo                                                           *
 *  Context:                                                                                       *
 *  This code is an MQTT client that subscribes to messages sent by the LORA gateway.              *
 *  The messages are received in JSON format, decoded from base64, and then the data is            *
 *  updated in a MySQL database.                                                                   *
 ***************************************************************************************************/

#include <stdio.h>
#include <mosquitto.h>                     // Mosquitto library to use the MQTT protocol.
#include <iostream>
#include <nlohmann/json.hpp>               // JSON library to handle JSON objects.
#include <cppcodec/base64_rfc4648.hpp>     // cppcodec library for base64 encoding and decoding.
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/statement.h>
#include <cppconn/resultset.h>
#include <cstring>

using namespace std;
using json = nlohmann::json;
using base64 = cppcodec::base64_rfc4648;

// Global variables to store messages and decoded data
char gateway_message[1024];
string bike_message, bikeID, battery, latitude, longitude, hemisphere;

// Function prototypes declaration
void splitFrame(struct mosquitto *mosq, void *obj, const struct mosquitto_message *message);
void updateDatabase();

// Function called whenever a message is received with the topic /data_bike
void my_message_callback(struct mosquitto *mosq, void *userdata, const struct mosquitto_message *message)
{
    if (message->payloadlen)
    {
        // Initialize buffer and copy the received message
        memset(gateway_message, 0, sizeof(gateway_message));
        memcpy(gateway_message, message->payload, message->payloadlen);

        // Parse the JSON message
        json gateway_message_json = json::parse(gateway_message);
        string data_b64{gateway_message_json["data"]};

        // Decode base64 data
        vector<uint8_t> data_decoded = base64::decode(data_b64);
        bike_message.assign(data_decoded.begin(), data_decoded.end());
        
        // Extract different parts of the message
        bikeID = bike_message.substr(1, 4);
        battery = bike_message.substr(6, 3);
        latitude = bike_message.substr(10, 7);
        longitude = bike_message.substr(18, 6);
        hemisphere = bike_message.substr(25, 1);

        try
        {
            // Connect to the MySQL database
            sql::Driver *driver;
            sql::Connection *con;
            sql::Statement *stmt;

            driver = get_driver_instance();
            con = driver->connect("serverName:3306", "AdminUsername", "password");
            con->setSchema("greenway");
            stmt = con->createStatement();

            // Update data in the database
            stmt->execute("UPDATE `Bikes` SET `Battery`='" + battery + "', `Latitude`='" + latitude + "', `Longitude`='" + longitude + "', `Hemisphere`='" + hemisphere + "' WHERE `BikeID`='" + bikeID + "'");
            cout << "Data updated successfully." << endl;
            delete stmt;
            delete con;
        }
        catch (sql::SQLException &e)
        {   // Handle SQL exceptions
            cout << "# ERR: " << e.what();
            cout << " (MySQL error code: " << e.getErrorCode();
            cout << ", SQLState: " << e.getSQLState() << " )" << endl;
        }
        cout << endl;
    }
    else
    {
        // Display a message if the payload is empty
        printf("%s (null)\n", message->topic);
    }
}

// Function called when connecting to the MQTT broker
void my_connect_callback(struct mosquitto *mosq, void *userdata, int result)
{
    int i;
    if (!result)
    {
        // Subscribe to the topic if connection to the MQTT broker is successful
        mosquitto_subscribe(mosq, NULL, "/data_bike", 2);
    }
    else
    {
        fprintf(stderr, "Connect failed\n");
    }
}

// Function called when subscribing to a topic
void my_subscribe_callback(struct mosquitto *mosq, void *userdata, int mid, int qos_count, const int *granted_qos)
{
    int i;

    printf("Subscribed (mid: %d): %d", mid, granted_qos[0]);
    for (i = 1; i < qos_count; i++)
    {
        printf(", %d", granted_qos[i]);
    }
    printf("\n");
}

// Log function for MQTT messages
void my_log_callback(struct mosquitto *mosq, void *userdata, int level, const char *str)
{
    printf("%s\n", str);
}

int main(void)
{
    int i;
    const char *host = "localhost";
    int port = 1883;
    int keepalive = 60;
    bool clean_session = true;
    struct mosquitto *mosq = NULL;

    // Initialize the Mosquitto library
    mosquitto_lib_init();
    mosq = mosquitto_new(NULL, clean_session, NULL);
    if (!mosq)
    {
        fprintf(stderr, "Error: Out of memory\n");
        return 1;
    }

    // Set callbacks for Mosquitto
    mosquitto_log_callback_set(mosq, my_log_callback);
    mosquitto_connect_callback_set(mosq, my_connect_callback);
    mosquitto_message_callback_set(mosq, my_message_callback);
    mosquitto_subscribe_callback_set(mosq, my_subscribe_callback);

    // Connect to the MQTT broker
    if (mosquitto_connect(mosq, host, port, keepalive))
    {
        fprintf(stderr, "Unable to connect.\n");
        return 1;
    }

    // Main Mosquitto loop
    mosquitto_loop_forever(mosq, -1, 1);

    // Clean up the Mosquitto library
    mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();
    return 0;
}
