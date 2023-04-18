// Based on the Paho C code example from www.eclipse.org/paho/
#include <iostream>
#include <sstream>
#include <fstream>
#include <string.h>
#include <chrono>
#include <thread>
#include <ctime>
#include <sstream>
#include "MQTTClient.h"
#include "ADXL345.h"
#define  CPU_LOAD "/proc/loadavg"
using namespace std;
using namespace exploringRPi;

//Please replace the following address with the address of your server
#define ADDRESS    "tcp://192.168.1.28:1883"
#define CLIENTID   "bbb_publish"
#define AUTHMETHOD "amin"
#define AUTHTOKEN  "password"
#define TOPIC      "ee513/Sensor"
#define QOS        2
#define TIMEOUT    10000L

float getCPULoad() {        		// get the CPU load
   string line;
   fstream fs;
   fs.open(CPU_LOAD, fstream::in); 	// read from the file
   fs >> line;
   fs.close();
   return stof(line.substr(0, line.find(' ')));
}

string getTime() {
   time_t rawtime;
   struct tm * timeinfo;
   char buffer[80];

   time (&rawtime);
   timeinfo = localtime(&rawtime);

   strftime(buffer,sizeof(buffer),"%H:%M:%S",timeinfo);
   return string{buffer};
}

int main(int argc, char* argv[]) {
   char str_payload[256];          // Set your max message size here
   MQTTClient client;
   MQTTClient_connectOptions opts = MQTTClient_connectOptions_initializer;
   MQTTClient_message pubmsg = MQTTClient_message_initializer;
   MQTTClient_deliveryToken token;
   MQTTClient_create(&client, ADDRESS, CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL);
   MQTTClient_willOptions will_opts = MQTTClient_willOptions_initializer;
   will_opts.topicName = TOPIC;
   will_opts.message = "Good luck!"; 
   opts.will = &will_opts; 
   opts.keepAliveInterval = 20;
   opts.cleansession = 1;
   opts.username = AUTHMETHOD;
   opts.password = AUTHTOKEN;
   int rc;
   if ((rc = MQTTClient_connect(client, &opts)) != MQTTCLIENT_SUCCESS) {
      cout << "Failed to connect, return code " << rc << endl;
      return -1;
   }

   ADXL345 sensor(2,0x53);
   sensor.setResolution(ADXL345::NORMAL);
   sensor.setRange(ADXL345::PLUSMINUS_4_G);
   sensor.readSensorState();

   auto time = getTime();
   sprintf(str_payload, "{ \"data\": { \"Time\": \"%s\", \"CPULoad\": %f, \"Pitch\": %f, \"Roll\": %f } }",
		  time.c_str() , getCPULoad(), sensor.getPitch(), sensor.getRoll());
   pubmsg.payload = str_payload;
   pubmsg.payloadlen = strlen(str_payload);
   pubmsg.qos = QOS;
   pubmsg.retained = 0;
   MQTTClient_publishMessage(client, TOPIC, &pubmsg, &token);
   cout << "Waiting for up to " << (int)(TIMEOUT/1000) <<
        " seconds for publication of " << str_payload <<
        " \non topic " << TOPIC << " for ClientID: " << CLIENTID << endl;
   rc = MQTTClient_waitForCompletion(client, token, TIMEOUT);
   cout << "Message with token " << (int)token << " delivered." << endl;
   
   MQTTClient_disconnect(client, 10000);
   MQTTClient_destroy(&client);
   return rc;
}
