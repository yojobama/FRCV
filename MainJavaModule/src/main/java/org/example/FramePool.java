package org.example;

import org.opencv.core.Mat;

import java.util.*;
import java.util.concurrent.ConcurrentLinkedQueue;

public class FramePool {
    private final HashMap<FrameSpec, ConcurrentLinkedQueue<Mat>> frames;

    public FramePool(List<FrameSpec> specs) {
        frames = new HashMap<>();

        // check for duplicates
        while(!specs.isEmpty()) {
            int count = 0;
            FrameSpec spec = specs.getFirst();
            for (int i = specs.size() - 1; i >= 0; i--) {
                if (specs.get(i) == specs.getFirst()) {
                    count++;
                    specs.remove(i);
                }
            }
            frames.put(spec, new ConcurrentLinkedQueue<>());
            for (int i = 0; i < count; i++) {
                frames.get(spec).add(new Mat());
            }
        }
    }

    public Optional<Mat> acquireFrame(FrameSpec spec) {
        Mat value = frames.get(spec).poll();
        if (value != null) {
            return Optional.of(value);
        } else {
            return Optional.empty();
        }
    }

    public boolean releaseFrame(FrameSpec spec, Mat frame) {
        return frames.get(spec).offer(frame);
    }
}
