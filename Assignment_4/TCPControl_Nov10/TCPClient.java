import java.io.*;
import java.net.*;
public class TCPClient {

	public static void main(String[] args) {
	        try{
	          Socket socket = new Socket("127.0.0.1", 9999);
	          PrintWriter out = new PrintWriter(socket.getOutputStream(), true);
	          int count = 0;
	          for (int i = 0 ; i<100; i++){
	              for (int j = 0 ; j<100; j++){
	                  float x =(float) (0.2*Math.random()-0.1);
	                  float y = (float) (0.2*Math.random()-0.1);
	                  float z = (float) (0.2*Math.random()-0.1);
	                  float xr =(float) (0.2*Math.random()-0.1);
	                  float yr =(float) (0.2*Math.random()-0.1);
	                  float zr =(float) (0.2*Math.random()-0.1);
	                  count++;
	                  out.println( x+" "+y+" "+z+" "+xr+" "+yr+" "+zr);
	                  System.out.println( count);
	              }
	          }
	          out.close();
	          socket.close();
	          System.out.println("*** Socke is closed ****");
	        } catch(IOException e){
				e.printStackTrace();
				System.out.println("error in TCP Client Side");
			}
    }
}