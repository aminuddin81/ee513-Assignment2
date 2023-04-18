#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "unistd.h"
#include "fcntl.h"
#include "MQTTClient.h"

#define ADDRESS "tcp://192.168.1.28:1883"
#define CLIENTID "bbb_sub1"
#define AUTHMETHOD "amin"
#define AUTHTOKEN "password"
#define TOPIC "ee513/test"
#define PAYLOAD "Hello World!"
#define QOS 1
#define TIMEOUT 10000L

volatile MQTTClient_deliveryToken deliveredtoken;

void delivered(void *context, MQTTClient_deliveryToken dt) {
	printf("Message with token value %d delivery confirmed\n", dt);
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
	int i;
	char* payloadptr;
	printf("Message arrived\n");
	printf(" topic: %s\n", topicName);
	printf(" message: ");
	payloadptr = (char*) message->payload;
	for(i=0; i<message->payloadlen; i++) {
	putchar(*payloadptr++);
}
putchar('\n');

blink_led();
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