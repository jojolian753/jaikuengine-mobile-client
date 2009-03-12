package fi.helsinki.cs.context.jabberproxy;

import java.io.IOException;

import java.io.OutputStream;


import java.net.Socket;

import java.text.SimpleDateFormat;
import java.util.Calendar;

import java.util.TimeZone;



import org.apache.log4j.Logger;
import org.xml.sax.Attributes;
import org.xml.sax.helpers.DefaultHandler;


/**
 * 
 * Event handler for SAX events. Parses ContextNetwork data and communicates with CoSe over rmi.
 * 
 * @author Kliment Yanev
 *
 */

public class ContextEventHandler extends DefaultHandler {

    private static final Logger logger = Logger.getLogger(ContextEventHandler.class);
    

    private Socket jabber;
    private OutputStream jOuts;
    
    private String output=NLSMessages.ContextEventHandler_EMPTYSTRING;
 
  
   
    private String lastsent=NLSMessages.ContextEventHandler_EMPTYSTRING;
    private String datetime=NLSMessages.ContextEventHandler_EMPTYSTRING;
    private String namespaces=NLSMessages.ContextEventHandler_EMPTYSTRING;
    private String lastpresence=NLSMessages.ContextEventHandler_EMPTYSTRING;
    private String ournick="";
    private String currentelement="";


    private int level=0;
    private XMLFilterThread filter=null;
    
    /**
     * Constructor
     * @param connection the socket that acknowledgements are sent over 
     */
    public ContextEventHandler(Socket jabberconnection, XMLFilterThread aFilter){
        super();
        //BasicConfigurator.configure();
        
        filter=aFilter;
        jabber=jabberconnection;
        try{
            jOuts=jabber.getOutputStream();
    
        }catch(IOException e){
            logger.error(NLSMessages.ContextEventHandler_WRITER_FROM_SOCKET,e);
            
        }
        

        
    }

    public String getOurNick() {
	return ournick;
    }
    
    public void startPrefixMapping (String prefix, String uri){
    	if (prefix.length()>0) {
    		namespaces+=NLSMessages.ContextEventHandler_NEWLINEXMLNSCOL+prefix+NLSMessages.ContextEventHandler_EQUALSQUOTE+uri+NLSMessages.ContextEventHandler_QUOTE;
    	} else {
    		namespaces+=NLSMessages.ContextEventHandler_NEWLINEXMLNS+NLSMessages.ContextEventHandler_EQUALSQUOTE+uri+NLSMessages.ContextEventHandler_QUOTE;
    	}
    }
    
    /**
     * 
     * Start element handler. Sets state variables and/or writes to output string
     * 
     * @param uri xml namespace
     * @param name local element name
     * @param qName global element name
     * @param attrs element attribute list
     */
    
    public void startElement(String uri, String name, String qName, Attributes attrs){
        level+=1;
        if(NLSMessages.ContextEventHandler_PRESENCE.equalsIgnoreCase(name)){
        	System.out.println(ournick + " Entering presence"); //$NON-NLS-1$

        }
	currentelement=name;
        //System.out.println("Element begins:"+name+getAttrs(attrs));
        if(level==2 && NLSMessages.ContextEventHandler_KEEPALIVE.equalsIgnoreCase(name)){
        	;
        }else if(level==1 && NLSMessages.ContextEventHandler_STREAM.equalsIgnoreCase(name)){
            output+=NLSMessages.ContextEventHandler_XMLHEADER;
            output+=NLSMessages.ContextEventHandler_TAGBEGIN;
            output+=qName;
            output+=getAttrs(attrs);
            output+=namespaces;
            namespaces=NLSMessages.ContextEventHandler_EMPTYSTRING;
            output+=NLSMessages.ContextEventHandler_TAGEND;
            /*try{
                if(null==outs){
                    outs=conn.getOutputStream();
                }
                outs.write(NLSMessages.ContextEventHandler_XMLHEADER_STREAM.getBytes());
                outs.flush();
                
            }catch (IOException e){
                logger.error(NLSMessages.ContextEventHandler_WRITE_TO_SOCKET,e);
            }*/
            jabberSend(output);
            output=NLSMessages.ContextEventHandler_EMPTYSTRING;
            
        } else if(level==2 && "suspend".equalsIgnoreCase(name)) {
		; // special handling, triggered in endElement
        } else if(level==2 && "resume".equalsIgnoreCase(name)) {
		; // special handling, triggered in endElement
        } else {
            
                output+=NLSMessages.ContextEventHandler_TAGBEGIN;
                output+=qName;
                output+=getAttrs(attrs);
                output+=namespaces;
                namespaces=NLSMessages.ContextEventHandler_EMPTYSTRING;
                output+=NLSMessages.ContextEventHandler_TAGEND;
            
        }
        //System.out.println("output string is: "+output);
    }
    
    /**
     * Character data handler. Either updates internal state or generates a bit of the output string.
     * @param ch a character array
     * @param start starting index of string in the character array
     * @param length length of string in the character array
     */
    public void characters(char ch[], int start, int length){
        //System.out.println("Characters entered"+new String(ch, start, length));
    	
    	//if(inId){
    	//	stanzaid+=new String(ch, start, length);
    	//}else{
    		String temp=new String(ch, start, length);
		if (currentelement.equals("username")) {
			ournick+=temp;
		}
    		temp=temp.replace(NLSMessages.ContextEventHandler_AMP, NLSMessages.ContextEventHandler_AMPESC);
    		temp=temp.replace(NLSMessages.ContextEventHandler_LT, NLSMessages.ContextEventHandler_LTESC);
    		output+=temp;
    		//System.out.println("output string is: "+output);
    	//}
        /*}else if(inIdent){
            if(inId){
                stanzaid+=new String(ch, start, length);
            }else if(inName){
                idname+=new String(ch, start, length);
            }else if(inImei){
                imei+=new String(ch, start, length);
            }
        }else{
            if(!new String(ch, start, length).equals(NLSMessages.ContextEventHandler_NEWLINE)){
                logger.debug(NLSMessages.ContextEventHandler_CHARACTERS_OUTSIDE_EVENT_OR_TUPLE+new String(ch, start, length));
            }
        }*/
    }
    /**
     * End tag handler. Updates internal state, ident string, and output string.
     * Calls rmi sender on tuple end.
     * @param uri the xml namespace
     * @param name the local element name
     * @param qName the global element name
     */
    public void endElement(String uri, String name, String qName){
        level-=1;
    	//System.out.println("Element ending"+name);
        if(level==1 && NLSMessages.ContextEventHandler_PRESENCE.equalsIgnoreCase(name)){
        	System.out.println(ournick + " Exiting presence"); //$NON-NLS-1$

        }
	if (name.equals("username")) {
        	filter.getJabberEH().setOurNick(ournick);
	}
        
        if (level==1 && NLSMessages.ContextEventHandler_KEEPALIVE.equalsIgnoreCase(name)){
        	SimpleDateFormat sdf = new SimpleDateFormat(NLSMessages.ContextEventHandler_DATEFORMAT);
        	sdf.setTimeZone(TimeZone.getTimeZone(NLSMessages.ContextEventHandler_TIMEZONE));
        	Calendar cal=Calendar.getInstance();
        	String date = sdf.format(cal.getTime());
        	System.out.println(ournick + " New timestamp is:"+date); //$NON-NLS-1$
        	lastpresence=lastpresence.replaceFirst(NLSMessages.ContextEventHandler_REGEX, date);
        	System.out.println(ournick + " Sending keepalive, "+lastpresence.length()+" characters long"); //$NON-NLS-1$ //$NON-NLS-2$
        	jabberSend(lastpresence);
        } else if(level==1 && "suspend".equalsIgnoreCase(name)) {
        	System.out.println(ournick + " Suspending");
        	filter.getJabberEH().setSuspended(true);
        	output="";
        } else if(level==1 && "resume".equalsIgnoreCase(name)) {
        	System.out.println(ournick + " Resuming");
        	filter.getJabberEH().setSuspended(false);
        	output="";        	
        } else if(level==0 && NLSMessages.ContextEventHandler_STREAM.equalsIgnoreCase(name)){
        	output+=NLSMessages.ContextEventHandler_BEGINTAG_SLASH;
            output+=qName;
            output+=NLSMessages.ContextEventHandler_TAGEND;
        } else {
            output+=NLSMessages.ContextEventHandler_BEGINTAG_SLASH;
            output+=qName;
            output+=NLSMessages.ContextEventHandler_TAGEND;
        }
        if( 1==level || (level==0 && NLSMessages.ContextEventHandler_STREAM.equalsIgnoreCase(name)) ){
            System.out.println(ournick + " output string is: "+output.length()+" characters "); //$NON-NLS-1$ //$NON-NLS-2$
        	System.out.println(ournick + " Sending"); //$NON-NLS-1$
        	jabberSend(output);
        	lastsent=output;
        	if(NLSMessages.ContextEventHandler_PRESENCE.equalsIgnoreCase(name)){
        		lastpresence=lastsent;
        		System.out.println(ournick + " Presence saved"); //$NON-NLS-1$
        	}
        	output=NLSMessages.ContextEventHandler_EMPTYSTRING;
        }
    }
    /**
     * Get the datetime associated with the last event sent
     * @return datetime the datetime in format that is defined in properties
     */
    public String getDateTime(){
        return datetime;
    }
    
    /**
     * Get the String that was last sent
     * @return lastsent the String that was last sent
     */
    public String getLastSent(){
        return lastsent;
    }
    
    
    
    public static String getAttrs(Attributes attrs){
        String attributes=NLSMessages.ContextEventHandler_EMPTYSTRING;
        for(int iterator=0;iterator<attrs.getLength();iterator++){
            attributes+=NLSMessages.ContextEventHandler_SPACE;
            attributes+=attrs.getQName(iterator);
            attributes+=NLSMessages.ContextEventHandler_EQUALSQUOTE;
            String a=attrs.getValue(iterator);
            a=a.replace(NLSMessages.ContextEventHandler_AMP, NLSMessages.ContextEventHandler_AMPESC);
            a=a.replace(NLSMessages.ContextEventHandler_LT, NLSMessages.ContextEventHandler_LTESC);
            attributes+=a.replace(NLSMessages.ContextEventHandler_QUOT, NLSMessages.ContextEventHandler_QUOTESC);
            attributes+=NLSMessages.ContextEventHandler_QUOTE;
        }
        return attributes;
    }
    
    private void jabberSend(String message){
	if (message.length()==0) { return; }
        //System.out.println(NLSMessages.ContextEventHandler_3+message);
    	System.out.println(ournick + " Sending"+message.length()+"characters"); //$NON-NLS-1$ //$NON-NLS-2$
        try{
        	if(null == jOuts){
        		jOuts=jabber.getOutputStream();
        	}
        	jOuts.write(message.getBytes());
        }catch(IOException e){
            logger.error(NLSMessages.ContextEventHandler_NAME_NOT_BOUND,e);
        }
        
    }
}
