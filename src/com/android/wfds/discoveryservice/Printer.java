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

import android.os.Parcel;
import android.os.Parcelable;
import android.text.TextUtils;
import android.util.Log;

import java.net.InetAddress;
import java.net.UnknownHostException;

/**
 * There is no public constructor. Instances are either returned by the printer
 * discovery provided by the print system, read from parcels or loaded from
 * shared preferences.
 */
public final class Printer implements Parcelable {

    private static final String TAG = Printer.class.getSimpleName();

    /*
     * Keep these declarations ordered alphabetically by field name. This helps
     * to keep readFromParcel and writeToParcel up-to-date.
     */
    private final String bonjourName;
    private final String bonjourDomainName;
    private final InetAddress inetAddress;
    private final String model;
    private final String mPDLs;

    /**
     * Package-level visible constructor to be used by the discovery helper
     *
     * @param inetAddress the string representation of the printer IP address
     * @param model the printer model name (e.g. "Officejet 6500 E709n")
     * @param bonjourName the printer name (e.g.HPD11201). If null or empty, the model
     *            name will be used as printer name.
     * @param bonjourDomainName Bonjour Domain Name
     * @throws IllegalArgumentException if either inetAddress or model is null or
     *             empty.
     */
    Printer(InetAddress inetAddress, String model, String bonjourName, String bonjourDomainName, String pdls) throws IllegalArgumentException {
        this.inetAddress = checkInetAddress(inetAddress);
        this.model = checkModel(model);
        this.bonjourName = bonjourName;
        this.bonjourDomainName = bonjourDomainName;
        this.mPDLs = pdls;
    }

    private static InetAddress checkInetAddress(InetAddress inetAddress) throws IllegalArgumentException {
        if (inetAddress == null) {
            throw new IllegalArgumentException("inetAddress can not be null");
        }
        return inetAddress;
    }

    private static String checkModel(String model) throws IllegalArgumentException {
        if (model == null || model.length() == 0) {
            throw new IllegalArgumentException("model can not be null nor empty");
        }
        return model;
    }

    private Printer(Parcel in) throws UnknownHostException {
        this.bonjourName = in.readString();
        this.bonjourDomainName = in.readString();
        int inetAddrSize = in.readInt();
        if (inetAddrSize > 0) {
            byte[] addr = new byte[inetAddrSize];
            in.readByteArray(addr);
            this.inetAddress = InetAddress.getByAddress(addr);
        } else {
            this.inetAddress = null;
        }
        this.model = in.readString();
        this.mPDLs = in.readString();
    }

    /* (non-Javadoc)
     * @see java.lang.Object#equals(java.lang.Object)
     */
    @Override
    public boolean equals(Object obj) {
        Printer p = (Printer) obj;
        return p != null &&
                (this.inetAddress != null && this.inetAddress.equals(p.inetAddress) || p.inetAddress == null);
    }

    /**
     * Copy the contents of other into this object
     */
    public Printer(Printer other) {
        this.inetAddress = other.inetAddress;
        this.model = other.model;
        this.bonjourName = other.bonjourName;
        this.bonjourDomainName = other.bonjourDomainName;
        this.mPDLs = other.mPDLs;
    }

    /**
     * @return the inetAddress
     */
    public InetAddress getInetAddress() {
        return this.inetAddress;
    }

    /**
     * @return the printer model name, e.g. "HP Officejet 6500 E709n"
     */
    public String getModel() {
        return this.model;
    }

    public String getBonjourName() {
        return this.bonjourName;
    }

    public String getBonjourDomainName() {
        return this.bonjourDomainName;
    }

    /* (non-Javadoc)
     * @see java.lang.Object#toString()
     */
    @Override
    public String toString() {
        String details = getBonjourName();
        return details != null ? this.model + " [" + details + ']' : this.model;
    }

    /**
     * Report the nature of this Printer's contents
     *
     * @return a bitmask indicating the set of special object types marshalled
     *         by the Printer.
     * @see android.os.Parcelable#describeContents()
     */
    @Override
    public int describeContents() {
        return 0;
    }

    /**
     * Writes the Printer contents to a Parcel, typically in order for it to be
     * passed through an IBinder connection.
     *
     * @param parcel The parcel to copy this Printer to.
     * @param flags Additional flags about how the object should be written. May
     *            be 0 or {@link android.os.Parcelable#PARCELABLE_WRITE_RETURN_VALUE
     *            PARCELABLE_WRITE_RETURN_VALUE}.
     * @see android.os.Parcelable#writeToParcel(android.os.Parcel, int)
     */
    @Override
    public void writeToParcel(Parcel parcel, int flags) {
        parcel.writeString(this.bonjourName);
        parcel.writeString(this.bonjourDomainName);
        /*
         * In spite of what Coverity thinks, InetAddress.getAddress() can NOT
         * return null. Never.
         */
        if (this.inetAddress != null) {
            byte[] addr = this.inetAddress.getAddress();
            if(addr != null) {
                parcel.writeInt(addr.length);
                parcel.writeByteArray(addr);
            } else {
                parcel.writeInt(0);
            }
        } else {
            parcel.writeInt(0);
        }
        parcel.writeString(this.model);
        parcel.writeString(this.mPDLs);
    }

    /**
     * Reads Printers from Parcels.
     */
    public static final Creator<Printer> CREATOR = new Creator<Printer>() {
        @Override
        public Printer createFromParcel(Parcel in) {
            try {
                return new Printer(in);
            } catch (UnknownHostException e) {
                Log.e(TAG, "createFromParcel() failed", e);
            }
            return null;
        }

        @Override
        public Printer[] newArray(int size) {
            return new Printer[size];
        }
    };
}
