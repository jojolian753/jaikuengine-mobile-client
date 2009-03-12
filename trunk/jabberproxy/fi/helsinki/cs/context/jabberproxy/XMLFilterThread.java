package fi.helsinki.cs.context.jabberproxy;

//import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.net.InetSocketAddress;
import java.net.Socket;
import java.net.SocketAddress;


import org.apache.log4j.Logger;
import org.xml.sax.InputSource;
import org.xml.sax.SAXException;
import org.xml.sax.XMLReader;
import org.xml.sax.helpers.XMLReaderFactory;

/**
 * Is spawned on each new connection by the ContextProxy. Parses XML and handles the connection
 * @author kliment
 *
 */

public class XMLFilterThread extends Thread {
    
    private static final Logger logger = Logger.getLogger(XMLFilterThread.class);
    
    private JabberEventHandler jabbereh=null;
    private Socket receiversocket;
    
    /**
     * Constructor, sets internal socket
     * @param recvsocket a socket that was returned by accept()
     */
    public XMLFilterThread(Socket recvsocket) {
        super();
        //BasicConfigurator.configure();
        //System.out.println("Created thread");
        this.receiversocket=recvsocket;
    }
    
    public void setJabberHandler(JabberEventHandler jeh){
    	jabbereh=jeh;
    }
    
    public JabberEventHandler getJabberEH(){
    	return jabbereh;
    }
    
    
    /**
     * Receives XML stream from the socket and parses it using the ContextEventHandler
     * Closes socket when done.
     */
    public void run(){
        //System.out.println("Running thread");
    	Socket jabber = null;
    	JabberConnectionThread jct = null;
        try{
        	jabber = new Socket();
        	SocketAddress sa = new InetSocketAddress(ContextProxy.getJabberAddress(), ContextProxy.getJabberPort());
            jabber.connect(sa);
        	XMLReader anXMLReader = XMLReaderFactory.createXMLReader();
        	jct = new JabberConnectionThread(jabber, receiversocket, this);
        	jct.start();
            ContextEventHandler aHandler = new ContextEventHandler(jabber, this);
            anXMLReader.setContentHandler(aHandler);
            anXMLReader.setErrorHandler(aHandler);

            InputStreamReader reader = new InputStreamReader(receiversocket.getInputStream());

            anXMLReader.parse(new InputSource(reader));
          
        }catch(IOException e){
            logger.error(NLSMessages.XMLFilterThread_READ_FROM_SOCKET, e);            
        }catch(SAXException e){
            logger.error(NLSMessages.XMLFilterThread_UNABLE_TO_GET_READER, e);
        } catch (Error e) {
            logger.error("Unhandled error", e);
        } finally {
        	if (jabber!=null) { try { jabber.close();} catch(Throwable t) { logger.error("Error in close of jabber socekt", t); } }
        	if (receiversocket!=null) { try { receiversocket.close();} catch(Throwable t) { logger.error("Error in close of client socket", t); } }
        }
    }
    
    /**
     * Gets the internal socket
     * @return receivesocket the socket that data is received on
     */
    public Socket getsocket(){
        return receiversocket;
    }
    
}
