namespace Server
{
    // a stored YOLOv8/v11 model (ONNX or RKNN export), uploaded via ModelController and
    // referenced by id when creating an ObjectDetectionSink
    public class Model
    {
        public int Id { get; set; }
        public string Name { get; set; } = "";
        public string ModelPath { get; set; } = "";
        public string LabelsPath { get; set; } = "";
        public YoloVariant Variant { get; set; }
        public int InputSize { get; set; } = 640;
        public float ConfThreshold { get; set; } = 0.25f;
        public float NmsThreshold { get; set; } = 0.45f;

        // which backend this model actually runs on - derived once, at upload, from the
        // uploaded file's own extension (see ModelManager.AddModel), not a preference a user
        // sets: an RKNN NPU export and an ONNX graph are different file formats, not the same
        // model choosing a different runtime the way ApriltagDetector's CPU/Vulkan backends do.
        public ObjectDetectionProvider Provider { get; set; }
    }
}
