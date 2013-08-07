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
package com.android.wfds.printservice.tasks;

import java.io.IOException;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.SocketException;
import java.net.SocketTimeoutException;
import java.net.UnknownHostException;

import android.content.Context;
import android.content.Intent;
import android.os.Handler;
import android.os.Message;
import android.os.RemoteException;
import android.text.TextUtils;
import android.util.Log;

import com.android.wfds.common.MobilePrintConstants;
import com.android.wfds.common.messaging.AbstractMessage;
import com.android.wfds.common.messaging.IMessenger;
import com.android.wfds.discoveryservice.MultiProtocolDiscovery;
import com.android.wfds.discoveryservice.Printer;
import com.android.wfds.printservice.WPrintService;

public class LocalPrinterDiscoveryTask extends AbstractMessageTask
{

    private static final int DEFAULT_INITIAL_TIMEOUT = 8000;
    private static final int DEFAULT_TIMEOUT_DECAY = 2000;
    private static final int DEFAULT_TIMEOUT_AFTER_FOUND = 5000;
    private static final int BUFFER_LENGTH = 4 * 1024;

    private final MultiProtocolDiscovery multiProtocolDiscovery;
    private final IMessenger mClientCallBack;
    private byte[] buffer = new byte[BUFFER_LENGTH];
    private final WPrintService mContext;

    public LocalPrinterDiscoveryTask(WPrintService context, AbstractMessage msg, Handler parentHandler) {
        super(msg, parentHandler);
        mContext = context;
        mClientCallBack = msg.replyTo;
        multiProtocolDiscovery = new MultiProtocolDiscovery(context);
    }

    @Override
    public Intent doInBackground(Void... params) {
        DatagramSocket socket = null;
        try
        {
            socket = multiProtocolDiscovery.createSocket();
            socket.setReuseAddress(true);
            sendQueryPacket(socket);
            receiveResponsePackets(socket);

        } catch (UnknownHostException exc)
        {
            Log.i(MobilePrintConstants.TAG,
                    "Could not resolve hostname during discovery.", exc);
        } catch (IOException exc)
        {
            Log.e(MobilePrintConstants.TAG,
                    "IO error occurred during printer discovery.", exc);
        } finally
        {
            multiProtocolDiscovery.releaseSocket(socket);
        }
        return null;
    }

    private void sendQueryPacket(DatagramSocket socket) throws UnknownHostException, IOException
    {
        DatagramPacket[] queryPackets = multiProtocolDiscovery
                .createQueryPackets();

        for (DatagramPacket packet : queryPackets)
        {
            socket.send(packet);
        }
    }

    /*
     * The algorithm for receiving the response packets will decrease the
     * timeout according to the search results. Socket timeout starts with a
     * value of 8s. If no printer is found, the socket timeout is decreased by
     * 2s until it reaches 0 and the algorithm stops listening for new
     * responses. So when no printer is found, the sequence of socket timeouts
     * is 8, 6, 4, and 2s, adding up to 20s of wait time. When a printer is
     * found, both timeout and decay are set to 5s, which means that the thread
     * will receive new packets with a timeout of 5s until no packet is
     * received. The first time the receive method reaches the timeout without
     * receiving any packets, the algorithm finishes.
     */
    private void receiveResponsePackets(final DatagramSocket socket) throws IOException
    {

        int timeout = DEFAULT_INITIAL_TIMEOUT;
        int remainingTO = DEFAULT_TIMEOUT_AFTER_FOUND;
        int decay = DEFAULT_TIMEOUT_DECAY;
        DatagramPacket packet = new DatagramPacket(buffer, BUFFER_LENGTH);

        while (!Thread.interrupted() && (timeout > 0))
        {
            try
            {
                socket.setSoTimeout(timeout);
                socket.receive(packet);
                Log.d(MobilePrintConstants.TAG,
                        "Response from " + packet.getAddress() + ":"
                                + packet.getPort());
                if (!Thread.interrupted())
                {
                    if (processIncomingPacket(packet))
                    {

                        // A valid printer was found: wait for another
                        // 5s and stop listening.
                        timeout = remainingTO;
                        decay = timeout;
                    } else
                    {
                        Log.w(MobilePrintConstants.TAG,
                                "Printer could not be parsed or is not supported.");
                    }
                    // Resets the packet length to reuse the packet.
                    packet.setLength(BUFFER_LENGTH);
                } else {
                    timeout = 0;
                }
            } catch (SocketTimeoutException exc)
            {

                // Socket timed out and we received no packets. If at
                // least one printer has
                // already been found, timeout is set to zero and the
                // algorithm finishes.
                // Otherwise, we will decrease the timeout by 2s and try
                // again.
                sendQueryPacket(socket);
                timeout -= decay;
            } catch (SocketException e)
            {
                e.printStackTrace();
            } catch (IOException e)
            {
                e.printStackTrace();
            }
        }
    }

    private boolean processIncomingPacket(DatagramPacket packet)
    {
        boolean foundSupportedPrinter = false;
        Printer[] printers = multiProtocolDiscovery.parseResponse(packet);

        if ((printers != null) && (printers.length > 0))
        {
            for (Printer printer : printers)
            {
                printerFound(printer);
                foundSupportedPrinter = true;
            }
        }
        return foundSupportedPrinter;
    }

    protected void printerFound(Printer printer)
    {
        String value;
        Intent returnIntent = new Intent(
                MobilePrintConstants.ACTION_PRINT_SERVICE_RETURN_DEVICE_RESOLVED);

        // Add Name
        returnIntent.putExtra(MobilePrintConstants.DISCOVERY_DEVICE_NAME,
                printer.getModel());
        // Add IP Address
        returnIntent.putExtra(MobilePrintConstants.DISCOVERY_DEVICE_ADDRESS,
                printer.getInetAddress().getHostAddress());

        value = printer.getBonjourName();
        if (!TextUtils.isEmpty(value))
            returnIntent.putExtra(MobilePrintConstants.DISCOVERY_DEVICE_BONJOUR_NAME, value);
        value = printer.getBonjourDomainName();
        if (!TextUtils.isEmpty(value))
            returnIntent.putExtra(MobilePrintConstants.DISCOVERY_DEVICE_BONJOUR_DOMAIN_NAME, value);
        try
        {
            if (mClientCallBack != null)
            {
                mClientCallBack.send(Message.obtain(null, 0, returnIntent));
            }
        } catch (RemoteException e)
        {
            e.printStackTrace();
        }
    }
}
