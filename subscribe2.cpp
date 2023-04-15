#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "MQTTClient.h"
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <json-c/json.h>
#include <math.h>

#define ADDRESS "tcp://192.168.1.28:1883"
#define CLIENTID "bbb_sub2"
#define AUTHMETHOD "amin"
#define AUTHTOKEN "password"
#define TOPIC "ee513/CPULoad"
#define PAYLOAD "Hello World!"
#define QOS 1
#define TIMEOUT 10000L

volatile MQTTClient_deliveryToken deliveredtoken;

void delivered(void *context, MQTTClient_deliveryToken dt) {
	printf("Subscriber 1: Message with token value %d delivery confirmed\n", dt);
	deliveredtoken = dt;
}

void blink_led()
{
    int i = 0, f = 0;

    // set direction out
    f = open("/sys/class/gpio/gpio60/direction", O_RDWR);
    write(f, "out", 3);
    close(f);

    f = open("/sys/class/gpio/gpio60/value", O_WRONLY);

    // Blink the LED 2 times
    for(i = 1; i <=2; i++)
    {
        // LED On
        write(f, "1", 1);
        sleep(1);

        // LED off;
        write(f, "0", 1);
        sleep(1);
    }

    close(f);
}

int msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message) {
	
	struct json_object *parsed_json;
	struct json_object *parsedX;
	struct json_object *parsedY;
	struct json_object *parsedZ;

    float gravity_range = 4.0f;
    float resolution = 1024.0f;
    float factor = gravity_range / resolution * 1000;

	parsed_json = json_tokener_parse((char*)message->payload);
	json_object_object_get_ex(parsed_json, "X", &parsedX);
	json_object_object_get_ex(parsed_json, "Y", &parsedY);
	json_object_object_get_ex(parsed_json, "Z", &parsedZ);

    signed int X = json_object_get_int(parsedX);
	signed int Y = json_object_get_int(parsedY);
	signed int Z = json_object_get_int(parsedZ);

    signed int accelerationX = (signed int)(X * factor);
	signed int accelerationY = (signed int)(Y * factor);
	signed int accelerationZ = (signed int)(Z * factor);

	std::cout << "Output content of payload:" << std::endl;
	printf("The topic these messages were published to is: %s\n", topicName);
    printf("CPU Load: %d degrees\n", json_object_get_int(CPULoad));
    printf("Current Time: %s\n", json_object_get_string(bbbTime));
    printf("X Co-ord: %d\n", json_object_get_int(parsedX));
    printf("Y Co-ord: %d\n", json_object_get_int(parsedY));
    printf("Z Co-ord: %d\n", json_object_get_int(parsedZ));
	
    signed int pitch = 180 * atan (accelerationX/sqrt(accelerationY*accelerationY + accelerationZ*accelerationZ))/M_PI;
	signed int roll = 180 * atan (accelerationY/sqrt(accelerationX*accelerationX + accelerationZ*accelerationZ))/M_PI
	
    std::cout << "\nOutput content of payload:" << std::endl;
	printf("The topic these messages were published to is: %s\n", topicName);
    printf("X Co-ord: %d\n", json_object_get_int(parsedX));
    printf("Y Co-ord: %d\n", json_object_get_int(parsedY));
    printf("Z Co-ord: %d", json_object_get_int(parsedZ));
	std::cout << "\n" << std::endl;
    std::cout << "Perform Pitch and Roll calculations:" << std::endl;
    printf("Pitch is: %d\n", pitch);
    printf("Roll is: %d\n", roll);

        if(pitch > 30){
	    end::cout << "Pitch value exceeds 30 - LED blinking!" << srd::endl;
	    blink_led();
        }
        else{
            std::cout << "Pitch value less than 30 - LED not blinking" << std::endl;
        }

MQTTClient_freeMessage(&message);
MQTTClient_free(topicName);
return 1;
}

void connlost(void *context, char *cause) {
	printf("\nConnection lost\n");
	printf(" cause: %s\n", cause);
}

int main(int argc, char* argv[]) {
MQTTClient client;
MQTTClient_connectOptions opts = MQTTClient_connectOptions_initializer;
int rc;
int ch;

MQTTClient_create(&client, ADDRESS, CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL);
opts.keepAliveInterval = 20;
opts.cleansession = 1;
opts.username = AUTHMETHOD;
opts.password = AUTHTOKEN;

MQTTClient_setCallbacks(client, NULL, connlost, msgarrvd, delivered);
if ((rc = MQTTClient_connect(client, &opts)) != MQTTCLIENT_SUCCESS) {
	printf("Failed to connect, return code %d\n", rc);
	exit(-1);
}
printf("Subscribing to topic %s\nfor client %s using QoS%d\n\n"
	"Press Q<Enter> to quit\n\n", TOPIC, CLIENTID, QOS);
MQTTClient_subscribe(client, TOPIC, QOS);

do {
	ch = getchar();
	} while(ch!='Q' && ch != 'q');
	MQTTClient_disconnect(client, 10000);
	MQTTClient_destroy(&client);
	return rc;
}