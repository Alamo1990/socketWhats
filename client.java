import java.lang.*;
import java.io.*;
import java.net.*;
import java.util.*;
import gnu.getopt.Getopt;


class client {

/********************* TYPES **********************/

/**
 * @brief Return codes for the protocol methods
 */
private static enum RC {
								OK,
								ERROR,
								USER_ERROR
};

/******************* ATTRIBUTES *******************/

private static String _server   = null;
private static int _port = -1;
private static String currUser = null;
private static Thread listener;
private static Socket receiveSc; //ASK


/********************* METHODS ********************/

/**
 * @param user - User name to register in the system
 *
 * @return OK if successful
 * @return USER_ERROR if the user is already registered
 * @return ERROR if another error occurred
 */
static RC register(String user){

	int res = -1;

	try {
		Socket sc = new Socket(_server, _port);
		OutputStream ostream = sc.getOutputStream();
		//ObjectOutput s = new ObjectOutputStream(ostream);
		DataOutputStream s = new DataOutputStream(sc.getOutputStream());
		DataInputStream istream = new DataInputStream(sc.getInputStream());


		/*MIRAR EL PRINT WRITTER*/
		// All strings must end with \0
		//s.writeObject("REGISTER");
		//s.writeObject(user);
		s.writeBytes("REGISTER\0");
		s.writeBytes(user);
		s.writeByte('\0');

		s.flush();

		byte[] msg = new byte[1];
		//res = istream.readChar();
		int bytesRead = istream.read(msg);
		res = msg[0];
		sc.close();
	}catch(Exception ex) {
		ex.printStackTrace();
		return RC.ERROR;
	}

	switch(res) {
		case 0:
		System.out.println("REGISTER OK");
		return RC.OK;
		case 1:
		System.out.println("REGISTER IN USE");
		return RC.USER_ERROR;
		case 2:
		System.out.println("REGISTER FAIL");
		return RC.ERROR;
		default:
		return RC.ERROR;
	}

}

/**
 * @param user - User name to unregister from the system
 *
 * @return OK if successful
 * @return USER_ERROR if the user does not exist
 * @return ERROR if another error occurred
 */
static RC unregister(String user){
	int res = -1;
	try{
		Socket sc = new Socket(_server, _port);
		DataOutputStream s = new DataOutputStream(sc.getOutputStream());
		DataInputStream istream = new DataInputStream(sc.getInputStream());

		s.writeBytes("UNREGISTER\0");
		s.writeBytes(user);
		s.writeByte('\0');

		s.flush();

		byte[] msg = new byte[1];
		//res = istream.readChar();
		int bytesRead = istream.read(msg);
		res = msg[0];
		sc.close();

	}catch( Exception e) {
		e.printStackTrace();
		return RC.ERROR;
	}
	switch (res) {
		case 0:
		System.out.println("UNREGISTER OK");
		return RC.OK;
		case 1:
		System.out.println("USER DOES NOT EXIST");
		return RC.USER_ERROR;
		case 2: System.out.println("UNREGISTER FAIL");
		return RC.ERROR;
		default:
		return RC.ERROR;
	}
}





/**
 * @param user - User name to connect to the system
 *
 * @return OK if successful
 * @return USER_ERROR if the user does not exist or if it is already connected
 * @return ERROR if another error occurred
 */
static RC connect(String user){

	int res = -1;
	ServerSocket serverSc;
	int port;
	try{
		Socket sc = new Socket(_server, _port);
		DataOutputStream s = new DataOutputStream(sc.getOutputStream());
		DataInputStream istream = new DataInputStream(sc.getInputStream());

		// ServerSocket serverSc = new ServerSocket(0);
		// int port = serverSc.getLocalPort();
		 serverSc = new ServerSocket(0);
		 port = serverSc.getLocalPort();

		/*listener = new Thread(){
			@Override
			public void run(){
				System.out.println("Waiting for server");

											try{
																			Socket recSc = serverSc.accept();
																			DataInputStream istrem = new DataInputStream(recSc.getInputStream());

																			while(true) {
																											if((istrem.readLine()).equals("SEND_MESSAGE")) {
																																			System.out.println("Message received");
																																			String usr = istrem.readLine();
																																			String id = istrem.readLine();
																																			String msg = istrem.readLine();

																																			System.out.println("MESSAGE " + id + " FROM " + usr +
																																																						":\n" + msg + "\nEND\n");
																											}else{} //ASK
																			}

											}catch( Exception e) {
																			e.printStackTrace();
																			return;
											}

			}
		};
*/
		currUser = user; // set the current user name

		s.writeBytes("CONNECT\0");
		s.writeBytes(user);
		s.writeBytes("\0");
		s.writeBytes(Integer.toString(port));
		s.writeBytes("\0");

		s.flush();

		byte[] msg = new byte[1];
		int bytesRead = istream.read(msg);
		res = msg[0];
		sc.close();
	}catch( Exception e) {
									e.printStackTrace();
									return RC.ERROR;
	}
	switch (res) {
	case 0:
									System.out.println("CONNECT OK");
									//listener.start();
									new receiveThread(port,serverSc).start();
									return RC.OK;
	case 1:
									System.out.println("CONNECT FAIL, USER DOES NOT EXIST");
									// serverSc.close();
									return RC.USER_ERROR;
	case 2:
									System.out.println("USER ALREADY CONNECTED");
									// serverSc.close();
									return RC.ERROR;
	case 3:
									System.out.println("CONNECT FAIL");
									// serverSc.close();
									return RC.ERROR;
	default:
									// serverSc.close();
									return RC.ERROR;
	}
}

/**
 * @param user - User name to disconnect from the system
 *
 * @return OK if successful
 * @return USER_ERROR if the user does not exist
 * @return ERROR if another error occurred
 */
static RC disconnect(String user){
								int res = -1;
								try{
																// Socket sc = new Socket(_server, _port);
																// DataOutputStream s = new DataOutputStream(sc.getOutputStream());
																// DataInputStream istream = new DataInputStream(sc.getInputStream());
																//
																// s.writeBytes("DISCONNECT\0");
																// s.writeBytes(user);
																// s.writeBytes("\0");
																//
																// s.flush();
																//
																// res = istream.readChar();
																//
																// sc.close();
																//
							//listener.stop(); //REVIEW
																// receiveSc.close();
																Socket sc = new Socket(_server, _port);
																DataOutputStream s = new DataOutputStream(sc.getOutputStream());
																DataInputStream istream = new DataInputStream(sc.getInputStream());

																currUser = null;

																s.writeBytes("DISCONNECT\0");
																s.writeBytes(user);
																s.writeByte('\0');

																s.flush();

																byte[] msg = new byte[1];
																int bytesRead = istream.read(msg);
																res = msg[0];
																sc.close();

								}catch( Exception e) {
																e.printStackTrace();
																return RC.ERROR;
								}
								switch (res) {
								case 0:
																System.out.println("DISCONNECT OK");
																return RC.OK;
								case 1:
																System.out.println("DISCONNECT FAIL / USER DOES NOT EXIST");
																return RC.USER_ERROR;
								case 2: System.out.println("DISCONNECT FAIL / USER NOT CONNECTED");
																return RC.USER_ERROR;
								case 3: System.out.println("DISCONNECT FAIL");
																return RC.ERROR;
								default:
																return RC.ERROR;
								}
}

/**
 * @param user    - Receiver user name
 * @param message - Message to be sent
 *
 * @return OK if the server had successfully delivered the message
 * @return USER_ERROR if the user is not connected (the message is queued for delivery)
 * @return ERROR the user does not exist or another error occurred
 */
static RC send(String user, String message){
 							int res = -1;
								if(message.length() > 255) {
																System.out.println("SEND FAIL"); //ASK
																return RC.USER_ERROR;
								}else if(currUser == null) {
																System.out.println("SEND FAIL"); //ASK
																return RC.USER_ERROR;
								}

								try {
																// CHECK MESSAGE LENGTH ?
																Socket sc = new Socket(_server, _port);
																DataOutputStream s = new DataOutputStream(sc.getOutputStream());
																DataInputStream istream = new DataInputStream(sc.getInputStream());

																s.writeBytes("SEND\0");
																s.writeBytes(currUser);
																s.writeByte('\0');
																s.writeBytes(user);
																s.writeByte('\0');
																s.writeBytes(message);
																s.writeByte('\0');

																s.flush();

																byte[] msg = new byte[1];
																byte[] msg_id = new byte[8];
																int bytesRead = istream.read(msg);
																bytesRead = istream.read(msg_id);
																res = msg[0];

																sc.close();

																switch(res) {
																case 0:


																								System.out.println("SEND OK - MESSAGE "+ new String(msg_id));
																								return RC.OK;
																case 1:
																								System.out.println("SEND FAIL / USER DOES NOT EXIST");
																								return RC.USER_ERROR;
																case 2:
																								System.out.println("SEND FAIL");
																								return RC.ERROR;
																default: return RC.ERROR;
																}
								}catch(Exception ex) {
																ex.printStackTrace();
																return RC.ERROR;
								}

}

/**
 * @brief Command interpreter for the client. It calls the protocol functions.
 */
static void shell()
{
								boolean exit = false;
								String input;
								String [] line;
								BufferedReader in = new BufferedReader(new InputStreamReader(System.in));

								while (!exit) {
																try {
																								System.out.print("c> ");
																								input = in.readLine();
																								line = input.split("\\s");

																								if (line.length > 0) {
																																/*********** REGISTER *************/
																																if (line[0].equals("REGISTER")) {
																																								if  (line.length == 2) {
																																																register(line[1]); // userName = line[1]
																																								} else {
																																																System.out.println("Syntax error. Usage: REGISTER <userName>");
																																								}
																																}

																																/********** UNREGISTER ************/
																																else if (line[0].equals("UNREGISTER")) {
																																								if  (line.length == 2) {
																																																unregister(line[1]); // userName = line[1]
																																								} else {
																																																System.out.println("Syntax error. Usage: UNREGISTER <userName>");
																																								}
																																}

																																/************ CONNECT *************/
																																else if (line[0].equals("CONNECT")) {
																																								if  (line.length == 2) {
																																																connect(line[1]); // userName = line[1]
																																								} else {
																																																System.out.println("Syntax error. Usage: CONNECT <userName>");
																																								}
																																}

																																/********** DISCONNECT ************/
																																else if (line[0].equals("DISCONNECT")) {
																																								if  (line.length == 2) {
																																																disconnect(line[1]); // userName = line[1]
																																								} else {
																																																System.out.println("Syntax error. Usage: DISCONNECT <userName>");
																																								}
																																}

																																/************** SEND **************/
																																else if (line[0].equals("SEND")) {
																																								if  (line.length >= 3) {
																																																// Remove first two words
																																																//String message = input.substring(input.indexOf(' ')+1).substring(input.indexOf(' ')+1);
																																																String message = input.substring(input.indexOf(' ')+1);
																																																message = message.substring(message.indexOf(' ')+1);
																																																send(line[1], message); // userName = line[1]
																																								} else {
																																																System.out.println("Syntax error. Usage: SEND <userName> <message>");
																																								}
																																}

																																/************** QUIT **************/
																																else if (line[0].equals("QUIT")) {
																																								if (line.length == 1) {
																																																exit = true;
																																								} else {
																																																System.out.println("Syntax error. Use: QUIT");
																																								}
																																}

																																/************* UNKNOWN ************/
																																else {
																																								System.out.println("Error: command '" + line[0] + "' not valid.");
																																}
																								}
																} catch (java.io.IOException e) {
																								System.out.println("Exception: " + e);
																								e.printStackTrace();
																}
								}
}

/**
 * @brief Prints program usage
 */
static void usage()
{
								System.out.println("Usage: java -cp . client -s <server> -p <port>");
}

/**
 * @brief Parses program execution arguments
 */
static boolean parseArguments(String [] argv)
{
								Getopt g = new Getopt("client", argv, "ds:p:");

								int c;
								String arg;

								while ((c = g.getopt()) != -1) {
																switch(c) {
																//case 'd':
																//	_debug = true;
																//	break;
																case 's':
																								_server = g.getOptarg();
																								break;
																case 'p':
																								arg = g.getOptarg();
																								_port = Integer.parseInt(arg);
																								break;
																case '?':
																								System.out.print("getopt() returned " + c + "\n");
																								break; // getopt() already printed an error
																default:
																								System.out.print("getopt() returned " + c + "\n");
																}
								}

								if (_server == null)
																return false;

								if ((_port < 1024) || (_port > 65535)) {
																System.out.println("Error: Port must be in the range 1024 <= port <= 65535");
																return false;
								}

								return true;
}



/********************* MAIN **********************/

public static void main(String[] argv){
								if(!parseArguments(argv)) {
																usage();
																return;
								}

								// Write code here

								shell();
								System.out.println("+++ FINISHED +++");
}
}


class receiveThread extends Thread{
	int port;
	ServerSocket serverSc;
	public receiveThread(int port, ServerSocket serverSc){
		this.port = port;
		this.serverSc = serverSc;
	}
	private String read(InputStream in){
		String ret = "";
		int b;
		try{
			while((b = in.read()) > 0) ret += (char)b;
		}catch(Exception e){
			e.printStackTrace();
		}
		System.out.println("Message received: "  + ret);
		return ret;
	}
		public void run(){
			// try {
   //         while (true) {
   //             Socket socket = serverSc.accept();
   //             try {
   //                 PrintWriter out =
   //                     new PrintWriter(socket.getOutputStream(), true);
   //                 out.println(new Date().toString());
   //             } finally {
   //                 socket.close();
   //             }
   //         }
   //     }catch( Exception e) {
			// 						try{
   //         serverSc.close();
			// 							}catch( Exception ee) {
			// 								ee.printStackTrace();
			// 							}
   //     }



			try{
				System.out.println("HELLO IM A THREAD with port " + this.port);
				//ServerSocket sc = new ServerSocket(port);
				Socket clientSocket = this.serverSc.accept();
				System.out.println("Connection received from " + clientSocket.getInetAddress().getHostName());

				DataOutputStream s = new DataOutputStream(clientSocket.getOutputStream());
				InputStream istream = clientSocket.getInputStream();
				//byte[] msg = new byte[256];

				// InputStreamReader isr = new InputStreamReader(clientSocket.getInputStream());
				// BufferedReader in = new BufferedReader(isr);
				// String line = "";

				while(true){

					if( read(istream).equals("SEND_MESSAGE")) {

													String usr =  read(istream);
													String id =  read(istream);
													String msg =  read(istream);

													System.out.println("MESSAGE " + id + " FROM " + usr +
																																":\n" + msg + "\nEND\n");
					}else{} //ASK
				// 			 //char []character =  new char[1];
				// 				String command = "";
				// 				String usr = "";
				// 				String id = "";
				// 				String msg = "";
				// 				char character;
				// 				// while( (character = (char) (in.read()) ) != '\0' ){
				// 				// 	System.out.println(character);
				// 				// 	command += character;
				// 				// }
				//
				// 				int x = 0;
				// 				// while(  x != (int)('\0') ){
				// 				// 		//System.out.println("WEKoooo " + x);
				// 				// 		command += (char)(x);
				// 				// 		x = in.read();
				// 				// }
				// 				int count = 0;
				// 				while(  x!=-1 ){
				// 						System.out.println("WEKoooo " + x);
				// 						x= in.read();
				// 						if(count == 4){
				// 							break;
				// 						}else if( x == 0){
				// 							count++;
				// 							continue;
				// 						}else if(x == (int)('\0')){
				// 							System.out.println("char 0 found");
				// 							count++;
				// 							continue;
				// 						}else{
				// 							switch(count){
				// 								case 0:
				// 									command += (char)(x);
				// 									break;
				// 									case 1:
				// 									usr += (char)(x);
				// 									break;
				// 									case 2:
				// 									id += (char)(x);
				// 									break;
				// 									case 3:
				// 									msg += (char)(x);
				// 									break;
				// 							}
				//
				// 						}
				//
				//
				// 						// command += (char)(x);
				// 						// x = in.read();
				// 				}


								// System.out.println(command);
								// System.out.println(usr);
								// System.out.println(count);
								// System.out.println("MESSAGE " + id + " FROM " + usr +
								// 																			":\n" + msg + "\nEND\n");

								/*while ((line = in.readLine()) != null) {
					    //System.out.println(line);
					    //response = response + line + "\n";

									if(line.equals("SEND_MESSAGE")){
										String usr = in.readLine();
										String id = in.readLine();
										String msg = in.readLine();
	ServerSocket serverSc;


										System.out.println("c> MESSAGE " + id + " FROM " + usr +
																													":\n" + msg + "\nEND\n");
									}
					    if (in.ready() == false) {
					        break;
					    }
					}*/
				//	System.out.println("c> MESSAGE " + id + " FROM " + usr +	":\n" + msg + "\nEND\n");
				}

				//
				//
				// //String msg;
				// //res = istream.readChar();
				// byte m = istream.readByte();
				// int count = 0;
				// while(m!='\0'){
				// 	msg[0] = m;
				// 	m = istream.readByte();
				// }
				// System.out.println("RESPONSE: " + msg);
				// //sc.close();
			}catch( Exception e) {
												e.printStackTrace();
				}

		}




}
