using System;
using System.IO;
using System.Threading.Tasks;

using Microsoft.AspNetCore.Mvc;
using Server.Web;

namespace Server.Controllers
{
    // ROADMAP.md Phase 8/E6: the server's own diagnostic log (DBLog.txt - DB.cs's own single
    // shared Logger instance, the only one this process constructs) had no REST exposure at all
    // until now - the only way to see it was SSH-ing into the coprocessor and tailing the file by
    // hand. Read-only and bounded (a fixed line count, not the whole file): this is for "what's
    // this coprocessor been doing recently", not a general log-shipping endpoint.
    internal class LogController : ControllerBase
    {
        // matches DB.cs's own `new Logger("DBLog.txt")` - a bare relative filename, resolved
        // (by the native Logger's own std::ofstream) against this process's working directory,
        // same as this controller's System.IO.File.ReadAllLinesAsync call below since both run in the
        // same OS process.
        private const string LogFilePath = "DBLog.txt";

        // GET: the last `lines` entries written to the server's own log file, oldest first -
        // empty (not an error) if the log file doesn't exist yet, e.g. immediately after a fresh
        // install before DB.cs's constructor has run.
        [HttpGet("log/tail")]
        public async Task<string[]> Tail([FromQuery] int lines = 200)
        {
            if (!System.IO.File.Exists(LogFilePath)) return Array.Empty<string>();

            string[] all = await System.IO.File.ReadAllLinesAsync(LogFilePath);
            return all.Length <= lines ? all : all[^lines..];
        }
    }
}
