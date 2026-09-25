using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

using Microsoft.AspNetCore.Mvc;
using Server.Web;

namespace Server.Controllers.sources
{
    internal class SourceController : ControllerBase
    {
        // GET: Source activation status
        [HttpGet("source/isActive")]
        public Task<bool> IsActive([FromQuery] int SourceID)
        {
            bool status = SourceManager.Instance.IsSourceActive(SourceID);
            return Task.FromResult(status);
        }

        // GET: All registered sources;
        [HttpGet("source/getAll")]
        public Task<Source[]> GetAllSources()
        {
            List<Source> sources = new List<Source>();
            
            foreach (var item in SourceManager.Instance.GetAllSourceIds())
                sources.Add(SourceManager.Instance.GetSourceById(item));
            
            return Task.FromResult(sources.ToArray());
        }

        // PATCH: Rename am ImageFile source;
        [HttpPatch("source/rename")]
        public Task Rename([FromQuery] int SourceID, [FromQuery] string newName)
        {
            SourceManager.Instance.ChangeSourceName(SourceID, newName);
            return Task.CompletedTask;
        }

        // DELETE: delete a source;
        [HttpDelete("source/delete")]
        public Task Delete([FromQuery] int SourceID)
        {
            SourceManager.Instance.DeleteSource(SourceID);
            return Task.CompletedTask;
        }

        // POST: save this source's most recently published frame to disk (ROADMAP.md Phase 7).
        // fileName only, not a full path - resolved under a fixed snapshots/ directory (created
        // on first use) rather than a caller-supplied path, so this can't be used to write
        // somewhere unintended on the coprocessor's filesystem. Returns false if the source has
        // never published a frame yet.
        [HttpPost("source/snapshot")]
        public Task<bool> SaveSnapshot([FromQuery] int SourceID, [FromQuery] string fileName)
        {
            string snapshotDir = Path.Combine(AppContext.BaseDirectory, "snapshots");
            Directory.CreateDirectory(snapshotDir);
            string safeFileName = Path.GetFileName(fileName); // strips any directory components a caller tried to sneak in
            string fullPath = Path.Combine(snapshotDir, safeFileName);
            bool saved = ManagerWrapper.Instance.SaveSnapshot(SourceID, fullPath);
            return Task.FromResult(saved);
        }
    }
}
