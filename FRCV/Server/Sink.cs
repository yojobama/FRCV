using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Server
{
    public enum SinkType
    {
        [Description("ApriltagSink")]
        ApriltagSink,
        [Description("ObjectDetectionSink")]
        ObjectDetectionSink,
        [Description("RecordingSink")]
        RecordingSink,
        [Description("CameraCalibrationSink")]
        CameraCalibrationSink,
        [Description("NetworkTablesSink")]
        NetworkTablesSink,
        [Description("WebRTCSink")]
        WebRTCSink,
        [Description("StereoCalibrationSink")]
        StereoCalibrationSink,
        [Description("StereoDepthSink")]
        StereoDepthSink,
        [Description("DepthFusionSink")]
        DepthFusionSink,
    }

    public class Sink
    {
       // class members 
        private SinkType type { get; set; }
        private int id { get; set; }
        private string name { get; set; }
        
        private Source? source;
        // the "right" source for a stereo sink (StereoCalibrationSink/StereoDepthSink) -
        // ordinary sinks only ever bind one source, but a stereo node's ISink side has
        // maxSources=2 with a fixed left/right meaning (see BindStereoSources /
        // IStereoRoleReceiver.h). `Source` above holds the left source for a stereo sink;
        // this holds the right one. Null for every non-stereo sink type.
        private Source? source2;

        // properties for the sink

        public SinkType Type
        {
            get => type;
            set
            {
                switch (value)
                {
                    case SinkType.ApriltagSink:
                        id = ManagerWrapper.Instance.CreateApriltagDetector();
                        break;
                    case SinkType.ObjectDetectionSink:
                        id = ManagerWrapper.Instance.CreateObjectDetectionSink(ObjectDetectionProvider.ONNX); // TODO: Add logic for selecting acceleration type (ONNX with REP, or Rknn)
                        break;
                    case SinkType.CameraCalibrationSink:
                        id = ManagerWrapper.Instance.CreateCameraCalibrator();
                        break;
                }
                type = value;
            }
        }

        public int Id
        {
            get => id;
        }

        public string Name
        {
            get => name;
            set => name = value;
        }
        
        public Source? Source
        {
            get => source;
            set => source = value;
        }

        public Source? Source2
        {
            get => source2;
            set => source2 = value;
        }

        // DepthFusionSink only: the StereoDepthSink id it reads its depth grid from directly
        // (see DepthFusionNode.h / AttachDepthFusionSource) - not a Source, since it isn't
        // reached through the normal ISink bind/Process path at all.
        public int? DepthSourceId { get; set; }

        public Sink(int id, string name, SinkType type, Source? source = null)
        {
            this.id = id;
            this.name = name;
            this.type = type;
        }

        public void ChangeType(SinkType type)
        {
            switch (type)
            {
                case SinkType.ApriltagSink:
                    id = ManagerWrapper.Instance.CreateApriltagDetector();
                    break;
                case SinkType.ObjectDetectionSink:
                    id = ManagerWrapper.Instance.CreateObjectDetectionSink(ObjectDetectionProvider.ONNX);
                    break;
                case SinkType.CameraCalibrationSink:
                    id = ManagerWrapper.Instance.CreateCameraCalibrator();
                    break;
            }
        }
    }
}
