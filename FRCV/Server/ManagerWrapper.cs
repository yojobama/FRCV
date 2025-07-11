namespace Server;

// basicly a singleton manager instance for all of the server to use
public class ManagerWrapper : Manager
{
    // Singleton instance of ManagerWrapper
    public static ManagerWrapper Instance { get; } = new ManagerWrapper();

    private ManagerWrapper() {}
}