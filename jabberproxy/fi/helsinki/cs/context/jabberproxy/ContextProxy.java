package fi.helsinki.cs.context.jabberproxy;

import java.io.IOException;
import java.net.ServerSocket;
import java.net.Socket;

import org.apache.log4j.BasicConfigurator;
import org.apache.log4j.Logger;


/**
 * A multithreaded proxy server that packages each incoming tuple from an 
 * XML stream into an rmi message, which is then sent to the CoServer.
 * 
 * @author Kliment Yanev
 *
 */


public class ContextProxy {
    
    private static final Logger logger = Logger.getLogger(ContextProxy.class);
    
    private static final int QUEUELENGTH = 100;
    private static final int PORTNUMBER = 0;
    private static final int URLNUMBER = 1;
    private static final int JABBERPORTNUMBER = 2;
    private static final int DATEFORMATNUMBER = 3;
    private static String dateformatstring = NLSMessages.ContextProxy_DEFAULTDATEFORMAT;
    private static String jabberaddr = NLSMessages.ContextProxy_DEFAULTADDURL;
    private static int jabberport = 5222;
    private static boolean socketready=false;
    private static ServerSocket listensocket=null;
    
    
    
    /**
     * Whether the socket is ready and listening
     * @return socketready is the socket ready
     */
    public static boolean isSocketReady(){
        return socketready;
    }
    
    /**
     * Returns the url of the Add service optionally set by arg2
     * @return jndiaddurl the URL
     */
    public static String getJabberAddress(){
        return jabberaddr;
    }
    public static int getJabberPort() {
    	return jabberport;
    }
    
    /**
     *Stops listening after the next request 
     *
     */
    public static void stopListening(){
        socketready=false;
    }
    /**
     * Gets the date format that is optionally set by arg3. Default is yyyy-MM-dd'T'HH:mm:ss.S
     * @return dateformat a format string of the type accepted by simpledateformatter
     */
    public static String getDateFormat(){
        return dateformatstring;
    } 

    /**
     * Accepts connections and spawns threads to handle them
     * @param args
     */
    public static void main(String[] args) {
        BasicConfigurator.configure();
        //System.setProperty("javax.xml.parsers.DocumentBuilderFactory", "org.apache.xerces.jaxp.DocumentBuilderFactoryImpl");
        //System.setProperty("org.xml.sax.driver", "org.apache.crimson.parser.XMLReaderImpl");
        
        dateformatstring = NLSMessages.ContextProxy_DEFAULTDATEFORMAT;
        jabberaddr = NLSMessages.ContextProxy_DEFAULTADDURL;
        
        if(args.length<1){
            System.err.println(NLSMessages.ContextProxy_SUPPLY_PORT_NUMBER);
            return;
        }
        
        if(args.length>URLNUMBER){
            jabberaddr=args[URLNUMBER];
        }
        if(args.length>JABBERPORTNUMBER) {
        	jabberport=Integer.parseInt(args[JABBERPORTNUMBER]);
        }
        
        if(args.length>DATEFORMATNUMBER){
            dateformatstring=args[DATEFORMATNUMBER];
        }
        
        try{
            listensocket = new ServerSocket(Integer.parseInt(args[PORTNUMBER]), QUEUELENGTH);
            socketready=true;
            //System.out.println("listening on socket");
            while(socketready){
                
                Socket receivesocket = listensocket.accept();
                //System.out.println("accepted connection");
                (new XMLFilterThread(receivesocket)).start();
            }
        }catch(IOException e){
            logger.error(NLSMessages.ContextProxy_SOCKET_OPERATION_FAILED, e);
            return;
        }catch(NumberFormatException e){
            logger.error(NLSMessages.ContextProxy_ILLEGAL_NUMBER_FORMAT, e);
            return;
        }
        

    }

}
