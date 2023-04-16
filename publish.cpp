// Based on the Paho C code example from www.eclipse.org/paho/
#include <iostream>
#include <sstream>
#include <fstream>
#include <string.h>
#include "MQTTClient.h"

using namespace std;

//Please replace the following address with the address of your server
#define ADDRESS "tcp://192.168.1.28:1883"
#define CLIENTID "bbb1"
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

int main(int argc, char* argv[]) {
	char str_payload[100]; // Set your max message size here
	MQTTClient client;
	MQTTClient_connectOptions opts = MQTTClient_connectOptions_initializer;
	MQTTClient_message pubmsg = MQTTClient_message_initializer;
	MQTTClient_deliveryToken token;
	MQTTClient_create(&client, ADDRESS, CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL);
	opts.keepAliveInterval = 20;
	opts.cleansession = 1;
	opts.username = AUTHMETHOD;
	opts.password = AUTHTOKEN;

	int rc;
	if ((rc = MQTTClient_connect(client, &opts)) != MQTTCLIENT_SUCCESS) {
		cout << "Failed to connect, return code " << rc << endl;
	return -1;
	}

	sprintf(str_payload, "{\"d\":{\"CPULoad\": %f }}", getCPULoad());
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