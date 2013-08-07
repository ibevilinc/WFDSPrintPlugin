/*
(c) Copyright 2013 Hewlett-Packard Development Company, L.P.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/
package com.android.wfds.discoveryservice;

import android.content.Context;
import android.net.ConnectivityManager;
import android.net.DhcpInfo;
import android.net.NetworkInfo;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.util.Log;

import java.io.IOException;
import java.net.InetAddress;
import java.net.InterfaceAddress;
import java.net.MulticastSocket;
import java.net.NetworkInterface;
import java.net.SocketException;
import java.net.UnknownHostException;
import java.util.List;


public class WifiUtils {

    private static final String EthernetInterface = "eth0";
    private static final int MULTICAST_TTL = 255;

    private static final String TAG = WifiUtils.class.getSimpleName();

    private Context context;

    public WifiUtils(Context context) {
        this.context = context;
    }

    public boolean isWifiConnected() {
        WifiInfo info = this.getWifiInfo();

        return connectedToNetwork(info) && obtainedIpAddress(info);
    }

    private WifiInfo getWifiInfo() {
        return this.getWifiManager().getConnectionInfo();
    }

    private DhcpInfo getDhcpInfo() {
        return this.getWifiManager().getDhcpInfo();
    }

    private WifiManager getWifiManager() {
        return (WifiManager) context.getSystemService(Context.WIFI_SERVICE);
    }

    private static boolean connectedToEthernet(Context context) {
        ConnectivityManager connMgr = (ConnectivityManager) context.getSystemService(Context.CONNECTIVITY_SERVICE);
        NetworkInfo ethInfo = connMgr.getNetworkInfo(ConnectivityManager.TYPE_ETHERNET);
        if (ethInfo == null) {
            return false;
        }
        return ethInfo.isConnectedOrConnecting();
    }

    private static boolean connectedToNetwork(WifiInfo info) {
        return ((info != null) && (info.getSSID() != null));
    }

    private static boolean obtainedIpAddress(WifiInfo info) {
        return ((info != null) && (info.getIpAddress() != 0));
    }

    public InetAddress getBroadcastAddress() throws UnknownHostException {
        if (connectedToEthernet(context)) {
            NetworkInterface netIf = null;
            try {
                netIf = NetworkInterface.getByName(EthernetInterface);
            } catch(SocketException e) {
                netIf = null;
            }
            if (netIf == null)
                return null;

            InetAddress broadcastAddress = null;
            List<InterfaceAddress> addresses = netIf.getInterfaceAddresses();
            if ((addresses != null) && !addresses.isEmpty()) {

                for(InterfaceAddress address : addresses) {
                    InetAddress bAddr = address.getBroadcast();
                    if (bAddr != null) {
                        broadcastAddress = bAddr;
                    }
                }
            }
            return broadcastAddress;
        } else {
            WifiInfo wifiInfo = this.getWifiInfo();
            DhcpInfo dhcpInfo = this.getDhcpInfo();

            if ((wifiInfo == null) || (dhcpInfo == null)) {
                return null;
            }
            int hostAddr = wifiInfo.getIpAddress();
            int broadcastAddress = (hostAddr & dhcpInfo.netmask) | ~dhcpInfo.netmask;
            byte[] broadcastAddressBytes = {
                    (byte) (broadcastAddress & 0xFF),
                    (byte) ((broadcastAddress >> 8) & 0xFF),
                    (byte) ((broadcastAddress >> 16) & 0xFF),
                    (byte) ((broadcastAddress >> 24) & 0xFF)};

            return InetAddress.getByAddress(broadcastAddressBytes);
        }
    }

    public boolean isWifiStateEnabled() {
        int state = this.getWifiManager().getWifiState();

        return ((state == WifiManager.WIFI_STATE_ENABLING)
                || (state == WifiManager.WIFI_STATE_ENABLED));
    }

    /**
     * Ensures that the caller receives an usable multicast socket.
     * In case the network configuration does not have a default gateway
     * set, the multicast socket might not work. This is the case when
     * the device is connected to a Wireless Direct printer. In that
     * cases, force a network interface into the socket.
     *
     * @return A ready-to-use multicast socket.
     */
    public static MulticastSocket createMulticastSocket(Context context)
            throws UnknownHostException, SocketException, IOException {
        MulticastSocket multicastSocket = new MulticastSocket();

        if (connectedToEthernet(context)) {
            NetworkInterface netIf = NetworkInterface.getByName("eth0");
            if (netIf != null)
                multicastSocket.setNetworkInterface(netIf);
        }
        else if (isWirelessDirect(context)) {
            WifiManager wifiManager = (WifiManager) context.getSystemService(Context.WIFI_SERVICE);
            WifiInfo wifiInfo = wifiManager.getConnectionInfo();
            int intaddr = wifiInfo.getIpAddress();
            byte[] byteaddr = new byte[] {
                    (byte) (intaddr & 0xff),
                    (byte) (intaddr >> 8 & 0xff),
                    (byte) (intaddr >> 16 & 0xff),
                    (byte) (intaddr >> 24 & 0xff)};
            InetAddress addr = InetAddress.getByAddress(byteaddr);
            NetworkInterface netIf = NetworkInterface.getByInetAddress(addr);

            multicastSocket.setNetworkInterface(netIf);
        }
        multicastSocket.setTimeToLive(MULTICAST_TTL);
        return multicastSocket;
    }

    public static boolean isWirelessDirect(Context context) {
        ConnectivityManager connManager =
                (ConnectivityManager) context.getSystemService(Context.CONNECTIVITY_SERVICE);
        NetworkInfo netInfo = connManager.getActiveNetworkInfo();

        if (netInfo != null && netInfo.isConnected() && (netInfo.getType() == ConnectivityManager.TYPE_WIFI)) {
            WifiManager wifiManager =
                    (WifiManager) context.getSystemService(Context.WIFI_SERVICE);
            DhcpInfo dhcpInfo = wifiManager.getDhcpInfo();

            if ((dhcpInfo != null) && (dhcpInfo.gateway == 0)) {
                Log.d(TAG, "isWirelessDirect: probably wireless direct.");
                return true;
            }
        }
        return false;
    }
}
