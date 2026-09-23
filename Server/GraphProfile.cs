namespace Server
{
    // ROADMAP.md Phase 8/E6: save/restore the ENTIRE node graph under a name - distinct from the
    // existing per-source AprilTag detection profile (PipelineProfile.cs/
    // PipelineProfileController.cs, already shipped: one camera source's own tag-size/
    // calibration/backend settings). This is "swap the whole coprocessor's job" - every source,
    // every sink, every binding - the shape a team would actually want for "match config" vs.
    // "pit/bench config" rather than reconfiguring node by node.
    //
    // Deliberately thin: DB.cs already has fully proven logic for both directions (Save() always
    // re-serializes Manager's live state fresh, and Load()/LoadInternal() already knows how to
    // reconstruct an entire graph from a JSON file - it's exactly what runs at every server
    // startup). Reusing that instead of a second, parallel graph (de)serialization path here is
    // what makes this safe: it can never drift from what a normal restart already does.
    public class GraphProfile
    {
        public static GraphProfile Instance { get; } = new GraphProfile();

        private const string ProfilesDir = "graph-profiles";
        private const string DataJsonPath = "data.json";

        private GraphProfile() { }

        private static string PathFor(string name) => Path.Combine(ProfilesDir, SanitizeName(name) + ".json");

        // profile names become file names directly - reject anything that isn't a plain path
        // segment rather than trying to escape it, so "../../etc/passwd" (or a Windows drive
        // path) can't reach outside ProfilesDir.
        private static string SanitizeName(string name)
        {
            if (string.IsNullOrWhiteSpace(name)) throw new ArgumentException("Graph profile name must not be empty");
            if (name.IndexOfAny(Path.GetInvalidFileNameChars()) >= 0 || name.Contains(".."))
                throw new ArgumentException($"Invalid graph profile name: {name}");
            return name;
        }

        public List<string> ListProfiles()
        {
            if (!Directory.Exists(ProfilesDir)) return new List<string>();
            return Directory.GetFiles(ProfilesDir, "*.json")
                .Select(Path.GetFileNameWithoutExtension)
                .Where(n => n != null)
                .Select(n => n!)
                .OrderBy(n => n, StringComparer.OrdinalIgnoreCase)
                .ToList();
        }

        // Save() first so the snapshot reflects whatever's live RIGHT NOW, not whatever data.json
        // happened to hold from the last unrelated save - DB.Save() re-reads Manager's actual
        // current state every time, it never trusts its own in-memory copy for this.
        public void SaveCurrentAs(string name)
        {
            Directory.CreateDirectory(ProfilesDir);
            DB.Instance.Save();
            File.Copy(DataJsonPath, PathFor(name), overwrite: true);
        }

        // Tears down every currently-live source/sink, then reconstructs the graph from the
        // saved snapshot via DB.Load() - the exact same path a server restart takes, just
        // pointed at graph-profiles/<name>.json instead of whatever was already in data.json.
        // Sinks before sources: a sink still bound to a source it's about to lose isn't itself
        // wrong (DeleteSource already unbinds any sink pointing at a deleted source), but
        // deleting in dependency order avoids relying on that unbind path doing the right thing
        // under a bulk teardown it wasn't originally written for.
        public void Activate(string name)
        {
            string path = PathFor(name);
            if (!File.Exists(path)) throw new FileNotFoundException($"No saved graph profile named '{name}'", path);

            foreach (int sinkId in SinkManager.Instance.getAllSinkIds().ToList())
            {
                SinkManager.Instance.DeleteSink(sinkId);
            }
            foreach (int sourceId in SourceManager.Instance.GetAllSourceIds().ToList())
            {
                SourceManager.Instance.DeleteSource(sourceId);
            }

            File.Copy(path, DataJsonPath, overwrite: true);
            DB.Instance.Load();
        }
    }
}
