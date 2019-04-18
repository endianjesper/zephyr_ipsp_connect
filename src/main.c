/*
 * Copyright (c) 2018 Endian Technologies
 *
 * SPDX-License-Identifier: Apache-2.0
 * 
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <errno.h>
#include <zephyr.h>
#include <logging/log.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>
#include <misc/byteorder.h>
#include <net/net_ip.h>
#include <net/net_if.h>
#include <net/bt.h>

LOG_MODULE_REGISTER(IPSP_ROUTER_CONNECT_SAMPLE, LOG_LEVEL_INF);

#define BT_CONNECTED 1
#define BT_DISCONNECTED 0

/* Will attempt to connect to this bluetooth address */
#define PEER_BT_ADDRESS "00:1a:7d:da:71:0b"

#define BT_THREAD_STACKSIZE 1024
K_THREAD_STACK_DEFINE(bt_thread_stack, BT_THREAD_STACKSIZE);

static struct k_thread bt_thread_data;

static struct net_if *interface;

static struct in6_addr *ip6_address;

static u8_t bt_conn_status;

static void check_device(const bt_addr_le_t *addr, s8_t rssi, u8_t type,
						 struct net_buf_simple *ad)
{
	char dev_addr[BT_ADDR_LE_STR_LEN];

	/* If device isn't connectable then skip */
	if (type != BT_LE_ADV_IND && type != BT_LE_ADV_DIRECT_IND)
	{
		return;
	}

	/* convert device address to string format */
	bt_addr_le_to_str(addr, dev_addr, sizeof(dev_addr));

	/* The address will contain some extra bytes that we don't want so remove it */
	char dev_addr_format[BT_ADDR_LE_STR_LEN];
	strncpy(dev_addr_format, dev_addr, sizeof(PEER_BT_ADDRESS));
	dev_addr_format[sizeof(PEER_BT_ADDRESS) - 1] = '\0';

	LOG_INF("Found device: %s", log_strdup(dev_addr));

	/* Check if the device is what we are looking for */
	if (strcmp(dev_addr_format, PEER_BT_ADDRESS) != 0)
	{
		return;
	}
	LOG_INF("Device with address %s was found", PEER_BT_ADDRESS);

	/* Stop the scan */
	if (bt_le_scan_stop())
	{
		return;
	}

	LOG_INF("Attemting to connect to %s", log_strdup(dev_addr_format));
	net_mgmt(NET_REQUEST_BT_CONNECT, interface, (void *)addr, sizeof(*addr));
}

static void bt_scan(void)
{
	LOG_INF("Scanning for connectable device with address %s", PEER_BT_ADDRESS);

	/* Begin scanning for connectable devices */
	int ret = bt_le_scan_start(BT_LE_SCAN_ACTIVE, check_device);
	if (ret)
	{
		LOG_INF("Scanning failed to start, bt_le_scan_start returned %d", ret);
		return;
	}
}

static void connected(struct bt_conn *conn, u8_t err)
{
	LOG_INF("Bluetooth connection established, waiting for ipv6 address");
	bt_conn_status = BT_CONNECTED;
}

static void disconnected(struct bt_conn *conn, u8_t reason)
{
	LOG_INF("Bluetooth disconnected");
	bt_conn_status = BT_DISCONNECTED;
}

static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
};

void main(void)
{
	bt_conn_status = BT_DISCONNECTED;

	int ret = bt_enable(NULL);

	if (ret)
	{
		LOG_INF("Bluetooth could not be initialized, bt_enable returned %d", ret);
		return;
	}

	LOG_INF("Bluetooth initialized");

	bt_conn_cb_register(&conn_callbacks);

	/* Get the default interface in use */
	interface = net_if_get_default();

	if (!interface)
	{
		LOG_INF("Interface is NULL!");
	}

	/* Initiate ble scan on separate thread */
	k_thread_create(&bt_thread_data, bt_thread_stack, BT_THREAD_STACKSIZE,
					(k_thread_entry_t)bt_scan,
					NULL, NULL, NULL, K_PRIO_COOP(7), 0, 0);

	while (1)
	{
		if (bt_conn_status == BT_CONNECTED)
		{
			/* check if there is an assigned ipv6 global address */
			ip6_address = net_if_ipv6_get_global_addr(&interface);
			if (ip6_address)
			{
				char ip6_address_str[INET6_ADDRSTRLEN];

				/* Convert ipv6 address to string */
				net_addr_ntop(AF_INET6, ip6_address, ip6_address_str, sizeof(ip6_address_str));

				LOG_INF("The interface has been assigned the global address %s", log_strdup(ip6_address_str));
				break;
			}
		}
		k_sleep(500);
	}

	while (1)
	{
		k_sleep(100000);
	}
}