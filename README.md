# Zephyr ipsp connect

This application is supposed to connect to a 6lowpan border router and recieve an ipv6 address through neighbour discovery.

# Create a 6loWPAN border router using a raspberry pi
Before you begin make sure to have an up to date verion of raspbian installed on your raspberry pi. Note: In order to have several devices connected to the router at once, you need to make sure that the kernel has [this](https://www.spinics.net/lists/linux-bluetooth/msg72040.html) patch.

- Step 1, install radvd
   - `sudo apt-get install radvd`
- Step 2, enable the 6lowpan kernel module
   - `sudo modprobe bluetooth_6lowpan`
   - `sudo su` (Need to be logged in as root for next command)
   - `echo 1 > /sys/kernel/debug/bluetooth/6lowpan_enable`
- Step 3, Configure radvd and ipv6 forwarding
   - Create the file `/etc/radvd.conf` with the following content. Note: `2001:db8::` should be replaced by the prefix of your choice
      ```
         interface bt0
         {
            IgnoreIfMissing on;
            AdvSendAdvert on;
            prefix 2001:db8::/64
            {
               AdvOnLink off;
               AdvAutonomous on;
               AdvRouterAddr on;
            };
         };
      ```
   - Enable ipv6 forwarding: `sudo echo 1 > /proc/sys/net/ipv6/conf/all/forwarding`
- Step 4, Restart the radvd service
   - `sudo systemctl enable radvd.service` (Might already be enabled but you can do this just in case)
   - `sudo systemctl restart radvd.service`
- Step 5, At this point the routing part is done, all that is left is to make the router connectable through bluetooth
   - `sudo hciconfig hci0 up` turn on bluetooth
   - `sudo hciconfig hci0 noscan` Disable page and inquiry scanning 
   - `sudo hciconfig hci0 leadv 0` Begin LE advertising and make connectable
### You're done!
Congartulations! you just created a border router for 6lowpan. It might be a good idea to find out what the bluetooth address is for your router. Just use the command `hciconfig` and you will be presented with something like this.
 ``` 
   hci0:    Type: USB
            BD Address: FF:FF:FF:FF:FF:FF ACL MTU: 377:10 SCO MTU: 64:8
            UP RUNNING 
            RX bytes:348 acl:0 sco:0 events:11 errors:0
            TX bytes:38 acl:0 sco:0 commands:11 errors:0
```
`FF:FF:FF:FF:FF:FF` would be the Bluetooth address in this case.

## Connecting to your new router
### From a linux machine
- From linux you have to enable the 6lowpan module first
   - `sudo modprobe bluetooth_6lowpan`
   - `sudo su` (Need to be logged in as root for next command)
   - `echo 1 > /sys/kernel/debug/bluetooth/6lowpan_enable`
- Now while still logged in as root, connect using the following command
   - `echo "connect <The BD-address of your router> 1" > /sys/kernel/debug/bluetooth/6lowpan_control`
When the connection is successful, a `bt0` interface should be created which you can use with the internets, GLHF!
### Using this sample
- Look for the line `#define PEER_BT_ADDRESS "xx:xx:xx:xx:xx:xx"` and replace `xx:xx:xx:xx:xx:xx` with the bt_address of your router (All letters must be in lowercase). Then do the regular build commands
- `mkdir build && cd build`
- `cmake -GNinja -DBOARD=<your_target_board> .. && ninja`
- `ninja flash`

## Troubleshooting
If you can't get a connection to the router, try to reset hci0 using `hciconfig hci0 reset`. This will require step 5 to be repeated.