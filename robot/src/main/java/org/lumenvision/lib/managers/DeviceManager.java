package frcv.managers;

import com.sun.net.httpserver.HttpContext;

import java.net.URI;
import java.net.http.HttpClient;
import java.net.http.HttpRequest;

public class DeviceManager {

    final HttpClient httpClient;
    HttpContext context;

    String m_address;

    final HttpRequest m_getProcessorUsageRequest;
    final HttpRequest m_getTemperatureRequest;
    final HttpRequest m_getMemoryUsageRequest;
    final HttpRequest m_getDiskUsageRequest;

    public DeviceManager(String address) {
        this.m_address = address;

        // creating the http client
        httpClient = HttpClient.newBuilder()
                .version(HttpClient.Version.HTTP_2)
                .followRedirects(HttpClient.Redirect.NORMAL)
                .build();

        // creating all of the requests

        m_getProcessorUsageRequest = HttpRequest.newBuilder()
                .uri(URI.create(m_address + "/api/device/cpuUsage"))
                .GET()
                .build();
        m_getTemperatureRequest = HttpRequest.newBuilder()
                .uri(URI.create(m_address + "/api/device/cpuTemperature"))
                .GET()
                .build();
        m_getMemoryUsageRequest = HttpRequest.newBuilder()
                .uri(URI.create(m_address + "/api/device/ramUsage"))
                .GET()
                .build();
        m_getDiskUsageRequest = HttpRequest.newBuilder()
                .uri(URI.create(m_address + "/api/device/diskUsage"))
                .GET()
                .build();
    }

    // TODO: implement all of the methods here

    public int getProcessorUsage() {
        return 0;
    }

    public int getTemperature() {
        return 0;
    }

    public int getMemoryUsage() {
        return 0;
    }

    public int getDiskUsage() {
        return 0;
    }
}
