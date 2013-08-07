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

import java.io.IOException;
import java.lang.ref.WeakReference;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.MulticastSocket;
import java.net.UnknownHostException;
import java.util.ArrayList;

import android.content.Context;
import android.util.Log;

public class MultiProtocolDiscovery implements IDiscovery {
    private static final String TAG = MultiProtocolDiscovery.class.getSimpleName();

    private WeakReference<Context> context;
    private ArrayList<IDiscovery> mDiscoveryMethods = new ArrayList<IDiscovery>();

    public MultiProtocolDiscovery(Context context) {
        this.context = new WeakReference<Context>(context);
        mDiscoveryMethods.add(new MDnsDiscovery(context));
    }

    @Override
    public DatagramSocket createSocket() throws IOException {

        // SNMP requires a broadcast socket, while mDNS requires a multicast socket with a TTL set.
        // Let's create an hybrid one and make everyone happy.
        MulticastSocket socket = WifiUtils.createMulticastSocket(context.get());

        socket.setBroadcast(true);
        return socket;
    }

    @Override
    public void releaseSocket(DatagramSocket socket) {
        if (socket != null) {
            socket.close();
        }
    }

    @Override
    public DatagramPacket[] createQueryPackets() throws UnknownHostException {

        ArrayList<DatagramPacket> datagramList = new ArrayList<DatagramPacket>();
        for(IDiscovery discoverMethod : mDiscoveryMethods) {
            DatagramPacket[] datagramPackets = discoverMethod.createQueryPackets();
            for(DatagramPacket packet : datagramPackets) {
                datagramList.add(packet);
            }
        }

        DatagramPacket[] discoveryPackets = new DatagramPacket[datagramList.size()];
        return datagramList.toArray(discoveryPackets);
    }

    @Override
    public final Printer[] parseResponse(DatagramPacket packet) {
        int port = packet.getPort();
        for(IDiscovery discoveryMethod : mDiscoveryMethods) {
            if (port == discoveryMethod.getPort()) {
                return discoveryMethod.parseResponse(packet);
            }
        }
        Log.e(TAG, "Received packet from unexpected port: "
                + packet.getAddress() + ":" + port);
        return null;
    }

    @Override
    public int getPort() {
        return -1;
    }
}
