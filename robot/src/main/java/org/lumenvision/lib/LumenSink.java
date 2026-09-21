package org.lumenvision.lib;

public abstract class LumenSink<Detection extends Object> {
    int id;
    String name;

    public LumenSink() {

    }

    public abstract Detection getDetection();
}
