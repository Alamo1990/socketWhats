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
	try{
		Socket sc = new Socket(_server, _port);
		DataOutputStream s = new DataOutputStream(sc.getOutputStream());
		DataInputStream istream = new DataInputStream(sc.getInputStream());

		receiveSc = new Socket(InetAddress.getLocalHost().getHostAddress(), 0);
		int port = receiveSc.getLocalPort();
		listener = new Thread(){
			@Override
			public void run(){

											try{
																			DataInputStream istrem = new DataInputStream(receiveSc.getInputStream());

																			while(true) {
																											if(((String)istrem.readBytes()).equals("SEND_MESSAGE")) {
																																			String usr = (String)istrem.readBytes();
																																			String id = (String)istrem.readBytes();
																																			String msg = (String)istrem.readBytes();

																																			System.out.println("c> MESSAGE " + id + " FROM " + usr +
																																																						":\n" + msg + "\nEND\n");
																											}else{} //ASK
																			} //TODO

											}catch( Exception e) {
																			e.printStackTrace();
																			return;
											}

			}
		};

		s.writeBytes("CONNECT\0");
		s.writeBytes(user);
		s.writeBytes("\0");
		s.writeBytes(Integer.toString(port));
		s.writeBytes("\0");

		s.flush();

		res = istream.readChar();

		sc.close();
		listener.start();

	}catch( Exception e) {
									e.printStackTrace();
									return RC.ERROR;
	}
	switch (res) {
	case 0:
									System.out.println("CONNECT OK");
									return RC.OK;
	case 1:
									System.out.println("CONNECT FAIL, USER DOES NOT EXIST");
									return RC.USER_ERROR;
	case 2:
									System.out.println("USER ALREADY CONNECTED");
									return RC.ERROR;
	case 3:
									System.out.println("CONNECT FAIL");
									return RC.ERROR;
	default:
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

																s.writeBytes("DISCONNECT\0");
																s.writeBytes(user);
																s.writeBytes("\0");

																s.flush();

																res = istream.readChar();

																sc.close();

																listener.stop(); //REVIEW
																receiveSc.close();

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

																s.writeBytes("SEND\0");
																s.writeBytes(currUser);
																s.writeBytes("\0");
																s.writeBytes(user);
																s.writeBytes("\0");
																s.writeBytes(message);
																s.writeBytes("\0");

																s.flush();

																sc.close();
																switch(istream.readChar()) {
																case 0:
																								System.out.println("c> SEND OK - MESSAGE "+istream.readChar());
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
																																																String message = input.substring(input.indexOf(' ')+1).substring(input.indexOf(' ')+1);
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

public static void main(String[] argv)
{
								if(!parseArguments(argv)) {
																usage();
																return;
								}

								// Write code here

								shell();
								System.out.println("+++ FINISHED +++");
}
}
