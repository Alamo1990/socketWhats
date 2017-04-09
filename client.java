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
private static String currUser = "";
private static receiveThread listener;
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
		DataOutputStream s = new DataOutputStream(sc.getOutputStream());
		DataInputStream istream = new DataInputStream(sc.getInputStream());

		// Send command to server
		s.writeBytes("REGISTER\0");
		s.writeBytes(user);
		s.writeByte('\0');

		s.flush();

		byte[] msg = new byte[1];
		// Read response from server
		int bytesRead = istream.read(msg);
		res = msg[0];
		sc.close();
	}catch(Exception ex) {
		ex.printStackTrace();
		return RC.ERROR;
	}

	switch(res) {
		case 0:
		System.out.println("c> REGISTER OK");
		return RC.OK;
		case 1:
		System.out.println("c> REGISTER IN USE");
		return RC.USER_ERROR;
		case 2:
		System.out.println("c> REGISTER FAIL");
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

		// Send command to server
		s.writeBytes("UNREGISTER\0");
		s.writeBytes(user);
		s.writeByte('\0');

		s.flush();

		byte[] msg = new byte[1];
		// Read response from server
		int bytesRead = istream.read(msg);
		res = msg[0];
		sc.close();

	}catch( Exception e) {
		e.printStackTrace();
		return RC.ERROR;
	}
	switch (res) {
		case 0:
		System.out.println("c> UNREGISTER OK");
		return RC.OK;
		case 1:
		System.out.println("c> USER DOES NOT EXIST");
		return RC.USER_ERROR;
		case 2: System.out.println("c> UNREGISTER FAIL");
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

if( !"".equals(currUser) ){
	System.out.println("c> USER ALREADY CONNECTED");
	return RC.ERROR;
}

	ServerSocket serverSc;
	int port;
	try{
		Socket sc = new Socket(_server, _port);
		DataOutputStream s = new DataOutputStream(sc.getOutputStream());
		DataInputStream istream = new DataInputStream(sc.getInputStream());

		 serverSc = new ServerSocket(0);
		 port = serverSc.getLocalPort();

		// Update user of the client
		currUser = user;

		// Send command to server
		s.writeBytes("CONNECT\0");
		s.writeBytes(user);
		s.writeBytes("\0");
		s.writeBytes(Integer.toString(port));
		s.writeBytes("\0");

		s.flush();
		// Read server response
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
									System.out.println("c> CONNECT OK");
									listener = new receiveThread(port,serverSc);
									listener.start();
									return RC.OK;
	case 1:
									System.out.println("c> CONNECT FAIL, USER DOES NOT EXIST");
									// serverSc.close();
									return RC.USER_ERROR;
	case 2:
									System.out.println("c> USER ALREADY CONNECTED");
									// serverSc.close();
									return RC.ERROR;
	case 3:
									System.out.println("c> CONNECT FAIL");
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

																Socket sc = new Socket(_server, _port);
																DataOutputStream s = new DataOutputStream(sc.getOutputStream());
																DataInputStream istream = new DataInputStream(sc.getInputStream());

																// Send command to server
																s.writeBytes("DISCONNECT\0");
																s.writeBytes(user);
																s.writeByte('\0');

																s.flush();
																// Receive server response
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
																System.out.println("c> DISCONNECT OK");
																//listener.stop();
																currUser = "";
																return RC.OK;
								case 1:
																System.out.println("c> DISCONNECT FAIL / USER DOES NOT EXIST");
																return RC.USER_ERROR;
								case 2: System.out.println("c> DISCONNECT FAIL / USER NOT CONNECTED");
																return RC.USER_ERROR;
								case 3: System.out.println("c> DISCONNECT FAIL");
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

								if( "".equals(currUser) ){
									System.out.println("c> SEND FAIL");
									return RC.ERROR;
								}

								if(message.length() > 255) {
																System.out.println("c> SEND FAIL"); //ASK
																return RC.USER_ERROR;
								}else if(currUser == null) {
																System.out.println("c> SEND FAIL"); //ASK
																return RC.USER_ERROR;
								}

								try {
																Socket sc = new Socket(_server, _port);
																DataOutputStream s = new DataOutputStream(sc.getOutputStream());
																DataInputStream istream = new DataInputStream(sc.getInputStream());

																// Send command to server
																s.writeBytes("SEND\0");
																s.writeBytes(currUser);
																s.writeByte('\0');
																s.writeBytes(user);
																s.writeByte('\0');
																s.writeBytes(message);
																s.writeByte('\0');

																s.flush();
																// Receive response from server
																byte[] msg = new byte[1];
																byte[] msg_id = new byte[8];
																int bytesRead = istream.read(msg);
																bytesRead = istream.read(msg_id);
																res = msg[0];

																sc.close();

																switch(res) {
																case 0:
																								System.out.println("c> SEND OK - MESSAGE "+ new String(msg_id));
																								return RC.OK;
																case 1:
																								System.out.println("c> SEND FAIL / USER DOES NOT EXIST");
																								return RC.USER_ERROR;
																case 2:
																								System.out.println("c> SEND FAIL");
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
								System.exit(0);
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

// Thread listening for messages
class receiveThread extends Thread{
	 int port;
	 ServerSocket serverSc;

	 public receiveThread(int port, ServerSocket serverSc){
		 this.port = port;
		 this.serverSc = serverSc;
	 }


		public void run(){
			try{
				while(true){

				Socket clientSocket = this.serverSc.accept();

				DataOutputStream s = new DataOutputStream(clientSocket.getOutputStream());
				InputStream istream = clientSocket.getInputStream();

				InputStreamReader isr = new InputStreamReader(clientSocket.getInputStream());
				BufferedReader in = new BufferedReader(isr);
				String line = "";
				boolean end = false;

				String command = "";
				String usr = "";
				String id = "";
				String msg = "";
				char character;

				int x = 0;
				// Count number of messages received
				int count = 0;
				// If socket has data to be read
				if(in.ready()){
					// Read 4 messages
					while(count < 4){
						// Read one byte
						x= in.read();
						if(count == 4){
							break;
						}else if( x == 0){
							count++; // If null character readed one word has been sent
							continue;
						}else if(x == (int)('\0')){
							count++; // If null character readed one word has been sent
							continue;
						}else{
							switch(count){
								case 0:
								 command += (char)(x); // Read command
									break;
									case 1:
									usr += (char)(x); // Read username
									break;
									case 2:
									id += (char)(x); // Read id
									break;
									case 3:
									msg += (char)(x); // Read message
									break;
							}
						}
					}//while if count < 4

					// Check if message received is a normal message
					if(command.equals("SEND_MESSAGE")){
						System.out.println(" MESSAGE " + id + " FROM " + usr +
					 																			":\n" + msg + "\nEND\n");
					}else{
						// An ACK message is received
						System.out.println(" SEND MESSAGE " + id + " OK\n");
					}

			 }// if ready
					// Close client socket
					clientSocket.close();
					System.out.print("c> ");
				}// while(true)
			}catch( Exception e) {
				e.printStackTrace();
			}
		}
}
