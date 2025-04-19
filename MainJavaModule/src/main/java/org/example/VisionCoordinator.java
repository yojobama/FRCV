package org.example;

import org.opencv.videoio.VideoCapture;

import java.util.ArrayList;
import java.util.List;

public class VisionCoordinator {
    private final VisionModule[] modules;
    List<Thread> threads = new ArrayList<>();

    public VisionCoordinator(VisionModule[] modules) {
        this.modules = modules;
    }

    void stopPipelines() {
        for (VisionModule module : modules) {
            module.camera().release();
        }
    }

    void runPipelines() {
        for (VisionModule module : modules) {
            threads.add(new Thread(() -> {
                VideoCapture capture = module.camera();
                while (capture.isOpened()) {
                    for (Pipeline pipeline: module.pipelines()) {
                        if (!pipeline.getIsProcessing()) {
                            capture.read(pipeline.getFrame());
                            Thread.ofVirtual().start(
                                    pipeline::processFrame
                            );
                        }
                    }
                }
            }));
        }
    }
}
