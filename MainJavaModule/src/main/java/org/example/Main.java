package org.example;

import org.opencv.core.Core;
import org.opencv.core.Mat;
import org.opencv.videoio.VideoCapture;
// Removed invalid import

//TIP To <b>Run</b> code, press <shortcut actionId="Run"/> or
// click the <icon src="AllIcons.Actions.Execute"/> icon in the gutter.
public class Main {
    public static void main(String[] args) {
        System.loadLibrary(Core.NATIVE_LIBRARY_NAME);

        VisionCoordinator coordinator;
        try {
            coordinator = new VisionCoordinator(new VisionModule[]{
                    new VisionModule(
                            new Pipeline[]{
                                    new ApriltagPipeline(new Mat(), new FrameSpec(640, 480, 0))
                            },
                            new VideoCapture(0)
                    )});

            coordinator.runPipelines();
            Runtime.getRuntime().addShutdownHook(new Thread(coordinator::stopPipelines));
            while(true);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }
}