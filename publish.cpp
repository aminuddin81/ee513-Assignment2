// Based on the Paho C code example from www.eclipse.org/paho/
#include <iostream>
#include <sstream>
#include <fstream>
#include <string.h>
#include <unistd.h>
#include "MQTTClient.h"
#include "ADXL345.h"

using namespace std;

//Please replace the following address with the address of your server
#define ADDRESS "tcp://192.168.1.28:1883"
#define CLIENTID "bbb_publish"
#define AUTHMETHOD "amin"
#define AUTHTOKEN "password"
#define TOPIC "ee513/test"
#define QOS 1
#define TIMEOUT 10000L

double getCPULoad() {	// get the CPU load
  double load;	// store as double
  ifstream stat_file("/proc/stat");	 // read from the file

  if (!stat_file.is_open()) {
    cerr << "Failed to open /proc/stat" << endl;
    return -1;
  }

  string line;
  getline(stat_file, line);
  istringstream iss(line);

  string cpu_label;
  iss >> cpu_label;

  if (cpu_label != "cpu") {
    cerr << "Failed to read cpu label from /proc/stat" << endl;
    return -1;
  }

  int user, nice, system, idle, iowait, irq, softirq, steal;
  iss >> user >> nice >> system >> idle >> iowait >> irq >> softirq >> steal;

  int total_cpu_time = user + nice + system + idle + iowait + irq + softirq + steal;
  int idle_cpu_time = idle + iowait;
  load = (double)(total_cpu_time - idle_cpu_time) / total_cpu_time;

  stat_file.close();
  return load;
}

void getTime(char* curTime){
	time_t bbbtime = time(NULL);
	struct tm tm = *localtime(&bbbtime);
	sprintf(curTime, "%02d:%02d:%02d", tm.tm_hour, tm.tm_min, tm.tm_sec);
}

int main(int argc, char* argv[]) {
	char str_payload[100]; // Set your max message size here
	MQTTClient client;
	MQTTClient_connectOptions opts = MQTTClient_connectOptions_initializer;
	MQTTClient_willOptions Willopts = MQTTClient_willOptions_initializer;	
	MQTTClient_message pubmsg = MQTTClient_message_initializer;
	MQTTClient_deliveryToken token;
	MQTTClient_create(&client, ADDRESS, CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL);
	opts.keepAliveInterval = 20;
	opts.cleansession = 1;
	opts.username = AUTHMETHOD;
	opts.password = AUTHTOKEN;

	//setting last will and test message
   	opts.will = &Willopts;
   	opts.will->message = "BBB connection has been unexpectedly ended";
   	opts.will->qos = 1;
   	opts.will->retained = 0;
   	opts.will->topicName = TOPIC;

	int rc;
	if ((rc = MQTTClient_connect(client, &opts)) != MQTTCLIENT_SUCCESS) {
		cout << "Failed to connect, return code " << rc << endl;
	return -1;
	}

	//init ADXL345 and get data from it
   	//ADXL345 accel(2, 0x53);
   	//int x1, y1,z1 = 0;
   	//accel.readAllADXL345Data(x1,y1,z1);

	sprintf(str_payload, "{\"d\":{\"CPULoad\": %f }}", getCPULoad());
	//sprintf(str_payload + strlen(str_payload), "\"X\": %d,\n", x1);
   	//sprintf(str_payload + strlen(str_payload), "\"Y\": %d,\n", y1);
   	//sprintf(str_payload + strlen(str_payload), "\"Z\": %d,\n", z1);
   	sprintf(str_payload + strlen(str_payload), "}");
	
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