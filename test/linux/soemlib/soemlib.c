#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <stdint.h>

#include "ethercat.h"

// IO databuffer
static char IOmap[4096];
// Expected working counter
static int expectedInputWKC;
static int expectedOutputWKC;
// List of network adapters
static ec_adaptert * adapters = NULL;

// Connect to network interface
// nic: The interface ID
// return: true=success / false=failure
boolean __stdcall soem_open(char* nic) {
	return ec_init(nic) > 0;
}

// Close the connection
void __stdcall soem_close(void) {
	ec_close();
}

#define ALL_DEVICES_OP_STATE	0	// All devices are in OP state
#define NO_DEVICES_FOUND		1	// No devices found
#define NOT_ALL_OP_STATE		2	// Not all devices are in OP state

// Configure attached devices
// return status: 0=All devices are in OP state / 1=No devices found / 2=Not all devices are in OP state
int __stdcall soem_config(void) {	
	// Find devices and compare them
	if (ec_config_init(FALSE) > 0) {
		printf("%d devices found and configured.\n",ec_slavecount);
		
		ec_config_map(&IOmap);
		ec_configdc();
		
		printf("Devices mapped, state to SAFE_OP.\n");
		
		// Request SAFE_OP state for all devices
		ec_statecheck(0, EC_STATE_SAFE_OP,	EC_TIMEOUTSTATE * 4);
		
		expectedOutputWKC = (ec_group[0].outputsWKC * 2);
		expectedInputWKC = ec_group[0].inputsWKC;
		
		// Request OP state
		ec_slave[0].state = EC_STATE_OPERATIONAL;
		// send one valid process data to make outputs in device happy
		ec_send_processdata();
		ec_receive_processdata(EC_TIMEOUTRET);
		ec_writestate(0);
		int chk = 40;
		// Wait for devices to become operational
		do {
			ec_send_processdata();
			ec_receive_processdata(EC_TIMEOUTRET);
			ec_statecheck(0, EC_STATE_OPERATIONAL, 50000);
		} while (chk-- && (ec_slave[0].state != EC_STATE_OPERATIONAL));
		
		if (ec_slave[0].state == EC_STATE_OPERATIONAL) {
			return ALL_DEVICES_OP_STATE;
		}

		return NOT_ALL_OP_STATE;
	}

	return NO_DEVICES_FOUND;
}

// Get amount of devices
// return: The number of devices
int __stdcall soem_getDeviceCount(void) {
	return ec_slavecount;
}


int __stdcall soem_getExpectedInputWorkingCount(void) {
	return expectedInputWKC;
}

int __stdcall soem_getExpectedOutputWorkingCount(void) {
	return expectedOutputWKC;
}

// Get current state of all devices
// return: The state of all devices
int __stdcall soem_getCurrentState(void) {
	return ec_readstate();
}

// Get state of a specific device
// device: The index of the device
// return: The status of the device
int __stdcall soem_getState(int device) {
	return ec_slave[device].state;
}

// Get AL status of a specific device
// device: The index of the device
// return: The AL status of the device
int __stdcall soem_getALStatusCode(int device)
{
	return ec_slave[device].ALstatuscode;
}

// Get AL status description for a specific device
// device: The index of the device
// desc: Buffer of 31 chars to store the description
void __stdcall soem_getALStatusDesc(int device, char* desc) {
	snprintf(desc, 31, "%s", ec_ALstatuscode2string( ec_slave[device].ALstatuscode));
}

// Request a state for a specific device
// device: The index of the device
// state: The state requested
void __stdcall soem_requestState(int device, int state) {
	ec_slave[device].state = state;
	ec_writestate(device);
}

// Get the name for a device
// device: The index of the device
// name: Buffer of 31 chars to store the name
void __stdcall soem_getName(int device, char* name) {
	snprintf(name, 31, "%s", ec_slave[device].name );
}

// Transfer PDO
// return: 
int __stdcall soem_transferPDO(void) {
	ec_send_processdata();
	return ec_receive_processdata(EC_TIMEOUTRET);
}

// Get input PDO of device
// device: The index of the device
// offset: Offset address
// return: Bytes read
uint8_t __stdcall soem_getInputPDO(int device, int offset) {
	uint8_t ret = 0;
	
	if (device <= ec_slavecount) {
		ret = ec_slave[device].inputs[offset];
	}

	return ret;
}

// Set output PDO
// device: The index of the device
// offset: Offset address
// value: Byte to be written
void __stdcall soem_setOutputPDO(int device, int offset, uint8_t value) {
	if (device <= ec_slavecount) {
		ec_slave[device].outputs[offset] = value;
	}
}

// Get output bits for a device
// device: The index of the device
// return: The amount of output bits
int __stdcall soem_getOutputBits(int device) {
	return ec_slave[device].Obits;
}

// Get input bits for a device
// device: The index of the device
// return: The amount of input bits
int __stdcall soem_getInputBits(int device) {
	return ec_slave[device].Ibits;
}

// Get amount of network adapters
// return: Number of adapters
int __stdcall soem_findAdapters(void) {
	adapters = ec_find_adapters();
	
	int num = 0;
	ec_adaptert * adapter = adapters;
	while (adapter != NULL) {
		adapter = adapter->next;
		num++;
	}

	return num;
}

// Get name of a network adapter
// index: The index of the adapter
// name: Buffer to store the name in
void __stdcall soem_getAdapterName(int index, char* name) {
	int num = 0;
	ec_adaptert * adapter = adapters;
	while (adapter != NULL) {
		if (num == index) {
			strcpy(name, adapter->name);
			return;
		}
		adapter = adapter->next;
		num++;
	}
	name[0] = '\0';
}

// Get description of a network adapter
// index: The index of the adapter
// desc: Buffer to store the description in
void __stdcall soem_getAdapterDesc(int index, char* desc) {
	int num = 0;
	ec_adaptert * adapter = adapters;
	while (adapter != NULL) {
		if (num == index) {
			strcpy(desc, adapter->desc);
			return;
		}
		adapter = adapter->next;
		num++;
	}
	desc[0] = '\0';
}