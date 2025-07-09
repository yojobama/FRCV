namespace Server;

public class ManagerWrapper : Manager
{
    public static ManagerWrapper Instance { get; } = new ManagerWrapper();

    public DB db { get; set; }

    public ManagerWrapper()
    {
        db = new DB("data.json");
    }
}