
public class hello
  {
  public static void main(String args[])
   {
   for(int i = 0; i < 60; i++)
    {
    System.out.println("Hello, world!");

    System.out.println("Args passed:");
    for(int j = 0; j < args.length; j++)
      System.out.println("\t" + args[j]);
    
    try { Thread.currentThread().sleep(5000); } catch(Exception e) {}
    }

   System.err.println("Exiting.");
   System.exit(0);
   }
 }
