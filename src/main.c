/* main.c - Application main entry point */

/*
 * Copyright (c) 2018 Endian Technologies
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <errno.h>
#include <zephyr.h>
#include <misc/printk.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>
#include <misc/byteorder.h>
#include <net/net_ip.h>
#include <net/net_if.h>
#include <net/bt.h>

/* Will attempt to connect to this bluetooth address */
#define PEER_BT_ADDRESS "b8:27:eb:8e:32:17"

#define BT_THREAD_STACKSIZE 1024
K_THREAD_STACK_DEFINE(bt_thread_stack, BT_THREAD_STACKSIZE);

static struct k_thread bt_thread_data;

static int connected;

static struct net_if *interface;

static struct in6_addr *ip6_address;


static void check_device(const bt_addr_le_t *addr, s8_t rssi, u8_t type,
			 struct net_buf_simple *ad)
{
	char dev_addr[BT_ADDR_LE_STR_LEN];

	/* If device isn't connectable then skip */
	if (type != BT_LE_ADV_IND && type != BT_LE_ADV_DIRECT_IND) {
		return;
	}

    /* convert device address to string format */
	bt_addr_le_to_str(addr, dev_addr, sizeof(dev_addr));
	
    /* The address will contain some extra bytes that we don't want so remove it */
    char dev_addr_format[BT_ADDR_LE_STR_LEN];
    strncpy(dev_addr_format, dev_addr, sizeof(PEER_BT_ADDRESS)); 
	dev_addr_format[sizeof(PEER_BT_ADDRESS)-1] = '\0';

	printk("Found device: %s\n",dev_addr);

    /* Check if the device is what we are lloking for */
    if(strcmp(dev_addr_format, PEER_BT_ADDRESS)!=0){
        return;
    }
	printk("Device with address %s was found\n", PEER_BT_ADDRESS);

    /* Stop the scan */
	if (bt_le_scan_stop()) {
		return;
	}

    printk("Attemting to connect to %s\n", dev_addr_format);
	net_mgmt(NET_REQUEST_BT_CONNECT, interface, (void*)addr, sizeof(*addr));
	connected = 1;
}

static void bt_scan(void){
	printk("Scanning for connectable device with address %s\n", PEER_BT_ADDRESS);

    /* Begin scanning for connectable devices */ 
	int ret = bt_le_scan_start(BT_LE_SCAN_ACTIVE, check_device);
	if (ret) {
		printk("Scanning failed to start, bt_le_scan_start returned %d\n", ret);
		return;
	}
}



void main(void)
{
	connected = 0;

	int ret = bt_enable(NULL);
	if (ret) {
		printk("Bluetooth could not be initialized, bt_enable returned %d\n", ret);
		return;
	}

	printk("Bluetooth initialized\n");


	/* Get the default interface in use */ 
	interface = net_if_get_default();

	if(!interface){
		printk("Interface is NULL!\n");
	}

	k_thread_create(&bt_thread_data, bt_thread_stack, BT_THREAD_STACKSIZE,
			(k_thread_entry_t)bt_scan,
			NULL, NULL, NULL, K_PRIO_COOP(7), 0, 0);	

	while(1){
		if(connected){
			/* check if there is an assigned ipv6 global address */
			ip6_address = net_if_ipv6_get_global_addr(&interface);
			if(ip6_address){
				char ip6_address_str[INET6_ADDRSTRLEN];

				/* Convert ipv6 address to string */
				net_addr_ntop(AF_INET6, ip6_address, ip6_address_str, sizeof(ip6_address_str));
			
				printk("The interface has been assigned the global address %s", ip6_address_str);
				break;
			}
			
			
		}
		k_sleep(500);
	}
}