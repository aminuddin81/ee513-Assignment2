#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include <thread>
#include <chrono>
#include <iostream>
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <fcntl.h>
#include "MQTTClient.h"

#define ADDRESS     "tcp://192.168.1.28:1883"
#define CLIENTID    "bbb_sub2"
#define AUTHMETHOD  "amin"
#define AUTHTOKEN   "password"
#define TOPIC       "ee513/Sensor"
#define PAYLOAD     "Hello World 2023!"
#define QOS         0
#define TIMEOUT     10000L

using namespace std;
using namespace std::chrono;
using namespace std::this_thread;

// This actuator application will flash the LED hooked up to GPIO1_28 (P9_12)

volatile MQTTClient_deliveryToken deliveredtoken;

void flashLED()
{
    int i = 0, f = 0;

    // set direction out
    f = open("/sys/class/gpio/gpio60/direction", O_RDWR);
    write(f, "out", 3);
    close(f);

    f = open("/sys/class/gpio/gpio60/value", O_WRONLY);

    // Blink the LED 5 times
    for(i = 1; i <=5; i++)
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

void getCurTime(char* curTime){
	time_t timeonBBB = time(NULL);
	struct tm tm = *localtime(&timeonBBB);
	sprintf(curTime, "%02d:%02d:%02d", tm.tm_hour, tm.tm_min, tm.tm_sec);
}

int getCPUFrequency() {
  int freq;
  ifstream freq_file("/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq");

  if (!freq_file.is_open()) {
    std::cerr << "Failed to open CPU frequency file" << std::endl;
    return -1;
  }

  freq_file >> freq;

  freq_file.close();
  return freq / 1000;
}

float determineRoll(std::string json){
    std::string pitchDelim {"\"Roll\":"};
    std::string endDelim {"}"};
    auto first_pos = json.find(pitchDelim);
    auto end_pos = first_pos + pitchDelim.length();
    auto last_pos = json.find_first_of(endDelim, end_pos);
    return std::stof(json.substr(end_pos, last_pos - end_pos));

}

void delivered(void *context, MQTTClient_deliveryToken dt) {
    printf("Message with token value %d delivery confirmed\n", dt);
    deliveredtoken = dt;
}

int msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message) {
    char curTime[9];
	int i;
    char* payloadptr;
    printf("Message arrived\n");
    printf("     topic: %s\n", topicName);
    printf("   message: ");
    payloadptr = (char*) message->payload;
    std::string json {payloadptr, (size_t)message->payloadlen};
    std::cout << json << std::endl;
    if(std::abs(determineRoll(json)) > 10){
	std::cout << "Take it handy" << std::endl;
		flashLED();		
		//std::cout << "Current CPU frequency: " << getCPUFrequency() << " MHz" << std::endl;
    }

    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    return 1;
}

void connlost(void *context, char *cause) {
    printf("\nConnection lost\n");
    printf("     cause: %s\n", cause);
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
