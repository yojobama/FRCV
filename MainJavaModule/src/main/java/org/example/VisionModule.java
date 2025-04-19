package org.example;

import org.opencv.videoio.VideoCapture;

public record VisionModule(Pipeline[] pipelines, VideoCapture camera) {
}
