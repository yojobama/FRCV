package frcv;

public abstract class FRCVSink<Detection extends Object> {
    int id;
    String name;

    public FRCVSink() {

    }

    public abstract Detection getDetection();
}
