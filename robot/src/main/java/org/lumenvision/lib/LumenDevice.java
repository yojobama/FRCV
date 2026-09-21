package org.lumenvision.lib;

import org.lumenvision.lib.managers.DeviceManager;
import org.lumenvision.lib.managers.SinkManager;
import org.lumenvision.lib.managers.SourceManager;

import java.util.List;

public class LumenDevice {
    DeviceManager m_deviceManager;
    SinkManager m_sinkManager;
    SourceManager m_sourceManager;

    public LumenDevice(String name) {

    }

    public List<LumenSink> enumerateSinks() {
        return null;
    }
}
