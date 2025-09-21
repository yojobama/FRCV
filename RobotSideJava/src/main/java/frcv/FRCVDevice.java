package frcv;

import frcv.managers.DeviceManager;
import frcv.managers.SinkManager;
import frcv.managers.SourceManager;

import java.util.List;

public class FRCVDevice {
    DeviceManager m_deviceManager;
    SinkManager m_sinkManager;
    SourceManager m_sourceManager;

    public FRCVDevice(String name) {

    }

    public List<FRCVSink> enumerateSinks() {
        return null;
    }
}
