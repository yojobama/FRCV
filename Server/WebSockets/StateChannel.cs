using EmbedIO;
using EmbedIO.WebSockets;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text.Json;
using System.Threading;
using System.Threading.Tasks;

namespace Server.WebSockets
{
    public record struct NodeStatsDto(double Fps, long LatencyUs);
    public record struct DeviceStatsDto(int CpuUsagePercent, int RamUsageMb, int DiskUsagePercent, int TemperatureC);
    public record struct SinkStateDto(Sink Sink, bool IsRunning);
    public record struct StateSnapshotDto(Source[] Sources, SinkStateDto[] Sinks, DeviceStatsDto Device, Dictionary<int, NodeStatsDto> NodeStats);

    // ROADMAP.md Phase 8a: pushes one consolidated state snapshot to every connected client on a
    // fixed server-side tick, replacing the webui's old per-client polling loop
    // (App.tsx's setInterval + useAppData.ts - confirmed each poll cost 1 (sources, itself 4
    // requests) + 1 (sinks) + N (one IsSinkActive native call per sink, every poll, every client)
    // + 3 (device stats) HTTP round trips). Computed once per tick here regardless of how many
    // clients are connected, then fanned out via BroadcastAsync - not once per client poll.
    public class StateChannel : WebSocketModule
    {
        private static readonly TimeSpan TickInterval = TimeSpan.FromSeconds(1);
        // per-node id: the frame count and wall-clock time it was sampled at, so the next tick
        // can turn "count now" into "frames per second" - see Manager::GetFrameCount's own
        // comment on why this division of labour (native: raw counter, C#: the delta) rather
        // than computing FPS natively.
        private readonly Dictionary<int, (ulong count, DateTime at)> _lastSample = new();

        public StateChannel(string urlPath) : base(urlPath, true)
        {
        }

        protected override Task OnMessageReceivedAsync(IWebSocketContext context, byte[] buffer, IWebSocketReceiveResult result)
            => Task.CompletedTask; // one-way channel - clients don't send anything meaningful today

        protected override void OnStart(CancellationToken cancellationToken)
        {
            base.OnStart(cancellationToken);
            _ = BroadcastLoopAsync(cancellationToken);
        }

        private async Task BroadcastLoopAsync(CancellationToken cancellationToken)
        {
            while (!cancellationToken.IsCancellationRequested)
            {
                try
                {
                    string json = JsonSerializer.Serialize(BuildSnapshot());
                    await BroadcastAsync(json);
                }
                catch (Exception ex)
                {
                    // a broadcast tick failing (e.g. a client disconnecting mid-send) must not
                    // kill the loop - the next tick should still go out to everyone still
                    // connected, matching how a single bad HTTP poll never used to take down the
                    // old polling loop either.
                    Console.WriteLine($"StateChannel broadcast tick failed: {ex.Message}");
                }

                try
                {
                    await Task.Delay(TickInterval, cancellationToken);
                }
                catch (TaskCanceledException)
                {
                    break;
                }
            }
        }

        private StateSnapshotDto BuildSnapshot()
        {
            var sourceIds = SourceManager.Instance.GetAllSourceIds();
            var sinkIds = SinkManager.Instance.getAllSinkIds();

            var sources = sourceIds.Select(id => SourceManager.Instance.GetSourceById(id)).ToArray();
            var sinks = sinkIds.Select(id => new SinkStateDto(
                SinkManager.Instance.GetSinkById(id),
                SinkManager.Instance.IsSinkRunning(id))).ToArray();

            // dual-role sinks (ApriltagDetector, ObjectDetectionSink, etc.) share the source id
            // space (see Manager::DeleteSink's own comment on this) - tracking every source id
            // AND every sink id covers both real cameras and detection sinks' own throughput
            // with one loop; GetFrameCount/GetLatencyUs just return 0 for whichever half of
            // each id doesn't apply.
            var trackedIds = sourceIds.Concat(sinkIds).Distinct();
            var nodeStats = new Dictionary<int, NodeStatsDto>();
            DateTime now = DateTime.UtcNow;

            foreach (int id in trackedIds)
            {
                ulong count = ManagerWrapper.Instance.GetFrameCount(id);
                long latencyUs = ManagerWrapper.Instance.GetLatencyUs(id);

                double fps = 0;
                if (_lastSample.TryGetValue(id, out var last) && count >= last.count)
                {
                    double elapsedSeconds = (now - last.at).TotalSeconds;
                    if (elapsedSeconds > 0) fps = (count - last.count) / elapsedSeconds;
                }
                _lastSample[id] = (count, now);

                nodeStats[id] = new NodeStatsDto(fps, latencyUs);
            }

            var resourceInfo = LinuxResourceMonitor.Instance.GetLatestResourceInfo();
            var device = new DeviceStatsDto(
                (int)resourceInfo.CpuUsagePercent,
                (int)resourceInfo.UsedMemoryMB,
                (int)resourceInfo.RootDiskUsage.UsedPercent,
                ManagerWrapper.Instance.GetCpuTemperature());

            return new StateSnapshotDto(sources, sinks, device, nodeStats);
        }
    }
}
