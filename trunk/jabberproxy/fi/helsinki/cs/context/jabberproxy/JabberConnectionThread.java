package fi.helsinki.cs.context.jabberproxy;

import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.net.Socket;

import org.apache.log4j.Logger;
import org.xml.sax.InputSource;
import org.xml.sax.SAXException;
import org.xml.sax.XMLReader;
import org.xml.sax.helpers.XMLReaderFactory;

public class JabberConnectionThread extends Thread {
	
	private static final Logger logger = Logger.getLogger(JabberConnectionThread.class);
	private Socket jabbersocket=null;
	private Socket clientsocket=null;
	private XMLFilterThread xft=null;
	
	public JabberConnectionThread(Socket jabber, Socket client, XMLFilterThread filterthread){
		super();
		jabbersocket=jabber;
		clientsocket=client;
		xft=filterthread;
	}
	
	public void run(){
		
		  try{
	        	
	        	XMLReader anXMLReader = XMLReaderFactory.createXMLReader();
	        	JabberEventHandler aHandler = new JabberEventHandler(clientsocket, xft);
	            anXMLReader.setContentHandler(aHandler);
	            anXMLReader.setErrorHandler(aHandler);

	            InputStreamReader reader = new InputStreamReader(jabbersocket.getInputStream());

	            anXMLReader.parse(new InputSource(reader));
	          
	        } catch(IOException e){
	            logger.error(NLSMessages.XMLFilterThread_READ_FROM_SOCKET, e);
	            
	        } catch(SAXException e){
	            logger.error(NLSMessages.XMLFilterThread_UNABLE_TO_GET_READER, e);
	        } finally {
	        	if (jabbersocket!=null) { try { jabbersocket.close();} catch(Throwable t) { logger.error("Error in close of jabber socekt", t); } }
	        	if (clientsocket!=null) { try { clientsocket.close();} catch(Throwable t) { logger.error("Error in close of client socket", t); } }
	        }
			
		
	}
}
