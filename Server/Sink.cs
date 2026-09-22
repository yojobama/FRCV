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
                        // unreachable in practice - see AddSink's identical no-model-overload
                        // comment; a real ObjectDetectionSink only ever comes from
                        // SinkManager.AddObjectDetectionSink, which resolves the provider from
                        // the chosen model's own file format (Model.Provider), not a hardcoded one.
                        id = ManagerWrapper.Instance.CreateObjectDetectionSink(ObjectDetectionProvider.ONNX);
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

        // ROADMAP.md Phase 8c: this constructor used to accept an unused `source` parameter -
        // no call site anywhere in this codebase ever passed one (confirmed by grepping every
        // `new Sink(...)` call site), but its mere presence broke deserialization: with exactly
        // one public constructor, System.Text.Json deserializes via constructor-parameter
        // matching (case-insensitively matching JSON property names to parameter names) rather
        // than property setters wherever a match exists, so a JSON "Source" property was being
        // passed to this constructor's own `source` parameter - which the constructor body then
        // silently discarded, never assigning it to the backing field - instead of reaching the
        // public Source property's setter the way Source2/DepthSourceId already correctly do
        // (neither has a matching constructor parameter). Every sink's primary Source binding
        // was silently lost on every single deserialization - i.e. every server restart with a
        // bound sink - confirmed the hard way while testing ROADMAP.md Phase 8c's live graph
        // rebuild across a restart. Removing the dead parameter fixes it: STJ now falls back to
        // the property setter for Source too, matching Source2/DepthSourceId.
        public Sink(int id, string name, SinkType type)
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
