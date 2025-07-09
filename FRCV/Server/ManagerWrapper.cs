namespace Server;

public class ManagerWrapper : Manager
{
    // Singleton instance of ManagerWrapper
    public static ManagerWrapper Instance { get; } = new ManagerWrapper();

    // class members
    private DB db;
    private SourceManager sourceManager;
    private SinkManager sinkManager;
    
    // class properties
    public DB Db { get => db; set => db = value; }
    
    
    
    // constructor is private to prevent instantiation from outside
    private ManagerWrapper()
    {
        db = new DB("data.json");
        sourceManager = new SourceManager();
        sinkManager = new SinkManager(ref sourceManager);
    }
}