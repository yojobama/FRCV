namespace ConsoleApp1
{
    internal class Program
    {
        static void Main(string[] args)
        {
            Console.WriteLine("Hello, World!");
            
            Logger logger = new Logger();

            logger.enterLog("I am a doo doo head!");
            var a = logger.GetAllLogs();

            foreach (var item in a)
            {
                Console.WriteLine(item.GetMessage());
            }
        }
    }
}
