package org.example;

import org.opencv.core.Point3;

public record ApriltagDetection(int id, double confidence, Point3 pose) {
}
