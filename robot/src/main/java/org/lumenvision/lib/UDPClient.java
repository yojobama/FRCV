package frcv;

import java.net.DatagramSocket;
import java.net.Inet4Address;
import java.net.SocketException;
import java.net.http.HttpClient;

public class UDPClient {
    DatagramSocket ds;
    HttpClient httpClient;
    Inet4Address address;

    public UDPClient(Inet4Address address) throws SocketException {
        ds = new DatagramSocket();
        this.address = address;
        httpClient = HttpClient.newHttpClient();
    }

    public String getResults() {
        return null;
    }

    void queryStart() {
        
    }

    void queryStop() {

    }
}
