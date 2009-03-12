package fi.helsinki.cs.context.jabberproxy;

import java.io.IOException;
import java.io.OutputStream;
import java.net.Socket;
import java.util.Map;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Collections;

import org.apache.log4j.Logger;
import org.xml.sax.Attributes;
import org.xml.sax.helpers.DefaultHandler;


public class JabberEventHandler extends DefaultHandler {
	
	private static final Logger logger = Logger.getLogger(JabberEventHandler.class);
	private Socket clientsocket=null;
	private OutputStream outs=null;
	private String namespaces=NLSMessages.ContextEventHandler_EMPTYSTRING;
	private String output=NLSMessages.ContextEventHandler_EMPTYSTRING;
	private int level=0;
	
	private boolean suspended=false;
	private Map<String, String> presences=Collections.synchronizedMap(new HashMap<String, String>());
	private String currentnick=new String();
	private String ournick="";
	
	
	public JabberEventHandler(Socket s, XMLFilterThread filter){
		super();
		clientsocket=s;
		filter.setJabberHandler(this);
	}
	
	public void setOurNick(String nick) {
		ournick=nick;
	}

    public void startPrefixMapping (String prefix, String uri){
    	if (prefix.length()>0) {
    		namespaces+=NLSMessages.JabberEventHandler_NEWLINE_XMLNSCOLON+prefix+NLSMessages.ContextEventHandler_EQUALSQUOTE+uri+NLSMessages.ContextEventHandler_QUOTE;
    	} else {
    		namespaces+=NLSMessages.JabberEventHandler_NEWLINE_XMLNS+NLSMessages.ContextEventHandler_EQUALSQUOTE+uri+NLSMessages.ContextEventHandler_QUOTE;
    	}
    }
    public void startElement(String uri, String name, String qName, Attributes attrs){
        level+=1;

        if(NLSMessages.JabberEventHandler_KEEPALIVE.equalsIgnoreCase(name)){
        	;
        }else if(NLSMessages.ContextEventHandler_STREAM.equalsIgnoreCase(name)){
            output+=NLSMessages.ContextEventHandler_XMLHEADER;
            output+=NLSMessages.ContextEventHandler_TAGBEGIN;
            output+=qName;
            output+=ContextEventHandler.getAttrs(attrs);
            output+=namespaces;
            namespaces=NLSMessages.ContextEventHandler_EMPTYSTRING;
            output+=NLSMessages.ContextEventHandler_TAGEND;
  
        	clientSend(output);
            output=NLSMessages.ContextEventHandler_EMPTYSTRING;
            
        
        }else{
            
        		if (NLSMessages.ContextEventHandler_PRESENCE.equalsIgnoreCase(name)) {
        			String nick=attrs.getValue("from");
        			if (nick!=null) {
        				currentnick=nick;
        			} else {
        				currentnick="";
        			}
        		}
                output+=NLSMessages.ContextEventHandler_TAGBEGIN;
                output+=qName;
                output+=ContextEventHandler.getAttrs(attrs);
                output+=namespaces;
                namespaces=NLSMessages.ContextEventHandler_EMPTYSTRING;
                output+=NLSMessages.ContextEventHandler_TAGEND;
            
        }
  
    
    }

    
    public void characters(char ch[], int start, int length){
    	String temp=new String(ch, start, length);
		temp=temp.replace(NLSMessages.JabberEventHandler_AMP, NLSMessages.JabberEventHandler_AMPESC);
		temp=temp.replace(NLSMessages.JabberEventHandler_LT, NLSMessages.JabberEventHandler_LTESC);
		output+=temp;
    }
    
    public void endElement(String uri, String name, String qName){
        level-=1;
  
     
        
        if(level==0 && NLSMessages.ContextEventHandler_STREAM.equalsIgnoreCase(name)){
        	output+=NLSMessages.ContextEventHandler_BEGINTAG_SLASH;
            output+=qName;
            output+=NLSMessages.ContextEventHandler_TAGEND;
            
       
        }else{
            output+=NLSMessages.ContextEventHandler_BEGINTAG_SLASH;
            output+=qName;
            output+=NLSMessages.ContextEventHandler_TAGEND;
        	if (suspended && level==1 && NLSMessages.ContextEventHandler_PRESENCE.equalsIgnoreCase(name) && currentnick.length()>0) {
                System.out.println(ournick + " suspended, storing presence for " + currentnick); //$NON-NLS-1$ //$NON-NLS-2$        		
        		presences.put(currentnick, output);
        		output="";
        	}            
        }
        if(1==level && output.length()>0 ){
            System.out.println(ournick + " output string is: "+output.length()+" characters "); //$NON-NLS-1$ //$NON-NLS-2$
        	System.out.println("Sending"); //$NON-NLS-1$
        	if (suspended && NLSMessages.ContextEventHandler_PRESENCE.equalsIgnoreCase(name)) {
        	} else {
            	clientSend(output);
        	}
        	
        	output=NLSMessages.ContextEventHandler_EMPTYSTRING;
        }
    }
    
    private void sendPresences()  {
    	Iterator<String> iter=presences.keySet().iterator();
        System.out.println(ournick + " Sending saved presences: ");
    	while (iter.hasNext()) {
		String nick=iter.next();
        	System.out.println(ournick + " Sending for " + nick);
    		clientSend(presences.get(nick));
    	}
    }
    public void setSuspended(boolean isSuspended)  {
    	if ( suspended && !isSuspended) {
    		sendPresences();
    		presences.clear();
    	}
    	suspended=isSuspended;
    }
    private synchronized void clientSend(String message)  {
    	try {        
	    	System.out.println(ournick + " Sending"+message.length()+"characters"); //$NON-NLS-1$ //$NON-NLS-2$
	    	if(null == outs){
	    		outs=clientsocket.getOutputStream();
	    	}
    		outs.write(message.getBytes());
    	} catch(Throwable t) {
    		throw new Error(t);
    	}
    }
    
}
